/**
 *
 * @file codelet_zpotrfsp.c
 *
 * StarPU codelets for Cholesky functions
 *
 * @copyright 2016-2021 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Ian Masliah
 * @date 2021-06-21
 *
 * @precisions normal z -> z c d s
 *
 * @addtogroup pastix_starpu
 * @{
 *
 **/
#define _GNU_SOURCE
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
#if defined( PASTIX_STARPU_PROFILING )
measure_t cblk_zpotrfsp_perf[STARPU_NMAXWORKERS];
#endif

struct cl_cblk_zpotrfsp_args_s {
    profile_data_t  profile_data;
    SolverMatrix   *solvmtx;
    SolverCblk     *cblk;
};

static struct starpu_perfmodel starpu_cblk_zpotrfsp1d_panel_model =
{
#if defined(PASTIX_STARPU_COST_PER_ARCH)       
    .type = STARPU_PER_ARCH,
    .arch_cost_function = cblk_potrf_cost,
#else
    .type = STARPU_HISTORY_BASED,
#endif
    .symbol = "cblk_zpotrfsp1d_panel",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void fct_cblk_zpotrfsp1d_panel_cpu(void *descr[], void *cl_arg)
{
    SolverMatrix                   *solvmtx;
    pastix_complex64_t             *L;
    int                             nbpivot;
    struct cl_cblk_zpotrfsp_args_s *args = (struct cl_cblk_zpotrfsp_args_s *)cl_arg;

    L = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);

    solvmtx = args->solvmtx;
    nbpivot = cpucblk_zpotrfsp1d_panel( solvmtx, args->cblk, L );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( cblk_zpotrfsp1d_panel, 1 );

void
starpu_task_cblk_zpotrfsp1d_panel( sopalin_data_t *sopalin_data,
                                   SolverCblk     *cblk,
                                   int             prio )
{
    struct cl_cblk_zpotrfsp_args_s *cl_arg;
#if defined(PASTIX_DEBUG_STARPU)
    char                           *task_name;
    asprintf( &task_name, "%s( %ld )", cl_cblk_zpotrfsp1d_panel_cpu.name, (long)(cblk - sopalin_data->solvmtx->cblktab) );
#endif

    /*
     * Create the arguments array
     */
    cl_arg                        = malloc( sizeof(struct cl_cblk_zpotrfsp_args_s) );
    cl_arg->solvmtx               = sopalin_data->solvmtx;
#if defined(PASTIX_STARPU_PROFILING)
    cl_arg->profile_data.measures = cblk_zpotrfsp_perf;
    cl_arg->profile_data.flops    = NAN;
#endif
    cl_arg->cblk                  = cblk;

    starpu_insert_task(
        pastix_codelet(&cl_cblk_zpotrfsp1d_panel_cpu),
        STARPU_CL_ARGS,                 cl_arg,                sizeof( struct cl_cblk_zpotrfsp_args_s ),
#if defined(PASTIX_STARPU_PROFILING)
        STARPU_CALLBACK_WITH_ARG_NFREE, cl_profiling_callback, cl_arg,
#endif
        STARPU_RW,                      cblk->handler[0],
#if defined(PASTIX_DEBUG_STARPU)
        STARPU_NAME,                    task_name,
#endif
#if defined(PASTIX_STARPU_HETEROPRIO)
        STARPU_PRIORITY,                BucketFacto1D,
#else
        STARPU_PRIORITY,                prio,
#endif
        0);
    (void) prio;
}

/**
 * Blok version
 */
#if defined( PASTIX_STARPU_PROFILING )
measure_t blok_zpotrfsp_perf[STARPU_NMAXWORKERS];
#endif

struct cl_blok_zpotrfsp_args_s {
    profile_data_t  profile_data;
    SolverMatrix   *solvmtx;
    SolverCblk     *cblk;
};

static struct starpu_perfmodel starpu_blok_zpotrfsp_model =
{
#if defined(PASTIX_STARPU_COST_PER_ARCH)
    .type = STARPU_PER_ARCH,
    .arch_cost_function = blok_potrf_cost,
#else
    .type = STARPU_HISTORY_BASED,
#endif
    .symbol = "blok_zpotrfsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void fct_blok_zpotrfsp_cpu(void *descr[], void *cl_arg)
{
    SolverMatrix                   *solvmtx;
    pastix_complex64_t             *L;
    int                             nbpivot;
    struct cl_blok_zpotrfsp_args_s *args = (struct cl_blok_zpotrfsp_args_s *)cl_arg;

    L = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);

    assert(args->cblk->cblktype & CBLK_TASKS_2D);

    solvmtx = args->solvmtx;
    nbpivot = cpucblk_zpotrfsp1d_potrf( solvmtx, args->cblk, L );

    (void)nbpivot;
}
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_CPU( blok_zpotrfsp, 1 );

void
starpu_task_blok_zpotrf( sopalin_data_t *sopalin_data,
                         SolverCblk     *cblk,
                         int             prio )
{
    struct cl_blok_zpotrfsp_args_s *cl_arg;
#if defined(PASTIX_DEBUG_STARPU)
    char                           *task_name;
    asprintf( &task_name, "%s( %ld )", cl_blok_zpotrfsp_cpu.name, (long)(cblk - sopalin_data->solvmtx->cblktab) );
#endif
    /*
     * Create the arguments array
     */
    cl_arg                        = malloc( sizeof(struct cl_blok_zpotrfsp_args_s) );
    cl_arg->solvmtx               = sopalin_data->solvmtx;
#if defined(PASTIX_STARPU_PROFILING)
    cl_arg->profile_data.measures = blok_zpotrfsp_perf;
    cl_arg->profile_data.flops    = NAN;
#endif
    cl_arg->cblk                  = cblk;

    starpu_insert_task(
        pastix_codelet(&cl_blok_zpotrfsp_cpu),        
        STARPU_CL_ARGS,                 cl_arg,                sizeof( struct cl_blok_zpotrfsp_args_s ),
#if defined(PASTIX_STARPU_PROFILING)
        STARPU_CALLBACK_WITH_ARG_NFREE, cl_profiling_callback, cl_arg,
#endif
        STARPU_RW,                      cblk->fblokptr->handler[0],
#if defined(PASTIX_DEBUG_STARPU)
        STARPU_NAME,                    task_name,
#endif
#if defined(PASTIX_STARPU_HETEROPRIO)
        STARPU_PRIORITY,                BucketFacto2D,
#else
        STARPU_PRIORITY,                prio,
#endif
        0);
    (void) prio;
}

/**
 * @}
 */
