/**
 *
 * @file pastix_subtask_symbfact.c
 *
 * PaStiX symbolic factorizations task.
 * Contains wrappers to the symbolic factorization step.
 * Affetcted by the compilation time options:
 *    - PASTIX_SYMBOL_FORCELOAD: Force to load the symbol matrix from file
 *    - PASTIX_SYMBOL_DUMP_SYMBMTX: Dump the symbol matrix in a postscript file.
 *    - COMPACT_SMX: Optimization for solve step (TODO: check if not obsolete)
 *    - FORGET_PARTITION: Force to forget the precomputed partition
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "spm.h"
#include "order.h"
#include "symbol.h"
#if defined(PASTIX_DISTRIBUTED)
#include "csc_utils.h"
#include "cscd_utils_intern.h"
#endif /* defined(PASTIX_DISTRIBUTED) */

/**
 *******************************************************************************
 *
 * @ingroup pastix_analyze
 *
 * @brief Computes the symbolic factorization step.
 *
 * Computes the symbolic matrix structure and if required the amalgamated
 * supernode partition.
 *
 * The function is a *centralized* algorithm to generate the symbol matrix
 * structure associated to the problem. It takes as input the ordemesh structure
 * (permutation array, inverse permutation array, and optionnal supernodes
 * array) and returns the modified ordemesh structure if changed, and the
 * symbolic structure.
 *  - If (PT-)Scotch has been used, it generates the structure with
 * symbolFaxGraph() thanks to the supernode partition given by Scotch.
 *  - If ILU(k) factorization will be performed or if the ordering tools didn't
 * provide the supernode partition, symbolKass() is used to generate both
 * supernode partition and associated symbol matrix structure.
 *
 * Both algorithms are working with a centralized version of the graph and are
 * on every nodes. If a distributed graph has been used, it is gather on each
 * node to compute the symbol matrix.
 * If symbolKass() is used, the ordering structure will be modified due to the
 * assembly step. Then, permutation, inverse permutation, partition, and
 * partition tree are modified internally. The new permutation arrays perm and
 * invp can be returned to the user if and invp vector will be modified and
 * returned to the user. The associated partition with its tree will be updated
 * accordingly.
 * BE CAREFULL if you give your own ordering and wants to keep it because it
 * will be overwritten by the updated one.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_INCOMPLETE, IPARM_LEVEL_OF_FILL,
 *   IPARM_AMALGAMATION_LVLCBLK, IPARM_AMALGAMATION_LVLBLAS,
 *   IPARM_IO_STRATEGY, IPARM_FLOAT, IPARM_FACTORIZATION
 *
 * On exit, the following parameters are set:
 *   IPARM_NNZEROS, DPARM_FACT_THFLOPS, DPARM_FACT_RLFLOPS
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *          On exit, the field symbmtx is initialized with the symbol matrix,
 *          and the field ordemesh is updated if the supernode partition is
 *          computed.
 *          - IPARM_INCOMPLETE switches the factorization mode from direct to ILU(k).
 *          - IPARM_LEVEL_OF_FILL defines the level of incomplete factorization
 *            if IPARM_INCOMPLETE == 1. If IPARM_LEVEL_OF_FILL < 0, the
 *            full pattern is generated as for direct factorization.
 *          - IPARM_AMALGAMATION_LVLCBLK is the ratio of amalgamation allowed
 *            based on reducing the number of supernodes only.
 *          - IPARM_AMALGAMATION_LVLBLAS is the ratio of amalgamation allowed
 *            based on reducing the computational cost (solve for ILU(k), or
 *            factorization for direct factorization).
 *          - IPARM_IO_STRATEGY will enable to load/store the result to files.
 *          If set to PastixIOSave, the symbmtx and the generated ordemesh is
 *          dump to file.
 *          If set to APÏ_IO_LOAD, the symbmtx (only) is loaded from the files.
 *
 * @param[inout] perm
 *          Array of size n.
 *          On entry, unused.
 *          On exit, if perm != NULL, contains the permutation array generated.
 *
 * @param[inout] invp
 *          Array of size n.
 *          On entry, unused.
 *          On exit, if invp != NULL, contains the inverse permutation array generated.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 * @retval PASTIX_ERR_OUTOFMEMORY if one allocation failed.
 * @retval PASTIX_ERR_INTEGER_TYPE if Scotch integer type is not the
 *         same size as PaStiX ones.
 * @retval PASTIX_ERR_INTERNAL if an error occurs internally to Scotch.
 *
 *******************************************************************************/
int
pastix_subtask_symbfact( pastix_data_t *pastix_data,
                         pastix_int_t  *perm,
                         pastix_int_t  *invp )
{
    pastix_int_t   *iparm;
    double         *dparm;
    pastix_graph_t *graph;
    Order          *ordemesh;
    pastix_int_t    n;
    int             procnum;
    Clock           timer;

#if defined(PASTIX_DISTRIBUTED)
    pastix_int_t           * PTS_perm     = pastix_data->PTS_permtab;
    pastix_int_t           * PTS_rev_perm = pastix_data->PTS_peritab;
    pastix_int_t           * tmpperm      = NULL;
    pastix_int_t           * tmpperi      = NULL;
    pastix_int_t             gN;
    pastix_int_t             i;
#endif

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_symbfact: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    iparm = pastix_data->iparm;
    dparm = pastix_data->dparm;

    if ( !(pastix_data->steps & STEP_ORDERING) ) {
        errorPrint("pastix_subtask_symbfact: pastix_subtask_order() has to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    procnum  = pastix_data->procnum;
    graph    = pastix_data->graph;
    ordemesh = pastix_data->ordemesh;

    if (graph == NULL) {
        errorPrint("pastix_subtask_symbfact: the pastix_data->graph field has not been initialized, pastix_subtask_order should be called first");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (ordemesh == NULL) {
        errorPrint("pastix_subtask_symbfact: the pastix_data->ordemesh field has not been initialized, pastix_subtask_order should be called first");
        return PASTIX_ERR_BADPARAMETER;
    }
    n = ordemesh->vertnbr;

    clockStart(timer);

    /* Make sure they are both 0-based */
    orderBase( ordemesh, 0 );
    graphBase( graph, 0 );

    print_debug(DBG_STEP, "-> pastix_subtask_symbfact\n");
    if (iparm[IPARM_VERBOSE] > PastixVerboseNot)
        pastix_print(procnum, 0, OUT_STEP_FAX );

    /* Allocate the symbol matrix structure */
    if (pastix_data->symbmtx == NULL) {
        MALLOC_INTERN( pastix_data->symbmtx, 1, SymbolMatrix );
    }
    else {
        errorPrint("pastix_subtask_symbfact: Symbol Matrix already allocated !!!");
    }

    /* Force Load of symbmtx */
#if defined(PASTIX_SYMBOL_FORCELOAD)
    iparm[IPARM_IO_STRATEGY] = PastixIOLoad;
#endif

    /*Symbol matrix loaded from file */
    if ( iparm[IPARM_IO_STRATEGY] & PastixIOLoad )
    {
        FILE *stream;
        PASTIX_FOPEN(stream, "symbname", "r" );
        symbolLoad( pastix_data->symbmtx, stream );
        fclose(stream);
    }
    /* Symbol matrix computed through Fax or Kass */
    else
    {
        int fax;
        pastix_int_t  nfax;
        pastix_int_t *colptrfax;
        pastix_int_t *rowfax;

        /* Check correctness of parameters */
        if (iparm[IPARM_INCOMPLETE] == 0)
        {
#if defined(COMPACT_SMX)
            if (procnum == 0)
                errorPrintW("COMPACT_SMX only works with incomplete factorization, force ILU(%d) factorization.",
                            iparm[IPARM_LEVEL_OF_FILL]);
            iparm[IPARM_INCOMPLETE] = 1;
#endif
        }
        /* End of parameters check */

        if ( ((iparm[IPARM_ORDERING] == PastixOrderScotch) ||
              (iparm[IPARM_ORDERING] == PastixOrderPtscotch)) &&
             (iparm[IPARM_SF_KASS] == 0) )
        {
            fax = 1;
        }
        else {
            fax = 0;
        }

        /*
         * Fax/Kass work with centralized interface, we convert the cscd to csc if required
         */
#if defined(PASTIX_DISTRIBUTED)
        if (graph->loc2glob != NULL)
        {
            cscd2csc_int( graph->n,
                          graph->colptr,
                          graph->rows,
                          NULL, NULL, NULL, NULL,
                          &nfax, &colptrfax, &rowfax,
                          NULL, NULL, NULL, NULL,
                          graph->loc2glob,
                          pastix_data->pastix_comm,
                          iparm[IPARM_DOF_NBR], 1);
        }
        else
#endif
        {
            nfax      = graph->n;
            colptrfax = graph->colptr;
            rowfax    = graph->rows;
        }

        symbolInit(pastix_data->symbmtx);
        pastix_data->symbmtx->dof = graph->dof;

        /*
         * The amalgamate supernodes partition has been found with (PT-)Scotch,
         * we use it to generate the symbol matrix structure.
         * This works only if direct factorization will be performed.
         */
        if (fax)
        {
            if (iparm[IPARM_VERBOSE] > PastixVerboseNot)
                pastix_print(procnum, 0, OUT_FAX_METHOD, "Fax " );
            symbolFaxGraph(pastix_data->symbmtx, /* Symbol Matrix   */
                           nfax,                 /* Number of nodes */
                           colptrfax,            /* Nodes list      */
                           rowfax,               /* Edges list      */
                           ordemesh);
        }
        /*
         * The amalgamate supernodes partition doesn't exist. (PT-)Scotch has
         * not been used, or ILU(k) factorization is performed and then, we
         * dropped the partition found by Scotch.
         * Kass is used to generate both the amalgamate supernode partition and
         * the symbol matrix stucture in this case.
         */
        else
        {
            pastix_graph_t tmpgraph;
            tmpgraph.gN     = nfax;
            tmpgraph.n      = nfax;
            tmpgraph.colptr = colptrfax;
            tmpgraph.rows   = rowfax;
            tmpgraph.loc2glob = NULL;

            if (iparm[IPARM_VERBOSE] > PastixVerboseNot)
                pastix_print(procnum, 0, OUT_FAX_METHOD, "Kass" );
            symbolKass(iparm[IPARM_VERBOSE],
                       iparm[IPARM_INCOMPLETE],
                       iparm[IPARM_LEVEL_OF_FILL],
                       iparm[IPARM_AMALGAMATION_LVLCBLK],
                       iparm[IPARM_AMALGAMATION_LVLBLAS],
                       pastix_data->symbmtx,
                       &tmpgraph,
                       ordemesh,
                       pastix_data->pastix_comm);

#if defined(PASTIX_DISTRIBUTED)
            if (PTS_perm != NULL)
            {
                gN = n;

                MALLOC_INTERN(tmpperm, gN, pastix_int_t);
                MALLOC_INTERN(tmpperi, gN, pastix_int_t);
                for (i = 0; i < gN; i++)
                    tmpperm[i] = ordemesh->permtab[PTS_perm[i]-1];

                memFree_null(ordemesh->permtab);
                ordemesh->permtab = tmpperm;

                for (i = 0; i < gN; i++)
                    tmpperi[i] = PTS_rev_perm[ordemesh->peritab[i]]-1;
                memFree_null(ordemesh->peritab);
                ordemesh->peritab = tmpperi;

                memFree_null(PTS_perm);
                memFree_null(PTS_rev_perm);
            }
#endif /* defined(PASTIX_DISTRIBUTED) */

            /*
             * Save the new ordering structure
             */
            if ( iparm[IPARM_IO_STRATEGY] & PastixIOSave )
            {
                if (procnum == 0) {
                    orderSave( ordemesh, NULL );
                }
            }

            /*
             * Return the new ordering to the user
             */
            if (perm != NULL) memcpy(perm, ordemesh->permtab, n*sizeof(pastix_int_t));
            if (invp != NULL) memcpy(invp, ordemesh->peritab, n*sizeof(pastix_int_t));
        }

        /* Set the beginning of the Schur complement */
        pastix_data->symbmtx->schurfcol = nfax - pastix_data->schur_n + pastix_data->symbmtx->baseval;

        if ( graph->loc2glob != NULL )
        {
            memFree_null(colptrfax);
            memFree_null(rowfax);
        }
    } /* not PastixIOLoad */

    /* Rebase to 0 */
    symbolBase( pastix_data->symbmtx, 0 );

    /* Rustine to be sure we have a tree
     * TODO: check difference with kassSymbolPatch */
#define RUSTINE
#ifdef RUSTINE
    symbolRustine( pastix_data->symbmtx,
                   pastix_data->symbmtx );
#endif

    /* Build the browtabs and Realign data structure */
    symbolBuildRowtab( pastix_data->symbmtx );
    symbolRealloc( pastix_data->symbmtx );

#if !defined(NDEBUG)
    if ( orderCheck( ordemesh ) != 0) {
        errorPrint("pastix_subtask_symbfact: orderCheck on final ordering after symbolic factorization failed !!!");
        assert(0);
    }
    if( symbolCheck(pastix_data->symbmtx) != 0 ) {
        errorPrint("pastix_subtask_symbfact: symbolCheck on final symbol matrix failed !!!");
        assert(0);
    }
#endif

    /*
     * Save the symbolic factorization
     */
    if ( iparm[IPARM_IO_STRATEGY] & PastixIOSave )
    {
        if (procnum == 0) {
            FILE *stream;
            PASTIX_FOPEN(stream, "symbgen", "w");
            symbolSave(pastix_data->symbmtx, stream);
            fclose(stream);
        }
    }

    /*
     * Dump an eps file of the symbolic factorization
     */
#if defined(PASTIX_SYMBOL_DUMP_SYMBMTX)
    if (procnum == 0)
    {
        FILE *stream;
        PASTIX_FOPEN(stream, "symbol.eps", "w");
        symbolDraw(pastix_data->symbmtx,
                   stream);
        fclose(stream);
    }
#endif

    /*
     * Computes statistics and print informations
     */
    iparm[IPARM_NNZEROS] = symbolGetNNZ( pastix_data->symbmtx );
    symbolGetFlops( pastix_data->symbmtx,
                    iparm[IPARM_FLOAT], iparm[IPARM_FACTORIZATION],
                    &(dparm[DPARM_FACT_THFLOPS]),
                    &(dparm[DPARM_FACT_RLFLOPS]) );

    clockStop(timer);

    if ( procnum == 0 ) {
        if (iparm[IPARM_VERBOSE] > PastixVerboseNo)
            symbolPrintStats( pastix_data->symbmtx );

        if ( iparm[IPARM_VERBOSE] > PastixVerboseNot ) {
            double fillin = (double)(iparm[IPARM_NNZEROS])
                / (double)( (pastix_data->csc)->gnnz );

            pastix_print( procnum, 0, OUT_FAX_SUMMARY,
                          (long)iparm[IPARM_NNZEROS],
                          fillin, clockVal(timer) );
        }
    }

    /* Invalidate following steps, and add order step to the ones performed */
    pastix_data->steps &= ~( STEP_ANALYSE |
                             STEP_NUMFACT |
                             STEP_SOLVE   |
                             STEP_REFINE  );
    pastix_data->steps |= STEP_SYMBFACT;

    return PASTIX_SUCCESS;
}
