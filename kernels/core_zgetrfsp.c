/**
 *
 * @file core_zgetrfsp.c
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

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t mzone = -1.;

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetf2sp - Computes the sequential static pivoting LU factorization of
 * the matrix m-by-n A = L * U.
 *
 *******************************************************************************
 *
 * @param[in] m
 *          The number of rows and columns of the matrix A.
 *
 * @param[in] n
 *          The number of rows and columns of the matrix A.
 *
 * @param[in,out] A
 *          The matrix A to factorize with LU factorization. The matrix
 *          is of size lda -by- n.
 *
 * @param[in] lda
 *          The leading dimension of the matrix A.
 *
 * @param[in,out] nbpivot
 *          Pointer to the number of piovting operations made during
 *          factorization. It is updated during this call
 *
 * @param[in] criteria
 *          Threshold use for static pivoting. If diagonal value is under this
 *          threshold, its value is replaced by the threshold and the number of
 *          pivots is incremented.
 *
 *******************************************************************************
 *
 * @return
 *          This routine will fail if it discovers a 0. on the diagonal during
 *          factorization.
 *
 *******************************************************************************/
static void core_zgetf2sp(pastix_int_t        m,
                          pastix_int_t        n,
                          pastix_complex64_t *A,
                          pastix_int_t        lda,
                          pastix_int_t       *nbpivot,
                          double              criteria )
{
    pastix_int_t k, minMN;
    pastix_complex64_t *Akk, *Aik, alpha;

    minMN = pastix_imin( m, n );

    Akk = A;
    for (k=0; k<minMN; k++) {
        Aik = Akk + 1;

        if ( cabs(*Akk) < criteria ) {
            (*Akk) = (pastix_complex64_t)criteria;
            (*nbpivot)++;
        }

        /* A_ik = A_ik / A_kk, i = k+1 .. n */
        alpha = 1. / (*Akk);
        cblas_zscal(m-k-1, CBLAS_SADDR( alpha ), Aik, 1 );

        if ( k+1 < minMN ) {

            /* A_ij = A_ij - A_ik * A_kj, i,j = k+1..n */
            cblas_zgeru(CblasColMajor, m-k-1, n-k-1,
                        CBLAS_SADDR(mzone),
                        Aik,        1,
                        Akk+lda, lda,
                        Aik+lda, lda);
        }

        Akk += lda+1;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp - Computes the block static pivoting LU factorization of
 * the matrix m-by-n A = L * U.
 *
 *******************************************************************************
 *
 * @param[in] m
 *          The number of rows and columns of the matrix A.
 *
 * @param[in] n
 *          The number of rows and columns of the matrix A.
 *
 * @param[in,out] A
 *          The matrix A to factorize with LU factorization. The matrix
 *          is of size lda -by- n.
 *
 * @param[in] lda
 *          The leading dimension of the matrix A.
 *
 * @param[in,out] nbpivot
 *          Pointer to the number of piovting operations made during
 *          factorization. It is updated during this call
 *
 * @param[in] criteria
 *          Threshold use for static pivoting. If diagonal value is under this
 *          threshold, its value is replaced by the threshold and the nu,ber of
 *          pivots is incremented.
 *
 *******************************************************************************
 *
 * @return
 *          This routine will fail if it discovers a 0. on the diagonal during
 *          factorization.
 *
 *******************************************************************************/
#define MAXSIZEOFBLOCKS 64

static void core_zgetrfsp(pastix_int_t        n,
                          pastix_complex64_t *A,
                          pastix_int_t        lda,
                          pastix_int_t       *nbpivot,
                          double              criteria)
{
    pastix_int_t k, blocknbr, blocksize, matrixsize, tempm;
    pastix_complex64_t *Akk, *Lik, *Ukj, *Aij;

    blocknbr = (pastix_int_t) ceil( (double)n/(double)MAXSIZEOFBLOCKS );

    Akk = A; /* Lk,k     */

    for (k=0; k<blocknbr; k++) {

        tempm = n - k * MAXSIZEOFBLOCKS;
        blocksize = pastix_imin(MAXSIZEOFBLOCKS, tempm);
        Lik = Akk + blocksize;
        Ukj = Akk + blocksize*lda;
        Aij = Ukj + blocksize;

        /* Factorize the diagonal block Akk*/
        core_zgetf2sp( tempm, blocksize, Akk, lda, nbpivot, criteria );

        matrixsize = tempm - blocksize;
        if ( matrixsize > 0 ) {

            /* Compute the column Ukk+1 */
            cblas_ztrsm(CblasColMajor,
                        CblasLeft, CblasLower,
                        CblasNoTrans, CblasUnit,
                        blocksize, matrixsize,
                        CBLAS_SADDR(zone), Akk, lda,
                                           Ukj, lda);

            /* Update Ak+1,k+1 = Ak+1,k+1 - Lk+1,k*Uk,k+1 */
            cblas_zgemm(CblasColMajor,
                        CblasNoTrans, CblasNoTrans,
                        matrixsize, matrixsize, blocksize,
                        CBLAS_SADDR(mzone), Lik, lda,
                                            Ukj, lda,
                        CBLAS_SADDR(zone),  Aij, lda);
        }

        Akk += blocksize * (lda+1);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp1d_getrf - Computes the LU factorization of one panel.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          Pointer to the structure representing the panel to factorize in the
 *          cblktab array.  Next column blok must be accessible through cblk[1].
 *
 * @param[in,out] L
 *          The pointer to the lower matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] U
 *          The pointer to the upper matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] criteria
 *          Threshold use for static pivoting. If diagonal value is under this
 *          threshold, its value is replaced by the threshold and the nu,ber of
 *          pivots is incremented.
 *
 *******************************************************************************
 *
 * @return
 *          This routine will fail if it discovers a 0. on the diagonal during
 *          factorization.
 *
 *******************************************************************************/
int core_zgetrfsp1d_getrf( SolverCblk         *cblk,
                           pastix_complex64_t *L,
                           pastix_complex64_t *U,
                           double              criteria)
{
    pastix_int_t ncols, stride;
    pastix_int_t nbpivot = 0;

    ncols  = cblk->lcolnum - cblk->fcolnum + 1;
    stride = cblk->stride;

    /* check if diagonal column block */
    assert( cblk->fcolnum == cblk->fblokptr->frownum );
    assert( cblk->lcolnum == cblk->fblokptr->lrownum );

    if ( !(cblk->cblktype & CBLK_DENSE) ) {
        L = cblk->fblokptr->LRblock[0].u;
        U = cblk->fblokptr->LRblock[1].u;
        stride = ncols;
    }

    core_zgeadd( CblasTrans, ncols, ncols,
                 1.0, U, stride,
                 1.0, L, stride );

    /* Factorize diagonal block */
    core_zgetrfsp(ncols, L, stride, &nbpivot, criteria);

    /* Transpose Akk in ucoeftab */
    core_zgetro(ncols, ncols, L, stride, U, stride);

    return nbpivot;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp1d_trsm - Apply all the trsm updates on one panel.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          Pointer to the structure representing the panel to factorize in the
 *          cblktab array.  Next column blok must be accessible through cblk[1].
 *
 * @param[in,out] L
 *          The pointer to the lower matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] U
 *          The pointer to the upper matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 *******************************************************************************
 *
 * @return
 *         \retval PASTIX_SUCCESS on successful exit.
 *
 *******************************************************************************/
int core_zgetrfsp1d_trsm( SolverCblk         *cblk,
                          pastix_complex64_t *L,
                          pastix_complex64_t *U)
{
    SolverBlok *fblok, *lblok;
    pastix_int_t dima, dimb, stride;

    dima   = cblk->lcolnum - cblk->fcolnum + 1;
    stride = cblk->stride;
    fblok  = cblk->fblokptr;   /* this diagonal block */
    lblok  = cblk[1].fblokptr; /* the next diagonal block */

    /* vertical dimension */
    dimb = stride - dima;

    /* if there is an extra-diagonal bloc in column block */
    if ( fblok+1 < lblok )
    {
        if (cblk->cblktype & CBLK_DENSE) {
            pastix_complex64_t *fL, *fU;

            /* first extra-diagonal bloc in column block address */
            fL = L + fblok[1].coefind;
            fU = U + fblok[1].coefind;

            cblas_ztrsm(CblasColMajor,
                        CblasRight, CblasUpper,
                        CblasNoTrans, CblasNonUnit,
                        dimb, dima,
                        CBLAS_SADDR(zone), L,  stride,
                                           fL, stride);

            cblas_ztrsm(CblasColMajor,
                        CblasRight, CblasUpper,
                        CblasNoTrans, CblasUnit,
                        dimb, dima,
                        CBLAS_SADDR(zone), U,  stride,
                                           fU, stride);
        }
        else {
            SolverBlok *blok;
            pastix_lrblock_t *lrblock;

            L = fblok->LRblock[0].u;
            U = fblok->LRblock[1].u;

            for(blok = fblok+1; blok<lblok; blok++) {

                /* Solve the lower part */
                lrblock = blok->LRblock;
                if (lrblock->rk != -1) {
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasNoTrans, CblasNonUnit,
                                lrblock->rk, dima,
                                CBLAS_SADDR(zone), L, stride,
                                lrblock->v, lrblock->rkmax);
                }
                else {
                    dimb = blok_rownbr( blok );
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasNoTrans, CblasNonUnit,
                                dimb, dima,
                                CBLAS_SADDR(zone), L, stride,
                                lrblock->u, dimb);
                }

                /* Solve the upper part */
                lrblock++;
                if (lrblock->rk != -1) {
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasNoTrans, CblasUnit,
                                lrblock->rk, dima,
                                CBLAS_SADDR(zone), U, stride,
                                lrblock->v, lrblock->rkmax);
                }
                else {
                    dimb = blok_rownbr( blok );
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasNoTrans, CblasUnit,
                                dimb, dima,
                                CBLAS_SADDR(zone), U, stride,
                                lrblock->u, dimb);
                }
            }
        }
    }

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp1d - Computes the LU factorization of one panel and apply
 * all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          Pointer to the structure representing the panel to factorize in the
 *          cblktab array.  Next column blok must be accessible through cblk[1].
 *
 * @param[in,out] L
 *          The pointer to the lower matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] U
 *          The pointer to the upper matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] criteria
 *          Threshold use for static pivoting. If diagonal value is under this
 *          threshold, its value is replaced by the threshold and the nu,ber of
 *          pivots is incremented.
 *
 *******************************************************************************
 *
 * @return
 *          This routine will fail if it discovers a 0. on the diagonal during
 *          factorization.
 *
 *******************************************************************************/
int core_zgetrfsp1d_panel( SolverCblk         *cblk,
                           pastix_complex64_t *L,
                           pastix_complex64_t *U,
                           double              criteria)
{
    pastix_int_t nbpivot = core_zgetrfsp1d_getrf(cblk, L, U, criteria);
    core_zgetrfsp1d_trsm(cblk, L, U);
    return nbpivot;
}

void core_zgetrfsp1d_gemm_LR( SolverCblk         *cblk,
                              SolverBlok         *blok,
                              SolverCblk         *fcblk,
                              pastix_complex64_t *L,
                              pastix_complex64_t *U,
                              pastix_complex64_t *Cl,
                              pastix_complex64_t *Cu,
                              pastix_complex64_t *work )
{
    SolverBlok *iterblok;
    SolverBlok *fblok;
    SolverBlok *lblok;

    pastix_complex64_t *Aik, *Akj, *Aij, *C;
    pastix_int_t stride, stridefc, indblok;
    pastix_int_t dimi, dimj, dima, dimb;

    stride  = cblk->stride;
    dima = cblk->lcolnum - cblk->fcolnum + 1;

    pastix_complex64_t *Cd = fcblk->lcoeftab;
    pastix_int_t stride_D  = fcblk->stride;

    /* First blok */
    indblok = blok->coefind;

    dimj = blok->lrownum - blok->frownum + 1;
    dimi = stride - indblok;

    /* Matrix A = Aik */
    Aik = L + indblok;
    Akj = U + indblok;

    /* Get the first block of the distant panel */
    fblok = fcblk->fblokptr;

    /* Move the pointer to the top of the right column */
    stridefc = fcblk->stride;
    C = Cl + (blok->frownum - fcblk->fcolnum) * stridefc;

    lblok = cblk[1].fblokptr;

    /* TODO: apply contributions by facing cblk !!! */

    /* for all following blocks in block column */
    for (iterblok=blok; iterblok<lblok; iterblok++) {

        /* Find facing blok */
        while (!is_block_inside_fblock( iterblok, fblok ))
        {
            fblok++;
            assert( fblok < fcblk[1].fblokptr );
        }


        Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;
        dimb = iterblok->lrownum - iterblok->frownum + 1;
        pastix_cblk_lock( fcblk );

        Aik = L + iterblok->coefind;

        /* If the blok modifies a diagonal block */
        if (fblok->coefind + iterblok->frownum - fblok->frownum < stride_D){

            core_zproduct_lr2dense(iterblok, Aik, stride,
                                   dima, L_side,
                                   blok, Akj, stride,
                                   dima, U_side,
                                   work, dimi);

            Aij = Cd + (blok->frownum - fcblk->fcolnum) * stride_D
                + fblok->coefind + iterblok->frownum - fblok->frownum;
            core_zgeadd( CblasNoTrans, dimb, dimj,
                         -1.0, work, dimi,
                          1.0, Aij,  stride_D );
        }

        /* If the block modifies an off-diagonal blok */
        else{

            /* The blok receiving a contribution is LR */
            if (fblok->rankL != -1){

                core_zproduct_lr2lr(iterblok, Aik, stride,
                                    dima, L_side,
                                    blok, Akj, stride,
                                    dima, U_side,
                                    fblok, Cl + fblok->coefind,
                                    C + fblok->coefind + iterblok->frownum - fblok->frownum,
                                    stridefc,
                                    stride_D, L_side,
                                    fcblk->fcolnum,
                                    work);
            }
            /* The blok receiving a contribution is dense */
            else{

                core_zproduct_lr2dense(iterblok, Aik, stride,
                                       dima, L_side,
                                       blok, Akj, stride,
                                       dima, U_side,
                                       work, dimi);

                Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;

                core_zgeadd( CblasNoTrans, dimb, dimj,
                             -1.0, work, dimi,
                              1.0, Aij,  stridefc );
            }
        }
        pastix_cblk_unlock( fcblk );
    }

    /*
     * Compute update on U
     */
    Aik = U + indblok;
    Akj = L + indblok;

    /* Restore data */
    fblok = fcblk->fblokptr;

    /*
     * Add contribution to facing cblk
     */
    C  = Cd + (blok->frownum - fcblk->fcolnum);
    /* Cu = Cu + (blok->frownum - fcblk->fcolnum) * stridefc; */

    /*
     * Update on L part (blocks facing diagonal block)
     */
    for (iterblok=blok+1; iterblok<lblok; iterblok++) {

        /* Find facing bloknum */
        if (!is_block_inside_fblock( iterblok, fblok ))
            break;

        Aij = C + (iterblok->frownum - fblok->frownum)*stride_D;
        dimb = iterblok->lrownum - iterblok->frownum + 1;
        Aik = U + iterblok->coefind;

        core_zproduct_lr2dense(iterblok, Aik, stride,
                               dima, U_side,
                               blok, Akj, stride,
                               dima, L_side,
                               work, dimi);

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasTrans, dimj, dimb,
                     -1.0, work, dimi,
                      1.0, Aij,  stride_D);
        pastix_cblk_unlock( fcblk );
    }

    /* C = Cu; */
    C = Cu + (blok->frownum - fcblk->fcolnum) * stridefc;

    /*
     * For all following blocks in block column,
     * Update is done on U directly
     */
    /* Keep updating on U */
    for (; iterblok<lblok; iterblok++) {

        /* Find facing bloknum */
        while (!is_block_inside_fblock( iterblok, fblok ))
        {
            fblok++;
            assert( fblok < fcblk[1].fblokptr );
        }

        dimb = iterblok->lrownum - iterblok->frownum + 1;
        Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;
        Aik = U + iterblok->coefind;

        /* The blok receiving a contribution is LR */
        if (fblok->rankU != -1){

            core_zproduct_lr2lr(iterblok, Aik, stride,
                                dima, U_side,
                                blok, Akj, stride,
                                dima, L_side,
                                fblok, Cu + fblok->coefind,
                                C + fblok->coefind + iterblok->frownum - fblok->frownum,
                                stridefc,
                                stride_D, U_side,
                                fcblk->fcolnum,
                                work);
        }
        /* The blok receiving a contribution is dense */
        else{

            core_zproduct_lr2dense(iterblok, Aik, stride,
                                   dima, U_side,
                                   blok, Akj, stride,
                                   dima, L_side,
                                   work, dimi);

            pastix_cblk_lock( fcblk );
            core_zgeadd( CblasNoTrans, dimb, dimj,
                         -1.0, work, dimi,
                          1.0, Aij,  stridefc );
            pastix_cblk_unlock( fcblk );
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp1d - Computes the Cholesky factorization of one panel, apply all
 * the trsm updates to this panel, and apply all updates to the right with no
 * lock.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          Pointer to the structure representing the panel to factorize in the
 *          cblktab array.  Next column blok must be accessible through cblk[1].
 *
 * @param[in] criteria
 *          Threshold use for static pivoting. If diagonal value is under this
 *          threshold, its value is replaced by the threshold and the number of
 *          pivots is incremented.
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal block.
 *
 *******************************************************************************/
int
core_zgetrfsp1d( SolverMatrix       *solvmtx,
                 SolverCblk         *cblk,
                 double              criteria,
                 pastix_complex64_t *work)
{
    pastix_complex64_t *L = cblk->lcoeftab;
    pastix_complex64_t *U = cblk->ucoeftab;
    SolverCblk  *fcblk;
    SolverBlok  *blok, *lblk;
    pastix_int_t nbpivot;

    nbpivot = core_zgetrfsp1d_panel(cblk, L, U, criteria);

    blok = cblk->fblokptr + 1; /* this diagonal block */
    lblk = cblk[1].fblokptr;   /* the next diagonal block */

    /* if there are off-diagonal supernodes in the column */
    for( ; blok < lblk; blok++ )
    {
        fcblk = (solvmtx->cblktab + blok->fcblknm);

        if ( cblk->cblktype & CBLK_DENSE) {
            /* Update on L */
            core_zgemmsp( PastixLower, PastixTrans, cblk, blok, fcblk,
                          L, U, fcblk->lcoeftab, work );

            /* Update on U */
            if ( blok+1 < lblk ) {
                core_zgemmsp( PastixUpper, PastixTrans, cblk, blok, fcblk,
                              U, L, fcblk->ucoeftab, work );
            }
        }
        else {
            core_zgetrfsp1d_gemm_LR( cblk, blok, fcblk,
                                     L, U, fcblk->lcoeftab, fcblk->ucoeftab, work );
        }
        pastix_atomic_dec_32b( &(fcblk->ctrbcnt) );
    }

    return nbpivot;
}
