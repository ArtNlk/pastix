/**
 *
 * @file codelet_ztrsmsp.c
 *
 * StarPU codelets for blas-like functions
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
#include "common.h"
#include "blend/solver.h"
#include "sopalin/sopalin_data.h"
#include "pastix_zcores.h"
#if defined(PASTIX_WITH_CUDA)
#include "pastix_zcuda.h"
#endif
#include "pastix_starpu.h"
#include "pastix_zstarpu.h"
#include "codelets.h"
#include "pastix_starpu_model.h"

/**
 * Block version
 */
#if defined( PASTIX_STARPU_PROFILING )
measure_t blok_ztrsmsp_perf[STARPU_NMAXWORKERS];
#endif

struct cl_blok_ztrsmsp_args_s {
    profile_data_t    profile_data;
    sopalin_data_t   *sopalin_data;
    pastix_coefside_t coef;
    pastix_side_t     side;
    pastix_uplo_t     uplo;
    pastix_trans_t    trans;
    pastix_diag_t     diag;
    const SolverCblk *cblk;
    pastix_int_t      blok_m;
};

static struct starpu_perfmodel starpu_blok_ztrsmsp_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "blok_ztrsmsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void fct_blok_ztrsmsp_cpu(void *descr[], void *cl_arg)
{
    const pastix_complex64_t     *A;
    pastix_complex64_t           *C;
    struct cl_blok_ztrsmsp_args_s *args = (struct cl_blok_ztrsmsp_args_s *) cl_arg;

    A = (const pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[0]);
    C = (pastix_complex64_t *)STARPU_VECTOR_GET_PTR(descr[1]);

    assert( args->cblk->cblktype & CBLK_TASKS_2D );

    args->profile_data.flops = cpublok_ztrsmsp( args->coef, args->side, args->uplo,
                                                args->trans, args->diag,
                                                args->cblk, args->blok_m, A, C,
                                                &(args->sopalin_data->solvmtx->lowrank) );
}

#if defined(PASTIX_WITH_CUDA)
static void fct_blok_ztrsmsp_gpu(void *descr[], void *cl_arg)
{
    const cuDoubleComplex         *A;
    cuDoubleComplex               *C;
    struct cl_blok_ztrsmsp_args_s *args = (struct cl_blok_ztrsmsp_args_s *) cl_arg;

    A = (const cuDoubleComplex *)STARPU_VECTOR_GET_PTR(descr[0]);
    C = (cuDoubleComplex *)STARPU_VECTOR_GET_PTR(descr[1]);

    assert( args->cblk->cblktype & CBLK_TASKS_2D );

    args->profile_data.flops = gpublok_ztrsmsp( args->coef, args->side, args->uplo,
                                                args->trans, args->diag,
                                                args->cblk, args->blok_m, A, C,
                                                &(args->sopalin_data->solvmtx->lowrank),
                                                starpu_cuda_get_local_stream() );
}
#endif /* defined(PASTIX_WITH_CUDA) */
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_ANY( blok_ztrsmsp, 2, STARPU_CUDA_ASYNC );

void
starpu_task_blok_ztrsmsp( sopalin_data_t   *sopalin_data,
                          pastix_coefside_t coef,
                          pastix_side_t     side,
                          pastix_uplo_t     uplo,
                          pastix_trans_t    trans,
                          pastix_diag_t     diag,
                          const SolverCblk *cblk,
                          SolverBlok       *blok,
                          int               prio )
{
    struct cl_blok_ztrsmsp_args_s *cl_arg;
    long long                      execute_where;
    pastix_int_t                   blok_m  = blok - cblk->fblokptr;

    /*
     * Create the arguments array
     */
    cl_arg                        = malloc( sizeof(struct cl_blok_ztrsmsp_args_s) );
    cl_arg->sopalin_data          = sopalin_data;
#if defined(PASTIX_STARPU_PROFILING)
    cl_arg->profile_data.measures = blok_ztrsmsp_perf;
    cl_arg->profile_data.flops    = NAN;
#endif
    cl_arg->coef                  = coef;
    cl_arg->side                  = side;
    cl_arg->uplo                  = uplo;
    cl_arg->trans                 = trans;
    cl_arg->diag                  = diag;
    cl_arg->cblk                  = cblk;
    cl_arg->blok_m                = blok_m;

    execute_where = cl_blok_ztrsmsp_any.where;
#if defined(PASTIX_WITH_CUDA)
    if ( (cblk->cblktype & CBLK_COMPRESSED) ) {
        execute_where &= (~STARPU_CUDA);
    }
#endif

    starpu_insert_task(
        pastix_codelet(&cl_blok_ztrsmsp_any),
        STARPU_CL_ARGS,       cl_arg,                       sizeof( struct cl_blok_ztrsmsp_args_s ),
        STARPU_EXECUTE_WHERE, execute_where,
#if defined(PASTIX_STARPU_PROFILING)
        STARPU_CALLBACK_WITH_ARG_NFREE, cl_profiling_callback, cl_arg,
#endif
        STARPU_R,             cblk->fblokptr->handler[coef],
        STARPU_RW,            blok->handler[coef],
#if defined(PASTIX_STARPU_HETEROPRIO)
        STARPU_PRIORITY,      BucketTRSM2D,
#else
        STARPU_PRIORITY,      prio,
#endif
        0);
    (void) prio;
}

/**
 * @}
 */
