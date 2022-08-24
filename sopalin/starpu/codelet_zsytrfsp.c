/**
 *
 * @file codelet_zsytrfsp.c
 *
 * StarPU codelets for LDL^t functions
 *
 * @copyright 2016-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2021-06-21
 *
 * @precisions normal z -> z c d s
 *
 * @addtogroup pastix_starpu
 * @{
 *
 **/
#include "common.h"
#include "blend/solver.h"
#include "sopalin/sopalin_data.h"
#include "pastix_zcores.h"
#include "pastix_starpu.h"
#include "pastix_zstarpu.h"
#include "codelets.h"
#include "pastix_starpu_model.h"

/**
 * Cblk version
 */
static struct starpu_perfmodel starpu_cblk_zsytrfsp1d_panel_model =
{
    .type = STARPU_PER_ARCH,
    .symbol = "cblk_zsytrf",
    .arch_cost_function = cblk_sytrf_cost,
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void fct_cblk_zsytrfsp1d_panel_cpu(void *descr[], void *cl_arg)
{
    sopalin_data_t *sopalin_data;
    SolverMatrix   *solvmtx;
    SolverCblk     *cblk;
    pastix_complex64_t *L;
    pastix_complex64_t *DL;
    int nbpivot;

    L  = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);
    DL = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[1]);

    starpu_codelet_unpack_args( cl_arg, &sopalin_data, &cblk );

    solvmtx = sopalin_data->solvmtx;
    nbpivot = cpucblk_zsytrfsp1d_panel( solvmtx, cblk, L, DL );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( cblk_zsytrfsp1d_panel, 2 );

void
starpu_task_cblk_zsytrfsp1d_panel( sopalin_data_t *sopalin_data,
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

#if defined(PASTIX_WITH_MPI)
    {
        int64_t tag_desc = sopalin_data->solvmtx->starpu_desc->mpitag;
        int64_t tag_cblk = 2 * cblk->gcblknum + 1;

        starpu_mpi_data_register( *(handler+1),
                                  tag_desc | tag_cblk,
                                  cblk->ownerid );
    }
#endif /* PASTIX_WITH_MPI */

    starpu_insert_task(
        pastix_codelet(&cl_cblk_zsytrfsp1d_panel_cpu),
        STARPU_VALUE,   &sopalin_data, sizeof(sopalin_data_t*),
        STARPU_VALUE,   &cblk,         sizeof(SolverCblk*),
        STARPU_RW,       cblk->handler[0],
        STARPU_W,        cblk->handler[1],
#if defined(PASTIX_STARPU_HETEROPRIO)
        STARPU_PRIORITY, 0,
#else
        STARPU_PRIORITY, prio,
#endif
        0);
}

/**
 * Blok version
 */
static struct starpu_perfmodel starpu_blok_zsytrfsp_model =
{
    .type = STARPU_PER_ARCH,
    .symbol = "blok_zsytrfsp",
    .arch_cost_function = blok_sytrf_cost,
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void fct_blok_zsytrfsp_cpu(void *descr[], void *cl_arg)
{
    sopalin_data_t *sopalin_data;
    SolverMatrix   *solvmtx;
    SolverCblk     *cblk;
    pastix_complex64_t *L;
    int nbpivot;

    L = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);

    starpu_codelet_unpack_args( cl_arg, &sopalin_data, &cblk );

    assert(cblk->cblktype & CBLK_TASKS_2D);

    solvmtx = sopalin_data->solvmtx;
    nbpivot = cpucblk_zsytrfsp1d_sytrf( solvmtx, cblk, L );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( blok_zsytrfsp, 1 );

void
starpu_task_blok_zsytrf( sopalin_data_t *sopalin_data,
                         SolverCblk     *cblk,
                         int             prio )
{
    starpu_insert_task(
        pastix_codelet(&cl_blok_zsytrfsp_cpu),
        STARPU_VALUE,   &sopalin_data, sizeof(sopalin_data_t*),
        STARPU_VALUE,   &cblk,         sizeof(SolverCblk*),
        STARPU_RW,       cblk->fblokptr->handler[0],
#if defined(PASTIX_STARPU_HETEROPRIO)
        STARPU_PRIORITY, 0,
#else
        STARPU_PRIORITY, prio,
#endif
        0);
}

/**
 * @}
 */
