/**
 *
 * @file codelet_ztrsmsp.c
 *
 * StarPU codelets for blas-like functions
 *
 * @copyright 2016-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2013-06-24
 *
 * @precisions normal z -> z c d s
 *
 * @addtogroup pastix_starpu
 * @{
 *
 **/
#include "common.h"
#include "solver.h"
#include "sopalin_data.h"
#include "pastix_zcores.h"
#include "sopalin/starpu/codelets.h"

/**
 * Cblk version
 */
/* static struct starpu_perfmodel starpu_cblk_ztrsmsp_model = */
/* { */
/* 	.type = STARPU_HISTORY_BASED, */
/* 	.symbol = "cblk_ztrsmsp", */
/* }; */

/* #if !defined(PASTIX_STARPU_SIMULATION) */
/* static void cl_cblk_ztrsmsp_cpu(void *descr[], void *cl_arg) */
/* { */
/*     pastix_coefside_t sideA; */
/*     pastix_coefside_t sideB; */
/*     pastix_trans_t    trans; */
/*     SolverCblk       *cblk; */
/*     SolverBlok       *blok; */
/*     SolverCblk       *fcblk; */
/*     sopalin_data_t   *sopalin_data; */
/*     const pastix_complex64_t *A; */
/*     const pastix_complex64_t *B; */
/*     pastix_complex64_t *C; */

/*     A = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[0]); */
/*     B = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[1]); */
/*     C = (pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[2]); */

/*     /\* Check layout due to workspace *\/ */
/*     starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &blok, &fcblk, &sopalin_data); */

/*     assert( cblk->cblktype  & CBLK_LAYOUT_2D ); */
/*     assert( fcblk->cblktype & CBLK_LAYOUT_2D ); */

/*     cpucblk_ztrsmsp( sideA, sideB, trans, */
/*                      cblk, blok, fcblk, */
/*                      A, B, C, NULL, */
/*                      &(sopalin_data->solvmtx->lowrank) ); */
/* } */

/* #if defined(PASTIX_WITH_CUDA) */
/* static void cl_cblk_ztrsmsp_gpu(void *descr[], void *cl_arg) */
/* { */
/*     pastix_coefside_t sideA; */
/*     pastix_coefside_t sideB; */
/*     pastix_trans_t    trans; */
/*     SolverCblk       *cblk; */
/*     SolverBlok       *blok; */
/*     SolverCblk       *fcblk; */
/*     sopalin_data_t   *sopalin_data; */
/*     const cuDoubleComplex *A; */
/*     const cuDoubleComplex *B; */
/*     cuDoubleComplex *C; */

/*     A = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[0]); */
/*     B = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[1]); */
/*     C = (cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[2]); */

/*     /\* Check layout due to workspace *\/ */
/*     assert( cblk->cblktype & CBLK_LAYOUT_2D ); */
/*     assert( fcblk->cblktype & CBLK_LAYOUT_2D ); */

/*     starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &blok, &fcblk, &sopalin_data); */

/*     gpucblk_ztrsmsp( sideA, sideB, trans, */
/*                      cblk, blok, fcblk, */
/*                      A, B, C, */
/*                      &(sopalin_data->solvmtx->lowrank), */
/*                      starpu_cuda_get_local_stream() ); */
/* } */
/* #endif /\* defined(PASTIX_WITH_CUDA) *\/ */
/* #endif /\* !defined(PASTIX_STARPU_SIMULATION) *\/ */

/* CODELETS_GPU( cblk_ztrsmsp, 3, STARPU_CUDA_ASYNC ) */

/* void */
/* starpu_task_cblk_ztrsmsp( pastix_coefside_t sideA, */
/*                           pastix_coefside_t sideB, */
/*                           pastix_trans_t    trans, */
/*                           const SolverCblk *cblk, */
/*                           const SolverBlok *blok, */
/*                           SolverCblk       *fcblk, */
/*                           sopalin_data_t   *sopalin_data ) */
/* { */
/*     starpu_insert_task( */
/*         pastix_codelet(&cl_cblk_ztrsmsp), */
/*         STARPU_VALUE, &sideA,             sizeof(pastix_coefside_t), */
/*         STARPU_VALUE, &sideB,             sizeof(pastix_coefside_t), */
/*         STARPU_VALUE, &trans,             sizeof(pastix_trans_t), */
/*         STARPU_VALUE, &cblk,              sizeof(SolverCblk*), */
/*         STARPU_VALUE, &blok,              sizeof(SolverBlok*), */
/*         STARPU_VALUE, &fcblk,             sizeof(SolverCblk*), */
/*         STARPU_R,      cblk->handler[sideA], */
/*         STARPU_R,      cblk->handler[sideB], */
/*         STARPU_RW,     fcblk->handler[sideA], */
/*         STARPU_VALUE, &sopalin_data,     sizeof(sopalin_data_t*), */
/* #if defined(PASTIX_STARPU_CODELETS_HAVE_NAME) */
/*         STARPU_NAME, "cblk_ztrsmsp", */
/* #endif */
/*         0); */
/* } */


/**
 * Block version
 */
static struct starpu_perfmodel starpu_blok_ztrsmsp_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "blok_ztrsmsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void cl_blok_ztrsmsp_cpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t coef;
    pastix_side_t     side;
    pastix_uplo_t     uplo;
    pastix_trans_t    trans;
    pastix_diag_t     diag;
    SolverCblk       *cblk;
    pastix_int_t      blok_m;
    sopalin_data_t   *sopalin_data;

    const pastix_complex64_t *A;
    pastix_complex64_t *C;

    A = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[0]);
    C = (pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[1]);

    starpu_codelet_unpack_args(cl_arg, &coef, &side, &uplo, &trans, &diag,
                               &cblk, &blok_m, &sopalin_data);

    assert( cblk->cblktype & CBLK_TASKS_2D );

    cpublok_ztrsmsp( coef, side, uplo, trans, diag,
                     cblk, blok_m, A, C,
                     &(sopalin_data->solvmtx->lowrank) );
}

#if defined(PASTIX_WITH_CUDA)
static void cl_blok_ztrsmsp_gpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t coef;
    pastix_side_t     side;
    pastix_uplo_t     uplo;
    pastix_trans_t    trans;
    pastix_diag_t     diag;
    SolverCblk       *cblk;
    pastix_int_t      blok_m;
    sopalin_data_t   *sopalin_data;

    const cuDoubleComplex *A;
    cuDoubleComplex *C;

    A = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[0]);
    C = (cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[1]);

    starpu_codelet_unpack_args(cl_arg, &coef, &side, &uplo, &trans, &diag,
                               &cblk, &blok_m, &sopalin_data);

    assert( cblk->cblktype & CBLK_TASKS_2D );

    gpublok_ztrsmsp( coef, side, uplo, trans, diag,
                     cblk, blok_m, A, C,
                     &(sopalin_data->solvmtx->lowrank),
                     starpu_cuda_get_local_stream() );
}
#endif /* defined(PASTIX_WITH_CUDA) */
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_GPU( blok_ztrsmsp, 2, STARPU_CUDA_ASYNC )

void
starpu_task_blok_ztrsmsp( pastix_coefside_t coef,
                          pastix_side_t     side,
                          pastix_uplo_t     uplo,
                          pastix_trans_t    trans,
                          pastix_diag_t     diag,
                          SolverCblk       *cblk,
                          SolverBlok       *blok,
                          sopalin_data_t   *sopalin_data,
                          int               prio )
{
    pastix_int_t blok_m = blok - cblk->fblokptr;

    starpu_insert_task(
        pastix_codelet(&cl_blok_ztrsmsp),
        STARPU_VALUE, &coef,             sizeof(pastix_coefside_t),
        STARPU_VALUE, &side,             sizeof(pastix_side_t),
        STARPU_VALUE, &uplo,             sizeof(pastix_uplo_t),
        STARPU_VALUE, &trans,            sizeof(pastix_trans_t),
        STARPU_VALUE, &diag,             sizeof(pastix_diag_t),
        STARPU_VALUE, &cblk,             sizeof(SolverCblk*),
        STARPU_VALUE, &blok_m,           sizeof(pastix_int_t),
        STARPU_R,      cblk->fblokptr->handler[coef],
        STARPU_RW,     blok->handler[coef],
        STARPU_VALUE, &sopalin_data,     sizeof(sopalin_data_t*),
#if defined(PASTIX_STARPU_CODELETS_HAVE_NAME)
        STARPU_NAME, "blok_ztrsmsp",
#endif
        STARPU_PRIORITY, prio,
        0);
}

/**
 * @}
 */
