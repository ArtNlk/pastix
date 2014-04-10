/**
 *
 * @file core_zhetrfsp.c
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
 * @precisions normal z -> c
 *
 **/
#include <assert.h>

#include "common.h"
#include "pastix_zcores.h"
#include <cblas.h>

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t mzone = -1.;

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zhetf2sp - Computes the sequential static pivoting
 * factorization of the hermitian matrix n-by-n A such that A = L * D * conj(L^t).
 *
 *******************************************************************************
 *
 * @param[in] n
 *          The number of rows and columns of the matrix A.
 *
 * @param[in,out] A
 *          The matrix A to factorize with Cholesky factorization. The matrix
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
static void core_zhetf2sp(pastix_int_t        n,
                          pastix_complex64_t *A,
                          pastix_int_t        lda,
                          pastix_int_t       *nbpivot,
                          double              criteria )
{
    pastix_int_t k;
    pastix_complex64_t *Akk = A;   /* A [k  ][k] */
    pastix_complex64_t *Amk = A+1; /* A [k+1][k] */
    pastix_complex64_t  zalpha;
    double  dalpha;

    for (k=0; k<n; k++){
        if ( cabs(*Akk) < criteria ) {
            (*Akk) = (pastix_complex64_t)criteria;
            (*nbpivot)++;
        }

        zalpha = 1. / (*Akk);

        /* Scale the diagonal to compute L((k+1):n,k) */
        cblas_zscal(n-k-1, CBLAS_SADDR( zalpha ), Amk, 1 );

        dalpha = (double)(-(*Akk));

        /* Move to next Akk */
        Akk += (lda+1);

        cblas_zher(CblasColMajor, CblasLower,
                   n-k-1, dalpha,
                   Amk, 1,
                   Akk, lda);

        /* Move to next Amk */
        Amk = Akk+1;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zhetrfsp - Computes the block static pivoting factorization of the
 * hermitian matrix n-by-n A such that A = L * D * conj(L^t).
 *
 *******************************************************************************
 *
 * @param[in] n
 *          The number of rows and columns of the matrix A.
 *
 * @param[in,out] A
 *          The matrix A to factorize with Cholesky factorization. The matrix
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

static void core_zhetrfsp(pastix_int_t        n,
                          pastix_complex64_t *A,
                          pastix_int_t        lda,
                          pastix_int_t       *nbpivot,
                          double              criteria,
                          pastix_complex64_t *work)
{
    pastix_int_t k, blocknbr, blocksize, matrixsize, col;
    pastix_complex64_t *tmp,*tmp1,*tmp2;
    pastix_complex64_t alpha;

    /* diagonal supernode is divided into MAXSIZEOFBLOCK-by-MAXSIZEOFBLOCKS blocks */
    blocknbr = (pastix_int_t) ceil( (double)n/(double)MAXSIZEOFBLOCKS );

    for (k=0; k<blocknbr; k++) {

        blocksize = pastix_imin(MAXSIZEOFBLOCKS, n-k*MAXSIZEOFBLOCKS);
        tmp  = A+(k*MAXSIZEOFBLOCKS)*(lda+1); /* Lk,k     */
        tmp1 = tmp  + blocksize;              /* Lk+1,k   */
        tmp2 = tmp1 + blocksize * lda;        /* Lk+1,k+1 */

        /* Factorize the diagonal block Akk*/
        core_zhetf2sp(blocksize, tmp, lda, nbpivot, criteria);

        if ((k*MAXSIZEOFBLOCKS+blocksize) < n) {

            matrixsize = n-(k*MAXSIZEOFBLOCKS+blocksize);

            /* Compute the column L(k+1:n,k) = (L(k,k)D(k,k))^{-1}A(k+1:n,k)    */
            /* 1) Compute A(k+1:n,k) = A(k+1:n,k)L(k,k)^{-T} = D(k,k)L(k+1:n,k) */
                        /* input: L(k,k) in tmp, A(k+1:n,k) in tmp1   */
                        /* output: A(k+1:n,k) in tmp1                 */
            cblas_ztrsm(CblasColMajor,
                        CblasRight, CblasLower,
                        CblasConjTrans, CblasUnit,
                        matrixsize, blocksize,
                        CBLAS_SADDR(zone), tmp,  lda,
                                           tmp1, lda);

            /* Compute L(k+1:n,k) = A(k+1:n,k)D(k,k)^{-1}     */
            for(col = 0; col < blocksize; col++) {
                /* copy L(k+1+col:n,k+col)*D(k+col,k+col) into work(:,col) */
                cblas_zcopy(matrixsize, tmp1+col*lda,     1,
                                        work+col*matrixsize, 1);

                                /* compute L(k+1+col:n,k+col) = A(k+1+col:n,k+col)D(k+col,k+col)^{-1} */
                alpha = 1. / *(tmp + col*(lda+1));
                cblas_zscal(matrixsize, CBLAS_SADDR(alpha),
                            tmp1+col*lda, 1);
            }

            /* Update A(k+1:n,k+1:n) = A(k+1:n,k+1:n) - (L(k+1:n,k)*D(k,k))*L(k+1:n,k)^T */
            cblas_zgemm(CblasColMajor,
                        CblasNoTrans, CblasConjTrans,
                        matrixsize, matrixsize, blocksize,
                        CBLAS_SADDR(mzone), work, matrixsize,
                                            tmp1, lda,
                        CBLAS_SADDR(zone),  tmp2, lda);
        }
    }
}


/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zhetrfsp1d - Computes the LDL^H factorization of one panel and apply
 * all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in] datacode
 *          TODO
 *
 * @param[in] cblknum
 *          Index of the panel to factorize in the cblktab array.
 *
 * @param[in,out] L
 *          The pointer to the matrix storing the coefficients of the
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
 *          The number of static pivoting during factorization of the diagonal block.
 *
 *******************************************************************************/
int core_zhetrfsp1d( SolverMatrix       *datacode,
                     pastix_int_t        cblknum,
                     pastix_complex64_t *L,
                     double              criteria,
                     pastix_complex64_t *work )
{
    SolverCblk   *cblk;
    SolverBlok   *blok;
    pastix_int_t  ncols, stride;
    pastix_int_t  fblknum, lblknum;
    pastix_int_t  nbpivot = 0;

    cblk    = &(datacode->cblktab[cblknum]);
    ncols   = cblk->lcolnum - cblk->fcolnum + 1;
    stride  = cblk->stride;
    fblknum = cblk->bloknum;   /* block number of this diagonal block */
    lblknum = cblk[1].bloknum; /* block number of the next diagonal block */

    /* check if diagonal column block */
    blok = &(datacode->bloktab[fblknum]);
    assert( cblk->fcolnum == blok->frownum );
    assert( cblk->lcolnum == blok->lrownum );

    /* Factorize diagonal block (two terms version with workspace) */
    core_zhetrfsp(ncols, L, stride, &nbpivot, criteria, work);

    /* if there are off-diagonal supernodes in the column */
    if ( fblknum+1 < lblknum )
    {
        pastix_complex64_t *fL;
        pastix_int_t nrows;

        /* vertical dimension */
        nrows = stride - ncols;

        /* the first off-diagonal block in column block address */
        fL = L + blok[1].coefind;

        /* Three terms version, no need to keep L and L*D */
        cblas_ztrsm(CblasColMajor,
                    CblasRight, CblasLower,
                    CblasConjTrans, CblasUnit,
                    nrows, ncols,
                    CBLAS_SADDR(zone), L,  stride,
                                       fL, stride);

        for (pastix_int_t k=0; k<ncols; k++)
        {
            pastix_complex64_t alpha;
            alpha = 1. / L[k+k*stride];
            cblas_zscal(nrows, CBLAS_SADDR(alpha), &(fL[k*stride]), 1);
        }
    }

    return nbpivot;
}


/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zhetrfsp1d - Computes the Cholesky factorization of one panel and apply
 * all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in] datacode
 *          TODO
 *
 * @param[in] cblk
 *          The pointer to the data structure that describes the panel to be
 *          factorized. Must be at least of size 2 in order to get the first
 *          block number of the following panel (cblk[1]).
 *
 * @param[in,out] L
 *          The pointer to the matrix storing the coefficients of the
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
 *          The number of static pivoting during factorization of the diagonal block.
 *
 *******************************************************************************/
void core_zhetrfsp1d_gemm( SolverMatrix       *datacode,
                           pastix_int_t        cblknum,
                           pastix_int_t        bloknum,
                           pastix_int_t        fcblknum,
                           pastix_complex64_t *L,
                           pastix_complex64_t *C,
                           pastix_complex64_t *work1,
                           pastix_complex64_t *work2 )
{
    SolverCblk *cblk  = &(datacode->cblktab[cblknum]);
    SolverCblk *fcblk = &(datacode->cblktab[fcblknum]);
    SolverBlok *blok;
    SolverBlok *fblok;

    pastix_complex64_t *Aik, *Aij;
    pastix_int_t lblknum;
    pastix_int_t stride, stridefc, indblok;
    pastix_int_t b, j;
    pastix_int_t dimi, dimj, dima, dimb;
    pastix_int_t ldw;

    stride  = cblk->stride;
    dima = cblk->lcolnum - cblk->fcolnum + 1;

    /* First blok */
    j = bloknum;
    blok = &(datacode->bloktab[bloknum]);
    indblok = blok->coefind;

    dimj = blok->lrownum - blok->frownum + 1;
    dimi = stride - indblok;

    /* Matrix A = Aik */
    Aik = L + indblok;

    /* Compute ldw which should never be larger than SOLVE_COEFMAX */
    ldw = dimi * dima;

    /* Compute the contribution */
    core_zgemdm( CblasNoTrans, CblasConjTrans,
                 dimi, dimj, dima,
                 1.,  Aik,   stride,
                      Aik,   stride,
                 0.,  work1, dimi,
                      L,     stride+1,
                      work2, ldw );

    /*
     * Add contribution to facing cblk
     * A(i,i+1:n) += work1
     */

    /* Get the first block of the distant panel */
    b     = fcblk->bloknum;
    fblok = &(datacode->bloktab[ b ]);

    /* Move the pointer to the top of the right column */
    stridefc = fcblk->stride;
    C = C + (blok->frownum - fcblk->fcolnum) * stridefc;

    lblknum = cblk[1].bloknum;

    /* for all following blocks in block column */
    for (j=bloknum; j<lblknum; j++,blok++) {

        /* Find facing bloknum */
        while (!is_block_inside_fblock( blok, fblok ))
        {
            b++; fblok++;
            assert( b < fcblk[1].bloknum );
        }

        Aij = C + fblok->coefind + blok->frownum - fblok->frownum;
        dimb = blok->lrownum - blok->frownum + 1;

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                     work1, dimi,
                     Aij,   stridefc );
        pastix_cblk_unlock( fcblk );

        /* Displacement to next block */
        work1 += dimb;
    }
}
