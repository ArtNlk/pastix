/**
 *
 * @file pastix_task_analyse.c
 *
 *  PaStiX analyse routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "order.h"
#include "param_blend.h"
#include "perf.h"
#include "blend.h"

pastix_int_t sizeofsolver(SolverMatrix *solvptr, pastix_int_t *iparm);

/*
  Function: pastix_task_blend

  Distribution task.

  Parameters:
  pastix_data - PaStiX data structure
  pastix_comm - PaStiX MPI communicator

*/
void pastix_task_blend(pastix_data_t *pastix_data)
{
    Dof            dofstr;
    BlendParam     blendpar;
    MPI_Comm       pastix_comm = pastix_data->inter_node_comm;
    pastix_int_t   procnbr  = pastix_data->inter_node_procnbr;
    pastix_int_t   procnum  = pastix_data->inter_node_procnum;
    pastix_int_t  *iparm    = pastix_data->iparm;
    double        *dparm    = pastix_data->dparm;
    SolverMatrix  *solvmatr = &(pastix_data->solvmatr);

    /* /\* Si on refait blend on doit jeter nos ancien coefs *\/ */
    /* if (pastix_data->malcof) */
    /* { */
    /*     CoefMatrix_Free( &(pastix_data->sopar), solvmatr, iparm[IPARM_FACTORIZATION]); */
    /*     pastix_data->malcof=0; */
    /* } */

    print_debug(DBG_STEP,"->API_TASK_ANALYSE\n");
    pastix_print( procnum, 0, "%s", OUT_STEP_BLEND );

    dofInit(&dofstr);
    dofConstant(&dofstr, 0, pastix_data->symbmtx->nodenbr,
                ((iparm[IPARM_DOF_COST] == 0)?iparm[IPARM_DOF_NBR]:iparm[IPARM_DOF_COST]));

    blendParamInit(&blendpar);
    blendpar.n         = pastix_data->n2;
    blendpar.smpnbr    = iparm[IPARM_NB_SMP_NODE_USED];
    blendpar.ricar     = iparm[IPARM_INCOMPLETE];
    blendpar.abs       = iparm[IPARM_ABS];
    blendpar.blcolmin  = iparm[IPARM_MIN_BLOCKSIZE];
    blendpar.blcolmax  = iparm[IPARM_MAX_BLOCKSIZE];
    blendpar.blblokmin = iparm[IPARM_MIN_BLOCKSIZE];
    blendpar.blblokmax = iparm[IPARM_MAX_BLOCKSIZE];

    if(blendpar.blcolmin > blendpar.blcolmax)
    {
        errorPrint("Parameter error : blocksize max < blocksize min (cf. iparm.txt).");
        ASSERT(blendpar.blcolmin <=  blendpar.blcolmax, MOD_SOPALIN);
    }

    blendpar.level2D    = iparm[IPARM_DISTRIBUTION_LEVEL];
    blendpar.ratiolimit = (double)(iparm[IPARM_DISTRIBUTION_LEVEL]);

    if (blendpar.autolevel)
        printf("ratiolimit=%lf\n",blendpar.ratiolimit);
    else
        printf("level2D=%ld\n", (long) blendpar.level2D);

#ifdef TRACE_SOPALIN
    if (procnum == 0)
        blendpar.tracegen = 1;
#endif

    blendpar.iparm = iparm;
    blendpar.dparm = dparm;

#ifdef FORCE_NOSMP
    iparm[IPARM_THREAD_NBR] = 1;
#endif

    solverBlend(solvmatr, pastix_data->symbmtx,
                procnbr, iparm[IPARM_THREAD_NBR], iparm[IPARM_CUDA_NBR],
                procnum, &blendpar, &dofstr );
    symbolExit(pastix_data->symbmtx);
    memFree_null(pastix_data->symbmtx);
    pastix_data->malslv = 1;

    if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
    {
        iparm[IPARM_NNZEROS]       *=2;
        dparm[DPARM_PRED_FACT_TIME]*=2.;
    }
    dparm[DPARM_SOLV_FLOPS] = (double)iparm[IPARM_NNZEROS]; /* number of operations for solve */

    iparm[IPARM_NNZEROS_BLOCK_LOCAL] = solvmatr->coefnbr;

    /* Affichage */
    dparm[DPARM_FILL_IN]       = dparm[DPARM_FILL_IN]      *(double)(iparm[IPARM_NNZEROS]/(iparm[IPARM_DOF_NBR]*iparm[IPARM_DOF_NBR]));
    if ((procnum==0) && (iparm[IPARM_VERBOSE] > API_VERBOSE_NOT))
    {
        fprintf(stdout,TIME_TO_ANALYSE,    (double)dparm[DPARM_ANALYZE_TIME]);
        fprintf(stdout, NNZERO_WITH_FILLIN_TH, (long)iparm[IPARM_NNZEROS]);
        fprintf(stdout, OUT_FILLIN_TH,         (double)dparm[DPARM_FILL_IN]);
        if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
            fprintf(stdout,NUMBER_OP_LU,     (double)dparm[DPARM_FACT_FLOPS]);
        else
            fprintf(stdout, NUMBER_OP_LLT,    (double)dparm[DPARM_FACT_FLOPS]);
        fprintf(stdout,TIME_FACT_PRED,PERF_MODEL,     (double)dparm[DPARM_PRED_FACT_TIME]);

    }
    if ((iparm[IPARM_VERBOSE] > API_VERBOSE_NO))
    {
        pastix_int_t solversize = sizeofsolver(solvmatr, iparm);

        fprintf(stdout,SOLVMTX_WITHOUT_CO,  (int)procnum, (double)MEMORY_WRITE(solversize),
                MEMORY_UNIT_WRITE(solversize));
        fprintf(stdout, NNZERO_WITH_FILLIN, (int)procnum, (long)iparm[IPARM_NNZEROS_BLOCK_LOCAL]);
    }
    if (iparm[IPARM_VERBOSE] > API_VERBOSE_YES)
    {
        pastix_int_t sizeL = solvmatr->coefnbr;
        pastix_int_t sizeG = 0;

        MPI_Reduce(&sizeL, &sizeG, 1, PASTIX_MPI_INT, MPI_MAX, 0, pastix_comm);

        if (procnum == 0)
        {
            sizeG *= sizeof(pastix_float_t);
            if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
                sizeG *= 2;

            fprintf(stdout, OUT_COEFSIZE, (double)MEMORY_WRITE(sizeG),
                    MEMORY_UNIT_WRITE(sizeG));
        }
    }

    iparm[IPARM_START_TASK]++;
}
