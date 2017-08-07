/**
 *
 * @file codelet_zgemmsp.c
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
static struct starpu_perfmodel starpu_cblk_zgemmsp_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "cblk_zgemmsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void cl_cblk_zgemmsp_cpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t sideA;
    pastix_coefside_t sideB;
    pastix_trans_t    trans;
    SolverCblk       *cblk;
    SolverBlok       *blok;
    SolverCblk       *fcblk;
    sopalin_data_t   *sopalin_data;
    const pastix_complex64_t *A;
    const pastix_complex64_t *B;
    pastix_complex64_t *C;

    A = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[0]);
    B = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[1]);
    C = (pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[2]);

    starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &blok, &fcblk, &sopalin_data);

    /* Check layout due to NULL workspace for now */
    assert(  cblk->cblktype & CBLK_LAYOUT_2D );
    assert( fcblk->cblktype & CBLK_LAYOUT_2D );

    cpucblk_zgemmsp( sideA, sideB, trans,
                     cblk, blok, fcblk,
                     A, B, C, NULL,
                     &(sopalin_data->solvmtx->lowrank) );
}

#if defined(PASTIX_WITH_CUDA)
static void cl_cblk_zgemmsp_gpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t sideA;
    pastix_coefside_t sideB;
    pastix_trans_t    trans;
    SolverCblk       *cblk;
    SolverBlok       *blok;
    SolverCblk       *fcblk;
    sopalin_data_t   *sopalin_data;
    const cuDoubleComplex *A;
    const cuDoubleComplex *B;
    cuDoubleComplex *C;

    A = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[0]);
    B = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[1]);
    C = (cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[2]);

    starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &blok, &fcblk, &sopalin_data);

    gpucblk_zgemmsp( sideA, sideB, trans,
                     cblk, blok, fcblk,
                     A, B, C,
                     &(sopalin_data->solvmtx->lowrank),
                     starpu_cuda_get_local_stream() );
}
#endif /* defined(PASTIX_WITH_CUDA) */
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_GPU( cblk_zgemmsp, 3, STARPU_CUDA_ASYNC )

void
starpu_task_cblk_zgemmsp( pastix_coefside_t sideA,
                          pastix_coefside_t sideB,
                          pastix_trans_t    trans,
                          const SolverCblk *cblk,
                          const SolverBlok *blok,
                          SolverCblk       *fcblk,
                          sopalin_data_t   *sopalin_data,
                          int               prio )
{
    starpu_insert_task(
        pastix_codelet(&cl_cblk_zgemmsp),
        STARPU_VALUE, &sideA,             sizeof(pastix_coefside_t),
        STARPU_VALUE, &sideB,             sizeof(pastix_coefside_t),
        STARPU_VALUE, &trans,             sizeof(pastix_trans_t),
        STARPU_VALUE, &cblk,              sizeof(SolverCblk*),
        STARPU_VALUE, &blok,              sizeof(SolverBlok*),
        STARPU_VALUE, &fcblk,             sizeof(SolverCblk*),
        STARPU_R,      cblk->handler[sideA],
        STARPU_R,      cblk->handler[sideB],
        STARPU_RW,     fcblk->handler[sideA],
        STARPU_VALUE, &sopalin_data,     sizeof(sopalin_data_t*),
#if defined(PASTIX_STARPU_CODELETS_HAVE_NAME)
        STARPU_NAME, "cblk_zgemmsp",
#endif
        STARPU_PRIORITY, prio,
        0);
}

/**
 * Blok version
 */
static struct starpu_perfmodel starpu_blok_zgemmsp_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "blok_zgemmsp",
};

#if !defined(PASTIX_STARPU_SIMULATION)
static void cl_blok_zgemmsp_cpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t sideA;
    pastix_coefside_t sideB;
    pastix_trans_t    trans;
    const SolverCblk *cblk;
    SolverCblk       *fcblk;
    pastix_int_t      blok_mk, blok_nk, blok_mn;
    sopalin_data_t   *sopalin_data;
    const pastix_complex64_t *A;
    const pastix_complex64_t *B;
    pastix_complex64_t *C;

    A = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[0]);
    B = (const pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[1]);
    C = (pastix_complex64_t *)STARPU_MATRIX_GET_PTR(descr[2]);

    starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &fcblk,
                               &blok_mk, &blok_nk, &blok_mn, &sopalin_data);

    assert( cblk->cblktype  & CBLK_TASKS_2D );
    assert( fcblk->cblktype & CBLK_TASKS_2D );

    cpublok_zgemmsp( sideA, sideB, trans,
                     cblk, fcblk,
                     blok_mk, blok_nk, blok_mn,
                     A, B, C,
                     &(sopalin_data->solvmtx->lowrank) );
}

#if defined(PASTIX_WITH_CUDA)
static void cl_blok_zgemmsp_gpu(void *descr[], void *cl_arg)
{
    pastix_coefside_t sideA;
    pastix_coefside_t sideB;
    pastix_trans_t    trans;
    const SolverCblk *cblk;
    SolverCblk       *fcblk;
    pastix_int_t      blok_mk, blok_nk, blok_mn;
    sopalin_data_t   *sopalin_data;
    const cuDoubleComplex *A;
    const cuDoubleComplex *B;
    cuDoubleComplex *C;

    A = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[0]);
    B = (const cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[1]);
    C = (cuDoubleComplex *)STARPU_MATRIX_GET_PTR(descr[2]);

    starpu_codelet_unpack_args(cl_arg, &sideA, &sideB, &trans, &cblk, &fcblk,
                               &blok_mk, &blok_nk, &blok_mn, &sopalin_data);

    assert( cblk->cblktype  & CBLK_TASKS_2D );
    assert( fcblk->cblktype & CBLK_TASKS_2D );

    gpublok_zgemmsp( sideA, sideB, trans,
                     cblk, fcblk,
                     blok_mk, blok_nk, blok_mn,
                     A, B, C,
                     &(sopalin_data->solvmtx->lowrank),
                     starpu_cuda_get_local_stream() );
}
#endif /* defined(PASTIX_WITH_CUDA) */
#endif /* !defined(PASTIX_STARPU_SIMULATION) */

CODELETS_GPU( blok_zgemmsp, 3, STARPU_CUDA_ASYNC )

void
starpu_task_blok_zgemmsp( pastix_coefside_t sideA,
                          pastix_coefside_t sideB,
                          pastix_trans_t    trans,
                          const SolverCblk *cblk,
                          SolverCblk       *fcblk,
                          const SolverBlok *blokA,
                          const SolverBlok *blokB,
                          sopalin_data_t   *sopalin_data,
                          int               prio )
{
    SolverBlok *blokC = fcblk->fblokptr;

    pastix_int_t frownum;
    pastix_int_t lrownum;
    pastix_int_t blok_mn = 0, j = 0;
    pastix_int_t blok_mk = blokA - cblk->fblokptr;
    pastix_int_t blok_nk = blokB - cblk->fblokptr;

    assert( blok_nk <= blok_mk );

    do {
        frownum = blokC->frownum;
        lrownum = blokC->lrownum;
        blok_mn += j;
        j = 1;

        /* Increase lrownum as long as blocks are facing the same cblk */
        while( (blokC < fcblk[1].fblokptr-1) &&
               (blokC[0].fcblknm == blokC[1].fcblknm) &&
               (blokC[0].lcblknm == blokC[1].lcblknm) )
        {
            blokC++; j++;
            lrownum = blokC->lrownum;
        }
        blokC++;
    }
    while( !((blokA->frownum >= frownum) &&
             (blokA->lrownum <= lrownum)) );

    blokC = fcblk->fblokptr + blok_mn;

    assert( blokA->lcblknm == blokB->lcblknm );
    assert( blokB->fcblknm == blokC->lcblknm );
    assert( blokC->frownum <= blokA->frownum );
    assert( blokA[-1].fcblknm != blokA[0].fcblknm );
    assert( blokB[-1].fcblknm != blokB[0].fcblknm );
    assert( (blok_mn == 0) || (blokC[-1].fcblknm != blokC[0].fcblknm) );

    starpu_insert_task(
        pastix_codelet(&cl_blok_zgemmsp),
        STARPU_VALUE, &sideA,             sizeof(pastix_coefside_t),
        STARPU_VALUE, &sideB,             sizeof(pastix_coefside_t),
        STARPU_VALUE, &trans,             sizeof(pastix_trans_t),
        STARPU_VALUE, &cblk,              sizeof(SolverCblk*),
        STARPU_VALUE, &fcblk,             sizeof(SolverCblk*),
        STARPU_VALUE, &blok_mk,           sizeof(pastix_int_t),
        STARPU_VALUE, &blok_nk,           sizeof(pastix_int_t),
        STARPU_VALUE, &blok_mn,           sizeof(pastix_int_t),
        STARPU_R,      blokA->handler[sideA],
        STARPU_R,      blokB->handler[sideB],
        STARPU_RW,     blokC->handler[sideA],
        STARPU_VALUE, &sopalin_data,     sizeof(sopalin_data_t*),
#if defined(PASTIX_STARPU_CODELETS_HAVE_NAME)
        STARPU_NAME, "blok_zgemmsp",
#endif
        STARPU_PRIORITY, prio,
        0);
}

/**
 * @}
 */
