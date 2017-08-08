/**
 *
 * @file pastix_subtask_blend.c
 *
 * PaStiX analyse blend subtask function
 *
 * @copyright 2004-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "spm.h"
#include "order.h"
#include "perf.h"
#include "elimintree.h"
#include "cost.h"
#include "cand.h"
#include "extendVector.h"
#include "blendctrl.h"
#include "solver.h"
#include "simu.h"
#include "blend.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_analyze
 *
 * @brief Compute the proportional mapping and the final solver structure.
 *
 * This function computes the structural information required to factorize
 * and solve the problem. It requires an ordering structure, as well as the
 * symbolic factorization structure. It results a solver structure that contains
 * all informations architecture and problem dependent to efficiently solve the
 * system.
 * On exit, the symbol structure is destroyed and only local uncompressed
 * information is stored in the solver structure.
 *
 * This routine is affected by, or returns, the following parameters:
 *   IPARM_ABS, IPARM_CUDA_NBR, IPARM_DISTRIBUTION_LEVEL, IPARM_DOF_NBR,
 *   IPARM_FACTORIZATION, IPARM_FLOAT, IPARM_GPU_CRITERIUM,
 *   IPARM_GPU_MEMORY_PERCENTAGE, IPARM_GPU_NBR, IPARM_INCOMPLETE,
 *   IPARM_MAX_BLOCKSIZE, IPARM_MIN_BLOCKSIZE ,IPARM_NNZEROS,
 *   IPARM_NNZEROS_BLOCK_LOCAL, IPARM_STARPU, IPARM_THREAD_NBR, IPARM_VERBOSE
 *
 *   DPARM_ANALYZE_TIME, DPARM_FACT_FLOPS, DPARM_FACT_RLFLOPS,
 *   DPARM_FACT_THFLOPS, DPARM_FILL_IN, DPARM_PRED_FACT_TIME, DPARM_SOLV_FLOPS
 *
 * This function is constructed as a sequence of steps that are described below.
 *
 * #### Construct an elimination tree
 *   A elimination tree structure is constructed out of the symbol matrix to be
 *   able to traverse the tree in a top-down fashion for the proportionnal
 *   mapping step.
 *
 * #### Construct the cost matrix
 *   For each column-block, and block of the symbolic structure, the cost of
 *   each operation is computed to evaluate the cost of each branch of the
 *   elimination tree. Costs of the blocks are the cost of the update generated
 *   out of this block when used as the B matrix in the GEMM update. Costs of
 *   the column block is the total cost associated to it: factorization, solve,
 *   and update in a right-looking algorithm. This means that the update cost is
 *   the one generated by this column block, and not the one received by the
 *   column-block.
 *
 * #### Construct the candidate array
 *   Dispatch properties such as low-rank compression, 2D tasks from the top to
 *   the bottom of the tree. Candidate array, and elimination tree are computed,
 *   and updated, simultaneously with the costs computed previously.
 *   This step is impacted by IPARM_DISTRIBUTION_LEVEL that defines the minimal
 *   width of nodes which can forward low-rank and 3D tasks properties to their
 *   son.
 *
 * #### Proportionnal Mapping
 *   This step performs the actual proportional mapping algorithm to define the
 *   subset of candidates to compute each supernode.
 *
 * #### Split symbol matrix
 *   Once the proportionnal mapping is performed on the original set of
 *   supernodes, the symbol matrix is split in smaller supernodes/blocks to
 *   allow for more parallelism.
 *
 * #### Simulation
 *   The simulation step defines the actual mapping per core of each supernode
 *   based on a simulation of the numerical factorization where each task is
 *   attributed to the first resource available to compute it.
 *
 * #### Solver structure generation
 *   Out of the previous step, the solver generator builds the local structure
 *   that is required for the numerical factorization and solve steps. It is
 *   mainly represented by a CSC like structure of the local blocks, linked to a
 *   CSR for the solve step and the structure that will holds the coefficients
 *   of the factorized matrix.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 * @retval PASTIX_ERR_OUTOFMEMORY if one allocation failed.
 *
 *******************************************************************************/
int
pastix_subtask_blend( pastix_data_t *pastix_data )
{
    BlendCtrl       ctrl;
    pastix_int_t    procnum, verbose;
    pastix_int_t   *iparm;
    double         *dparm;
    pastix_order_t *ordeptr;
    symbol_matrix_t   *symbmtx;
    SolverMatrix   *solvmtx;
    SimuCtrl       *simuctrl;
    double          timer_all     = 0.;
    double          timer_current = 0.;

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_blend: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( !(pastix_data->steps & STEP_SYMBFACT) ) {
        errorPrint("pastix_subtask_blend: pastix_subtask_symbfact() has to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    iparm   = pastix_data->iparm;
    dparm   = pastix_data->dparm;
    procnum = pastix_data->inter_node_procnum;
    ordeptr = pastix_data->ordemesh;
    symbmtx = pastix_data->symbmtx;
    verbose = iparm[IPARM_VERBOSE];

    if (ordeptr == NULL) {
        errorPrint("pastix_subtask_blend: the pastix_data->ordemesh field has not been initialized, pastix_task_order should be called first");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (symbmtx == NULL) {
        errorPrint("pastix_subtask_blend: the pastix_data->symbmtx has not been initialized, pastix_task_symbfact should be called first");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (symbmtx->dof < 1) {
        errorPrint("pastix_subtask_blend: Dof number has not been correctly initialized");
        return PASTIX_ERR_BADPARAMETER;
    }

    /* Free graph structure, we don't need it anymore */
    if (pastix_data->graph != NULL) {
        graphExit( pastix_data->graph );
        memFree_null( pastix_data->graph );
    }

    /* Cleanup the solver structure if we already computed it */
    if ( pastix_data->solvmatr != NULL ) {
        solverExit( pastix_data->solvmatr );
        memFree_null( pastix_data->solvmatr );
    }
    solvmtx = (SolverMatrix*)malloc(sizeof(SolverMatrix));
    pastix_data->solvmatr = solvmtx;

    /* Start the analyze step */
    clockStart(timer_all);
    if ( verbose > PastixVerboseNot ) {
        pastix_print( procnum, 0, OUT_STEP_BLEND );
    }

    /* Create the control structure that parameterize the analyze step */
    blendCtrlInit( pastix_data, &ctrl );

    if( verbose > PastixVerboseNo)
        pastix_print( procnum, 0, OUT_BLEND_CONF,
                      (long)ctrl.clustnbr, (long)ctrl.local_nbcores, (long)ctrl.local_nbthrds);

    /* Verify the coherence of the initial symbol matrix */
    if(ctrl.debug)
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_CHKSMBMTX );
        }
        symbolCheck(symbmtx);
    }

    /* Build the elimination tree from the symbolic partition */
    {
        if( verbose > PastixVerboseYes) {
            pastix_print( procnum, 0, OUT_BLEND_ELIMTREE );
        }
        clockStart(timer_current);

        ctrl.etree = eTreeBuild(symbmtx);

        clockStop(timer_current);
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_ELIMTREE_TIME,
                          clockVal(timer_current) );
        }
    }

    /* Build the cost matrix from the symbolic partition */
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_COSTMATRIX );
        }
        clockStart(timer_current);

        ctrl.costmtx = costMatrixBuild( symbmtx,
                                         iparm[IPARM_FLOAT],
                                         iparm[IPARM_FACTORIZATION] );

        clockStop(timer_current);
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_COSTMATRIX_TIME,
                          clockVal(timer_current) );
        }
    }

    /* Build the candtab array to store candidate information on each cblk */
    {
        MALLOC_INTERN(ctrl.candtab, symbmtx->cblknbr, Cand);
        candInit( ctrl.candtab, symbmtx->cblknbr );

        /* Initialize costs in elimination tree and candtab array for proportionnal mapping */
        candBuild( ctrl.autolevel,
                   ctrl.level2D,
                   ctrl.ratiolimit,
                   ctrl.candtab,
                   ctrl.etree,
                   symbmtx,
                   ctrl.costmtx );
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_ELIMTREE_TOTAL_COST,
                          ctrl.etree->nodetab[ eTreeRoot(ctrl.etree) ].subtree );
        }
    }

    /* Proportional mapping step that distributes the candidates over the tree */
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_PROPMAP );
        }
        clockStart(timer_current);

        propMappTree( ctrl.candtab,
                      ctrl.etree,
                      ctrl.total_nbcores,
                      ctrl.nocrossproc,
                      ctrl.allcand );

        /* Set the cluster candidates according to the processor candidates */
        candSetClusterCand( ctrl.candtab, symbmtx->cblknbr,
                            ctrl.core2clust, ctrl.total_nbcores );

        clockStop(timer_current);

        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_PROPMAP_TIME,
                          clockVal(timer_current) );
        }

        /* Let's check the result if ask */
        if ( ctrl.debug ) {
            assert( candCheck( ctrl.candtab, symbmtx ) );
        }
    }

    /*
     * Split the existing symbol matrix according to the number of candidates
     * and cblk types.
     * It takes the original symbol and candtab, and return the new symbol and
     * candtab. If the symbmtx is modified, the costmtx is updated, as well as
     * the tree.
     */
    if(1)
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_SPLITSYMB );
        }
        clockStart(timer_current);

        splitSymbol(&ctrl, symbmtx);

        clockStop(timer_current);
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_SPLITSYMB_TIME,
                          clockVal(timer_current) );
        }
    }

    if(ctrl.count_ops && (ctrl.leader == procnum)) {
        symbolGetFlops( symbmtx,
                        iparm[IPARM_FLOAT],
                        iparm[IPARM_FACTORIZATION],
                        &(dparm[DPARM_FACT_THFLOPS]),
                        &(dparm[DPARM_FACT_RLFLOPS]) );
    }

#if defined(PASTIX_SYMBOL_DUMP_SYMBMTX)
    {
        FILE *stream = NULL;
        stream = pastix_fopenw( &(pastix_data->dirtemp), "symbol_after_split.eps", "w" );
        if ( stream ) {
            symbolDraw( symbmtx, stream );
            fclose( stream );
        }
    }
#endif

    if (0)
    {
        FILE *file = NULL;
        file = pastix_fopenw( &(pastix_data->dirtemp), "symbol_after_split", "w" );
        if ( file ) {
            symbolSave( symbmtx, file );
            fclose( file );
        }
    }

    /* Simulation step to perform the data distribution over the nodes and compute the priorities of each task */
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_BUILDSIMU );
        }
        clockStart(timer_current);

        /* Initialize simulation structure */
        MALLOC_INTERN(simuctrl, 1, SimuCtrl);
        simuInit( simuctrl, symbmtx, ctrl.candtab,
                  ctrl.clustnbr,
                  ctrl.total_nbcores );

        /* Create task array */
        simuTaskBuild(simuctrl, symbmtx, ctrl.candtab);
        clockStop(timer_current);

        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_BUILDSIMU_TIME,
                          clockVal(timer_current),
                          (long)simuctrl->tasknbr );
        }

        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_SIMU );
        }
        clockStart(timer_current);

        simuRun( simuctrl, &ctrl, symbmtx );

        clockStop(timer_current);
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_SIMU_TIME,
                          clockVal(timer_current) );
        }
    }

#ifdef PASTIX_DYNSCHED
    /**
     * If dynamic scheduling is asked, let's perform a second proportionnal
     * mapping step:
     *    - this is made only on local data
     *    - no crossing is allowed between branches
     */
    {
        clockStart(timer_current);

        splitPartLocal( &ctrl, simuctrl, symbmtx );

        clockStop(timer_current);
        if( verbose>PastixVerboseNo)
            pastix_print( procnum, 0, "  -- Split build at time: %g --\n", clockVal(timer_current));
    }
#endif

    /* CostMatrix and Elimination Tree are no further used */
    costMatrixExit( ctrl.costmtx );
    memFree_null( ctrl.costmtx );
    eTreeExit( ctrl.etree );

    /**
     * Generate the final solver structure that collects data from the different
     * simulation structures and convert to local numbering
     */
    {
        if( verbose > PastixVerboseYes ) {
            pastix_print( procnum, 0, OUT_BLEND_SOLVER );
        }
        clockStart(timer_current);

        solverMatrixGen(ctrl.clustnum, solvmtx, symbmtx, simuctrl, &ctrl);

        clockStop(timer_current);
        if( verbose > PastixVerboseNo ) {
            pastix_print( procnum, 0, OUT_BLEND_SOLVER_TIME,
                          clockVal(timer_current) );
            if( verbose > PastixVerboseYes ) {
                solverPrintStats( solvmtx );
            }
        }
    }

    /* Free allocated memory */
    simuExit(simuctrl, ctrl.clustnbr, ctrl.total_nbcores, ctrl.local_nbctxts);

    /* Realloc solver memory in a contiguous way  */
    {
        solverRealloc(solvmtx);

#if defined(PASTIX_DEBUG_BLEND)
        if (!ctrl.ricar) {
            if( verbose > PastixVerboseYes ) {
                pastix_print( procnum, 0, OUT_BLEND_CHKSOLVER );
            }
            solverCheck(solvmtx);
        }
#endif
    }

    blendCtrlExit(&ctrl);

    /* End timing */
    clockStop(timer_all);
    set_dparm(dparm, DPARM_ANALYZE_TIME, clockVal(timer_all) );

    if (verbose > PastixVerboseYes)
        symbolPrintStats( pastix_data->symbmtx );

    /* Symbol is not used anymore */
    symbolExit(pastix_data->symbmtx);
    memFree_null(pastix_data->symbmtx);

    /* Computes and print statistics */
    {
        if (iparm[IPARM_FACTORIZATION] == PastixFactLU)
        {
            iparm[IPARM_NNZEROS]        *= 2;
            dparm[DPARM_PRED_FACT_TIME] *= 2.;
        }
        dparm[DPARM_SOLV_FLOPS] = (double)iparm[IPARM_NNZEROS]; /* number of operations for solve */

        iparm[IPARM_NNZEROS_BLOCK_LOCAL] = solvmtx->coefnbr;

        /* Affichage */
        dparm[DPARM_FILL_IN] = (double)(iparm[IPARM_NNZEROS]) / (double)(pastix_data->csc->gnnzexp);

        if (verbose > PastixVerboseNot) {
            pastix_print( 0, 0, OUT_BLEND_SUMMARY,
                          (long)iparm[IPARM_NNZEROS],
                          (double)dparm[DPARM_FILL_IN],
                          pastixFactotypeStr( iparm[IPARM_FACTORIZATION] ),
                          printflopsv( dparm[DPARM_FACT_THFLOPS] ), printflopsu( dparm[DPARM_FACT_THFLOPS] ),
                          PERF_MODEL, dparm[DPARM_PRED_FACT_TIME],
                          dparm[DPARM_ANALYZE_TIME] );

            if (0) /* TODO: consider that when moving to distributed */
            {
                if ((verbose > PastixVerboseNo))
                {
                    fprintf(stdout, NNZERO_WITH_FILLIN, (int)procnum, (long)iparm[IPARM_NNZEROS_BLOCK_LOCAL]);
                }
                if (verbose > PastixVerboseYes)
                {
#ifdef PASTIX_WITH_MPI
                    MPI_Comm pastix_comm = pastix_data->inter_node_comm;
#endif
                    pastix_int_t sizeL = solvmtx->coefnbr;
                    pastix_int_t sizeG = 0;

                    MPI_Reduce(&sizeL, &sizeG, 1, PASTIX_MPI_INT, MPI_MAX, 0, pastix_comm);

                    if (procnum == 0)
                    {
                        sizeG *= sizeof(pastix_complex64_t);
                        if (iparm[IPARM_FACTORIZATION] == PastixFactLU)
                            sizeG *= 2;

                        fprintf(stdout, OUT_COEFSIZE, (double)MEMORY_WRITE(sizeG),
                                MEMORY_UNIT_WRITE(sizeG));
                    }
                }
            }
        }
    }

    /* Backup the solver for debug */
    if (0)
    {
        FILE *file = NULL;
        file = pastix_fopenw( &(pastix_data->dirtemp), "solvergen", "w" );
        if ( file ) {
            solverSave( solvmtx, file );
            fclose(file);
        }
    }

    /* Invalidate following steps, and add analyze step to the ones performed */
    pastix_data->steps &= ~( STEP_NUMFACT |
                             STEP_SOLVE   |
                             STEP_REFINE  );
    pastix_data->steps |= STEP_ANALYSE;

    return PASTIX_SUCCESS;
}
