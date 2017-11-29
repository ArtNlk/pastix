/**
 *
 * @file codelet_zhetrfsp.c
 *
 * StarPU codelets for LDL^h functions
 *
 * @copyright 2016-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2017-06-24
 *
 * @precisions normal z -> z c
 *
 * @addtogroup pastix_starpu
 * @{
 *
 **/
#include "common.h"
#include "solver.h"
#include "sopalin_data.h"
#include "pastix_zcores.h"
#include "pastix_starpu.h"
#include "codelets.h"

/**
 * Cblk version
 */
static struct starpu_perfmodel starpu_cblk_zhetrfsp1d_panel_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "cblk_zhetrfsp1d_panel",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void cl_cblk_zhetrfsp1d_panel_cpu(void *descr[], void *cl_arg)
{
    sopalin_data_t *sopalin_data;
    SolverCblk *cblk;
    pastix_complex64_t *L, *DL;
    int nbpivot;

    L  = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);
    DL = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[1]);

    starpu_codelet_unpack_args( cl_arg, &cblk, &sopalin_data );

    assert( !(cblk->cblktype & CBLK_TASKS_2D) );

    nbpivot = cpucblk_zhetrfsp1d_panel( cblk, L, DL, sopalin_data->diagthreshold,
                                        &(sopalin_data->solvmtx->lowrank) );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( cblk_zhetrfsp1d_panel, 2 )

void
starpu_task_cblk_zhetrfsp1d_panel( sopalin_data_t *sopalin_data,
                                   SolverCblk     *cblk,
                                   int             prio )
{
    starpu_data_handle_t *handler = (starpu_data_handle_t*)(cblk->handler);
    pastix_int_t N = cblk_colnbr( cblk );
    pastix_int_t M = cblk->stride;

    if ( M-N > 0 ) {
        starpu_vector_data_register( handler + 1, -1, (uintptr_t)NULL, M * N,
                                     sopalin_data->solvmtx->starpu_desc->typesze );
    }
    else {
        starpu_vector_data_register( handler + 1, -1, (uintptr_t)NULL, 0,
                                     sopalin_data->solvmtx->starpu_desc->typesze );
    }

    starpu_insert_task(
        pastix_codelet(&cl_cblk_zhetrfsp1d_panel),
        STARPU_VALUE, &cblk,         sizeof(SolverCblk*),
        STARPU_VALUE, &sopalin_data, sizeof(sopalin_data_t*),
        STARPU_RW,     cblk->handler[0],
        STARPU_W,      cblk->handler[1],
#if defined(PASTIX_STARPU_CODELETS_HAVE_NAME)
        STARPU_NAME, "cblk_zhetrfsp1d_panel",
#endif
        STARPU_PRIORITY, prio,
        0);
}

/**
 * Blok version
 */
static struct starpu_perfmodel starpu_blok_zhetrfsp_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "blok_zhetrfsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void cl_blok_zhetrfsp_cpu(void *descr[], void *cl_arg)
{
    sopalin_data_t *sopalin_data;
    SolverCblk *cblk;
    pastix_complex64_t *L;
    int nbpivot;

    L = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);

    starpu_codelet_unpack_args( cl_arg, &cblk, &sopalin_data );

    assert(cblk->cblktype & CBLK_TASKS_2D);

    nbpivot = cpucblk_zhetrfsp1d_hetrf( cblk, L, sopalin_data->diagthreshold );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( blok_zhetrfsp, 1 )

void
starpu_task_blok_zhetrf( sopalin_data_t *sopalin_data,
                         SolverCblk     *cblk,
                         int             prio )
{
    starpu_insert_task(
        pastix_codelet(&cl_blok_zhetrfsp),
        STARPU_VALUE, &cblk,         sizeof(SolverCblk*),
        STARPU_VALUE, &sopalin_data, sizeof(sopalin_data_t*),
        STARPU_RW,     cblk->fblokptr->handler[0],
#if defined(PASTIX_STARPU_CODELETS_HAVE_NAME)
        STARPU_NAME, "blok_zhetrfsp",
#endif
        STARPU_PRIORITY, prio,
        0);
}

/**
 * @}
 */
