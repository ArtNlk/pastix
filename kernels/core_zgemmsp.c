/**
 *
 * @file core_zgemmsp.c
 *
 *  PaStiX kernel routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
#include "common.h"
#include <cblas.h>
#include "blend/solver.h"
#include "pastix_zcores.h"

static pastix_complex64_t mzone =  -1.;
static pastix_complex64_t zone  =  1.;
static pastix_complex64_t zzero =  0.;

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgemmsp_1d1d - Computes the updates associated to one off-diagonal block
 * between two cblk stored as 1D block columns.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          If uplo == PastixLower, the contribution of:
 *          (block .. (cblk[1].fblokptr-1)) -by- block is computed and added to
 *          C, otherwise the contribution:
 *          (block+1 .. (cblk[1].fblokptr-1)) -by- block is computed and added
 *          to C.
 *          The pointer to the data structure that describes the panel from
 *          which we compute the contributions. Next column blok must be
 *          accessible through cblk[1].
 *
 * @param[in] trans
 *          Specify the transposition used for the B matrix. It has to be either
 *          PastixTrans or PastixConjTrans.
 *
 * @param[in] cblk
 *          The cblk structure to which block belongs to. The A and B pointers
 *          must be the coeftab of this column block.
 *          Next column blok must be accessible through cblk[1].
 *
 * @param[in] blok
 *          The block from which we compute the contributions.
 *
 * @param[in] fcblk
 *          The pointer to the data structure that describes the panel on which
 *          we compute the contributions. The C pointer must be one of the
 *          oceftab from this fcblk. Next column blok must be accessible through
 *          fcblk[1].
 *
 * @param[in] A
 *          The pointer to the coeftab of the cblk.lcoeftab matrix storing the
 *          coefficients of the panel when the Lower part is computed,
 *          cblk.ucoeftab otherwise. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] B The pointer to the coeftab of the cblk.lcoeftab matrix storing
 *          the coefficients of the panel, if Symmetric/Hermitian cases or if
 *          upper part is computed; cblk.ucoeftab otherwise. Must be of size
 *          cblk.stride -by- cblk.width
 *
 * @param[in,out] C
 *          The pointer to the fcblk.lcoeftab if the lower part is computed,
 *          fcblk.ucoeftab otherwise.
 *
 * @param[in] work
 *          Temporary memory buffer.
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal
 *          block.
 *
 *******************************************************************************/
static inline void
core_zgemmsp_1d1d( int uplo, int trans,
                   const SolverCblk         *cblk,
                   const SolverBlok         *blok,
                         SolverCblk         *fcblk,
                   const pastix_complex64_t *A,
                   const pastix_complex64_t *B,
                         pastix_complex64_t *C,
                         pastix_complex64_t *work )
{
    const SolverBlok *iterblok;
    const SolverBlok *fblok;
    const SolverBlok *lblok;

    pastix_complex64_t *tmpC;
    pastix_complex64_t *wtmp;
    pastix_int_t stride, stridef, indblok;
    pastix_int_t M, N, K, m;
    int shift;

    /* Both cblk and fcblk are stored in 1D */
    assert(!(cblk->cblktype  & CBLK_SPLIT));
    assert(!(fcblk->cblktype & CBLK_SPLIT));

    shift = (uplo == PastixUpper) ? 1 : 0;

    stride  = cblk->stride;
    stridef = fcblk->stride;
    K = cblk_colnbr( cblk );

    /* First blok */
    indblok = blok->coefind;

    N = blok_rownbr( blok );
    M = stride - indblok - (shift * N);

    /* Matrix A = Aik */
    A = A + indblok + (shift * N);
    B = B + indblok;

    /*
     * Compute update A * B'
     */
    wtmp = work;
    cblas_zgemm( CblasColMajor, CblasNoTrans, trans,
                 M, N, K,
                 CBLAS_SADDR(zone),  A,    stride,
                                     B,    stride,
                 CBLAS_SADDR(zzero), wtmp, M );

    /*
     * Add contribution to C in fcblk
     */

    /* Get the first block of the distant panel */
    fblok = fcblk->fblokptr;

    /* Move the pointer to the top of the right column */
    C = C + (blok->frownum - fcblk->fcolnum) * stridef;

    lblok = cblk[1].fblokptr;

    /* for all following blocks in block column */
    for (iterblok=blok+shift; iterblok<lblok; iterblok++) {

        /* Find facing blok */
        while (!is_block_inside_fblock( iterblok, fblok ))
        {
            fblok++;
            assert( fblok < fcblk[1].fblokptr );
        }

        tmpC = C + fblok->coefind + iterblok->frownum - fblok->frownum;
        m = blok_rownbr( iterblok );

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasNoTrans, m, N,
                     -1.0, wtmp, M,
                      1.0, tmpC, stridef );
        pastix_cblk_unlock( fcblk );

        /* Displacement to next block */
        wtmp += m;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgemmsp_1d2d - Computes the updates associated to one off-diagonal block
 * between a cblk stored in 1D storage, and one stored in 2D.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          If uplo == PastixLower, the contribution of:
 *          (block .. (cblk[1].fblokptr-1)) -by- block is computed and added to
 *          C, otherwise the contribution:
 *          (block+1 .. (cblk[1].fblokptr-1)) -by- block is computed and added
 *          to C.
 *          The pointer to the data structure that describes the panel from
 *          which we compute the contributions. Next column blok must be
 *          accessible through cblk[1].
 *
 * @param[in] trans
 *          Specify the transposition used for the B matrix. It has to be either
 *          PastixTrans or PastixConjTrans.
 *
 * @param[in] cblk
 *          The cblk structure to which block belongs to. The A and B pointers
 *          must be the coeftab of this column block.
 *          Next column blok must be accessible through cblk[1].
 *
 * @param[in] blok
 *          The block from which we compute the contributions.
 *
 * @param[in] fcblk
 *          The pointer to the data structure that describes the panel on which
 *          we compute the contributions. The C pointer must be one of the
 *          oceftab from this fcblk. Next column blok must be accessible through
 *          fcblk[1].
 *
 * @param[in] A
 *          The pointer to the coeftab of the cblk.lcoeftab matrix storing the
 *          coefficients of the panel when the Lower part is computed,
 *          cblk.ucoeftab otherwise. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] B The pointer to the coeftab of the cblk.lcoeftab matrix storing
 *          the coefficients of the panel, if Symmetric/Hermitian cases or if
 *          upper part is computed; cblk.ucoeftab otherwise. Must be of size
 *          cblk.stride -by- cblk.width
 *
 * @param[in,out] C
 *          The pointer to the fcblk.lcoeftab if the lower part is computed,
 *          fcblk.ucoeftab otherwise.
 *
 * @param[in] work
 *          Temporary memory buffer.
 *
 * @param[in] tol
 *          Tolerance for low-rank compression kernels
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal
 *          block.
 *
 *******************************************************************************/
static inline void
core_zgemmsp_1d2d( int uplo, int trans,
                   const SolverCblk         *cblk,
                   const SolverBlok         *blok,
                         SolverCblk         *fcblk,
                   const pastix_complex64_t *A,
                   const pastix_complex64_t *B,
                         pastix_complex64_t *C )
{
    const SolverBlok *iterblok;
    const SolverBlok *fblok;
    const SolverBlok *lblok;
    const pastix_complex64_t *blokA;
    const pastix_complex64_t *blokB;
    pastix_complex64_t *blokC;

    pastix_int_t stride, stridef;
    pastix_int_t M, N, K;
    int shift;

    /* cblk is stored in 1D and fcblk in 2D */
    assert(!(cblk->cblktype & CBLK_SPLIT));
    assert( fcblk->cblktype & CBLK_SPLIT );

    shift = (uplo == PastixUpper) ? 1 : 0;
    stride  = cblk->stride;

    /* Get the B block and its dimensions */
    blokB = B + blok->coefind;

    stride  = cblk->stride;
    stridef = fcblk->stride;
    K = cblk_colnbr( cblk );
    N = blok_rownbr( blok );

    /**
     * Add contribution to C in fcblk:
     *    Get the first facing block of the distant panel, and the last block of
     *    the current cblk
     */
    fblok = fcblk->fblokptr;
    lblok = cblk[1].fblokptr;

    /* Move the pointer to the top of the right column */
    C = C + (blok->frownum - fcblk->fcolnum) * stridef;

    for (iterblok=blok+shift; iterblok<lblok; iterblok++) {

        /* Find facing blok */
        while (!is_block_inside_fblock( iterblok, fblok ))
        {
            fblok++;
            assert( fblok < fcblk[1].fblokptr );
        }

        /* Get the A block and its dimensions */
        blokA = A + iterblok->coefind;
        M = blok_rownbr( iterblok );

        blokC = C + fblok->coefind + iterblok->frownum - fblok->frownum;

        pastix_cblk_lock( fcblk );
        cblas_zgemm( CblasColMajor, CblasNoTrans, trans,
                     M, N, K,
                     CBLAS_SADDR(mzone), blokA, stride,
                                         blokB, stride,
                     CBLAS_SADDR(zone),  blokC, stridef );
        pastix_cblk_unlock( fcblk );
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgemmsp_2d2d - Computes the updates associated to one off-diagonal block
 * between two cblk stored in 2D.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          If uplo == PastixLower, the contribution of:
 *          (block .. (cblk[1].fblokptr-1)) -by- block is computed and added to
 *          C, otherwise the contribution:
 *          (block+1 .. (cblk[1].fblokptr-1)) -by- block is computed and added
 *          to C.
 *          The pointer to the data structure that describes the panel from
 *          which we compute the contributions. Next column blok must be
 *          accessible through cblk[1].
 *
 * @param[in] trans
 *          Specify the transposition used for the B matrix. It has to be either
 *          PastixTrans or PastixConjTrans.
 *
 * @param[in] cblk
 *          The cblk structure to which block belongs to. The A and B pointers
 *          must be the coeftab of this column block.
 *          Next column blok must be accessible through cblk[1].
 *
 * @param[in] blok
 *          The block from which we compute the contributions.
 *
 * @param[in] fcblk
 *          The pointer to the data structure that describes the panel on which
 *          we compute the contributions. The C pointer must be one of the
 *          oceftab from this fcblk. Next column blok must be accessible through
 *          fcblk[1].
 *
 * @param[in] A
 *          The pointer to the coeftab of the cblk.lcoeftab matrix storing the
 *          coefficients of the panel when the Lower part is computed,
 *          cblk.ucoeftab otherwise. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] B The pointer to the coeftab of the cblk.lcoeftab matrix storing
 *          the coefficients of the panel, if Symmetric/Hermitian cases or if
 *          upper part is computed; cblk.ucoeftab otherwise. Must be of size
 *          cblk.stride -by- cblk.width
 *
 * @param[in,out] C
 *          The pointer to the fcblk.lcoeftab if the lower part is computed,
 *          fcblk.ucoeftab otherwise.
 *
 * @param[in] work
 *          Temporary memory buffer.
 *
 * @param[in] tol
 *          Tolerance for low-rank compression kernels
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal
 *          block.
 *
 *******************************************************************************/
static inline void
core_zgemmsp_2d2d( int uplo, int trans,
                   const SolverCblk         *cblk,
                   const SolverBlok         *blok,
                         SolverCblk         *fcblk,
                   const pastix_complex64_t *A,
                   const pastix_complex64_t *B,
                         pastix_complex64_t *C )
{
    const SolverBlok *iterblok;
    const SolverBlok *fblok;
    const SolverBlok *lblok;
    const pastix_complex64_t *blokA;
    const pastix_complex64_t *blokB;
    pastix_complex64_t *blokC;

    pastix_int_t stride, stridef;
    pastix_int_t M, N, K;
    int shift;

    /* cblk is stored in 1D and fcblk in 2D */
    assert(!(cblk->cblktype & CBLK_SPLIT));
    assert( fcblk->cblktype & CBLK_SPLIT );

    shift = (uplo == PastixUpper) ? 1 : 0;
    stride  = cblk->stride;

    /* Get the B block and its dimensions */
    blokB = B + blok->coefind;

    stride  = cblk->stride;
    stridef = fcblk->stride;
    K = cblk_colnbr( cblk );
    N = blok_rownbr( blok );

    /**
     * Add contribution to C in fcblk:
     *    Get the first facing block of the distant panel, and the last block of
     *    the current cblk
     */
    fblok = fcblk->fblokptr;
    lblok = cblk[1].fblokptr;

    /* Move the pointer to the top of the right column */
    C = C + (blok->frownum - fcblk->fcolnum) * stridef;

    for (iterblok=blok+shift; iterblok<lblok; iterblok++) {

        /* Find facing blok */
        while (!is_block_inside_fblock( iterblok, fblok ))
        {
            fblok++;
            assert( fblok < fcblk[1].fblokptr );
        }

        /* Get the A block and its dimensions */
        blokA = A + iterblok->coefind;
        M = blok_rownbr( iterblok );

        blokC = C + fblok->coefind + iterblok->frownum - fblok->frownum;

        pastix_cblk_lock( fcblk );
        cblas_zgemm( CblasColMajor, CblasNoTrans, trans,
                     M, N, K,
                     CBLAS_SADDR(mzone), blokA, stride,
                                         blokB, stride,
                     CBLAS_SADDR(zone),  blokC, stridef );
        pastix_cblk_unlock( fcblk );
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgemmsp - Computes the updates associated to one off-diagonal block.
 *
 *******************************************************************************
 *
 * @param[in] uplo
 *          If uplo == PastixLower, the contribution of:
 *          (block .. (cblk[1].fblokptr-1)) -by- block is computed and added to
 *          C, otherwise the contribution:
 *          (block+1 .. (cblk[1].fblokptr-1)) -by- block is computed and added
 *          to C.
 *          The pointer to the data structure that describes the panel from
 *          which we compute the contributions. Next column blok must be
 *          accessible through cblk[1].
 *
 * @param[in] trans
 *          Specify the transposition used for the B matrix. It has to be either
 *          PastixTrans or PastixConjTrans.
 *
 * @param[in] cblk
 *          The cblk structure to which block belongs to. The A and B pointers
 *          must be the coeftab of this column block.
 *          Next column blok must be accessible through cblk[1].
 *
 * @param[in] blok
 *          The block from which we compute the contributions.
 *
 * @param[in] fcblk
 *          The pointer to the data structure that describes the panel on which
 *          we compute the contributions. The C pointer must be one of the
 *          oceftab from this fcblk. Next column blok must be accessible through
 *          fcblk[1].
 *
 * @param[in] A
 *          The pointer to the coeftab of the cblk.lcoeftab matrix storing the
 *          coefficients of the panel when the Lower part is computed,
 *          cblk.ucoeftab otherwise. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] B The pointer to the coeftab of the cblk.lcoeftab matrix storing
 *          the coefficients of the panel, if Symmetric/Hermitian cases or if
 *          upper part is computed; cblk.ucoeftab otherwise. Must be of size
 *          cblk.stride -by- cblk.width
 *
 * @param[in,out] C
 *          The pointer to the fcblk.lcoeftab if the lower part is computed,
 *          fcblk.ucoeftab otherwise.
 *
 * @param[in] work
 *          Temporary memory buffer.
 *
 * @param[in] tol
 *          Tolerance for low-rank compression kernels
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal
 *          block.
 *
 *******************************************************************************/
void core_zgemmsp( int uplo, int trans,
                   const SolverCblk         *cblk,
                   const SolverBlok         *blok,
                         SolverCblk         *fcblk,
                   const pastix_complex64_t *A,
                   const pastix_complex64_t *B,
                   pastix_complex64_t *C,
                   pastix_complex64_t *work )
{
    if ( cblk->cblktype & CBLK_SPLIT ) {
        if ( fcblk->cblktype & CBLK_SPLIT ) {
            core_zgemmsp_2d2d( uplo, trans,
                               cblk, blok, fcblk,
                               A, B, C );
        }
        else {
            core_zgemmsp_1d2d( uplo, trans,
                               cblk, blok, fcblk,
                               A, B, C );
        }
    }
    else {
        core_zgemmsp_1d1d( uplo, trans,
                           cblk, blok, fcblk,
                           A, B, C, work );
    }
}

