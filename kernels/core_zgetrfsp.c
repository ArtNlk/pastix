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
#include "pastix_zcores.h"
#include <cblas.h>
#include "../blend/solver.h"
#include <lapacke.h>

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t mzone = -1.;
static pastix_complex64_t zzero =  0.;

#if defined(PASTIX_WITH_HODLR)
#include "cHODLR_Matrix.h"
#endif /* defined(PASTIX_WITH_HODLR) */

pastix_int_t z_compress_LR(pastix_complex64_t *fL,
                           pastix_int_t dima,
                           pastix_int_t dimb,
                           double *s,
                           pastix_complex64_t *u,
                           pastix_int_t ldu,
                           pastix_complex64_t *v,
                           pastix_int_t ldv,
                           pastix_int_t stride){

    double superb[dimb];
    pastix_complex64_t *block;
    pastix_int_t        ret;
    pastix_int_t        i;

    pastix_int_t dim_min = dima;
    if (dimb < dim_min)
        dim_min = dimb;

    block = malloc( dima * dimb * sizeof(pastix_complex64_t));
    if (block == NULL){
        printf("STRANGE\n");
        exit(1);
    }
    for (i=0; i<dima; i++){
        memcpy( block + i * dimb, fL + i * stride, dimb * sizeof(pastix_complex64_t));
    }

    /* memset(block, 0, dima * dimb * sizeof(pastix_complex64_t)); */

    ret = LAPACKE_zgesvd( CblasColMajor, 'A', 'A',
                          dimb, dima, block, dimb,
                          s, u, ldu, v, ldv, superb );

    if( ret != 0 ){
        printf("SVD FAILED %ld\n\n", ret);
        return -1;
    }

    char *tol        = getenv("TOLERANCE");
    double tolerance = atof(tol);

    pastix_int_t rank = dim_min;
    for (i=0; i<dim_min-1; i++){
        if (s[i] / s[0] < tolerance){
            rank = i+1;
            break;
        }
    }

    for (i=rank; i<dim_min; i++){
        s[i] = 0;
    }

    for (i=0; i<dim_min; i++){
        cblas_dscal(dimb, s[i], &(u[i*ldu]), 1);
    }

    free(block);
    return rank;
}

void z_uncompress_LR(pastix_complex64_t *fL,
                     pastix_int_t dima,
                     pastix_int_t dimb,
                     pastix_complex64_t *u,
                     pastix_int_t ldu,
                     pastix_complex64_t *v,
                     pastix_int_t ldv,
                     pastix_int_t stride,
                     pastix_int_t rank){

    cblas_zgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                dimb, dima, rank,
                CBLAS_SADDR(zone),  u,  ldu,
                                    v,  ldv,
                CBLAS_SADDR(zzero), fL, stride);
}

pastix_int_t z_add_LR(pastix_complex64_t *u1,
                      pastix_complex64_t *v1,
                      pastix_complex64_t *u2,
                      pastix_complex64_t *v2,
                      pastix_int_t dim_u1,
                      pastix_int_t dim_v1,
                      pastix_int_t rank_1,
                      pastix_int_t dim_u2,
                      pastix_int_t dim_v2,
                      pastix_int_t rank_2,
                      pastix_int_t ldv2,
                      pastix_int_t x2,
                      pastix_int_t y2){

    /* Idem for block u2 v2^T */
    /* u1 is of size dim_u1 * rank_1 */
    /* v1 is of size dim_u1 * rank_1 */

    /* We suppose u1 v1^T is the matrix which receive the contribution */
    /* Then, dim_u1 >= dim_u2 and dim_v1 >= dim_v2 */
    /* (x2, y2) represents the first index of u2 v2^T contributing to u1 v1^T */

    pastix_int_t dim_u = pastix_imax(dim_u1, dim_u2);
    pastix_int_t dim_v = pastix_imax(dim_v1, dim_v2);
    pastix_int_t rank  = rank_1 + rank_2;
    pastix_int_t i, j;

    if (dim_u2 > dim_u1 || dim_v2 > dim_v1){
        printf("\n\nDimensions are not correct\n\n");
        exit(1);
    }

    pastix_int_t minMN_1 = pastix_imin(dim_u, rank);
    /* Rank is too high for u1u2 */
    if (minMN_1 == dim_u){
        return -1;
    }

    pastix_int_t minMN_2 = pastix_imin(dim_v, rank);
    /* Rank is too high for v1v2 */
    if (minMN_2 == dim_v){
        return -1;
    }

    /* u1v1^T + u2v2^T = (u1 u2) (v1 v2)^T */
    /* Compute QR decomposition of (u1 u2) = Q1 R1 */
    /* Compute QR decomposition of (v1 v2) = Q2 R2 */
    /* Compute SVD of R1 R2^T = u \sigma v^T*/
    /* Final solution is (Q1 u \sigma^[1/2]) (Q2 v \sigma^[1/2])^T */

    pastix_complex64_t *u1u2, *v1v2;
    u1u2 = malloc( dim_u * rank * sizeof(pastix_complex64_t));
    v1v2 = malloc( dim_v * rank * sizeof(pastix_complex64_t));

    /* TODO: apply when dim_u1 != dim_u2 AND dim_v1 != dim_v2 */
    memset(u1u2, 0, dim_u * rank * sizeof(pastix_complex64_t));
    memset(v1v2, 0, dim_v * rank * sizeof(pastix_complex64_t));

    /* printf("DIM %ld %ld %ld\n", dim_u1, dim_u2, dim_u); */

    memcpy(u1u2, u1, dim_u1 * rank_1 * sizeof(pastix_complex64_t));
    for (i=0; i<rank_2; i++){
        for (j=0; j<dim_u2; j++){
            u1u2[dim_u * (rank_1 + i) + x2 + j] = u2[i * dim_u2 + j];
        }
    }

    for (i=0; i<dim_v1; i++){
        for (j=0; j<rank_1; j++){
            v1v2[dim_v * j + i]                 = v1[i * dim_v1 + j];
        }
    }

    /* WARNING: minus because of the extend add */
    for (i=0; i<dim_v2; i++){
        for (j=0; j<rank_2; j++){
            v1v2[dim_v * (rank_1 + j) + i + y2] = -v2[i * ldv2 + j];
        }
    }

    pastix_int_t ret;
    pastix_complex64_t *tau1 = malloc( minMN_1 * sizeof(pastix_complex64_t));
    memset(tau1, 0, minMN_1 * sizeof(pastix_complex64_t));
    ret = LAPACKE_zgeqrf( CblasColMajor, dim_u, rank,
                          u1u2, dim_u, tau1 );


    pastix_complex64_t *tau2 = malloc( minMN_2 * sizeof(pastix_complex64_t));
    memset(tau2, 0, minMN_2 * sizeof(pastix_complex64_t));
    ret = LAPACKE_zgeqrf( CblasColMajor, dim_v, rank,
                          v1v2, dim_v, tau2 );

    pastix_complex64_t *R1, *R2, *R;
    R1 = malloc(rank * rank * sizeof(pastix_complex64_t));
    R2 = malloc(rank * rank * sizeof(pastix_complex64_t));
    R  = malloc(rank * rank * sizeof(pastix_complex64_t));
    memset(R1, 0, rank * rank * sizeof(pastix_complex64_t));
    memset(R2, 0, rank * rank * sizeof(pastix_complex64_t));
    memset(R , 0, rank * rank * sizeof(pastix_complex64_t));

    for (i=0; i<rank; i++){
        memcpy(R1 + rank * i, u1u2 + dim_u * i, (i+1) * sizeof(pastix_complex64_t));
        memcpy(R2 + rank * i, v1v2 + dim_v * i, (i+1) * sizeof(pastix_complex64_t));
    }

    /* printf("RANKS %ld %ld\n", rank_1, rank_2); */
    /* for (i=0; i<rank; i++){ */
    /*     for (j=0; j<rank; j++){ */
    /*         printf("%10.3g ", R2[i*rank+j]); */
    /*     } */
    /*     printf("\n"); */
    /* } */

    /* Compute R1 R2 */
    cblas_zgemm(CblasColMajor, CblasNoTrans, CblasTrans,
                rank, rank, rank,
                CBLAS_SADDR(zone),  R1, rank,
                                    R2, rank,
                CBLAS_SADDR(zzero), R,  rank);

    double superb[rank];
    double *s;
    pastix_complex64_t *u, *v;
    s = malloc( rank * sizeof(double));
    u = malloc( rank * rank * sizeof(pastix_complex64_t));
    v = malloc( rank * rank * sizeof(pastix_complex64_t));
    memset(s, 0, rank * sizeof(double));
    memset(u, 0, rank * rank * sizeof(pastix_complex64_t));
    memset(v, 0, rank * rank * sizeof(pastix_complex64_t));

    ret = LAPACKE_zgesvd( CblasColMajor, 'A', 'A',
                          rank, rank, R, rank,
                          s, u, rank, v, rank, superb );

    if (ret != 0){
        printf("LAPACKE_zgesvd FAILED\n");
        exit(1);
    }

    /* Keep rank as constant wrt the block receiving contribution */
    /* for (i=rank_1; i<rank; i++){ */
    /*     if (fabs(s[i]) > 1e-16){ */
    /*         printf("Strange as we add two identical matrices 0 %.3g i-1 %.3g i %.3g\n", */
    /*                s[0], s[i-1], s[i]); */
    /*     } */
    /* } */

    pastix_int_t new_rank = rank;
    char *tol             = getenv("TOLERANCE");
    double tolerance      = atof(tol);

    /* for (i=0; i<rank-1; i++){ */
    /*     if (s[i] / s[0] < tolerance){ */
    /*         new_rank = i+1; */
    /*         break; */
    /*     } */
    /* } */
    /* for (i=new_rank; i<rank; i++){ */
    /*     s[i] = 0; */
    /* } */


    /* Scal u as before to take into account singular values */
    for (i=0; i<rank; i++){
        cblas_dscal(rank, s[i], &(u[rank * i]), 1);
    }

    memset(u1, 0, dim_u1 * dim_u1 * sizeof(pastix_complex64_t));

    for (i=0; i<rank; i++){
        memcpy(u1 + dim_u * i, u + rank * i, rank * sizeof(pastix_complex64_t));
    }

    /* We need non-transposed version of v */
    pastix_complex64_t *v3;
    v3 = malloc(  dim_v * dim_v * sizeof(pastix_complex64_t));
    memset(v3, 0, dim_v * dim_v * sizeof(pastix_complex64_t));

    for (i=0; i<rank; i++){
        for (j=0; j<rank; j++){
            v3[dim_v * j + i] = v[rank * i + j];
        }
    }

    ret = LAPACKE_dormqr(CblasColMajor, 'L', 'N',
                         dim_u, rank, minMN_1, /* to be replaced by rank */
                         u1u2, dim_u, tau1,
                         u1, dim_u1);

    /* We are suppose to apply this on right */
    ret = LAPACKE_dormqr(CblasColMajor, 'L', 'N',
                         dim_v, rank, minMN_2, /* to be replaced by rank */
                         v1v2, dim_v, tau2,
                         v3, dim_v1);


    /* We added two identical matrices */
    /* for (i=0; i<dim_u1; i++){ */
    /*     for (j=0; j<dim_u1; j++){ */
    /*         u1[dim_u1*i+j] *= 0.5; */
    /*     } */
    /* } */

    memset(v1, 0, dim_v * dim_v * sizeof(pastix_complex64_t));
    for (i=0; i<rank; i++){
        for (j=0; j<dim_v1; j++){
            v1[dim_v1 * j + i] = v3[dim_v1 * i + j];
        }
    }

    /* Set unused part of u to 0 */
    for (i=rank; i<dim_u1; i++){
        for (j=0; j<dim_u1; j++){
            u1[i*dim_u1+j] = 0;
        }
    }

    printf("Rank was OK to add the two LR structures %ld %ld %ld %ld\n",
           dim_u, rank_1, rank_2, new_rank);

    free(u1u2);
    free(v1v2);
    free(u);
    free(s);
    free(v);
    free(tau1);
    free(tau2);
    free(R);
    free(R1);
    free(R2);
    free(v3);
    return new_rank;
}

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
static pastix_int_t compute_cblklevel( pastix_int_t cblknum )
{
    /* WARNING: TODO REMOVE !!! */
    return -1;
    /* cblknum level has already been computed */
    pastix_int_t father = treetab[cblknum];
    if ( father == -1 ) {
        return 1;
    }
    else {
        return compute_cblklevel( father ) + 1;
    }
}

int core_zgetrfsp1d_getrf( SolverCblk         *cblk,
                           pastix_complex64_t *D,
                           pastix_complex64_t *nul,
                           double              criteria)
{
    (void) nul;
    pastix_int_t ncols, n;
    pastix_int_t nbpivot = 0;

    ncols = cblk->lcolnum - cblk->fcolnum + 1;

    /* n corresponds to the size of diagonal block (n-by-n supported) */
    n = cblk->lcolnum - cblk->fcolnum + 1;

    /* Level in the elimination tree of the supernode beeing factorized */
    pastix_int_t level = compute_cblklevel( current_cblk );
    (void) level;

    /* check if diagonal column block */
    assert( cblk->fcolnum == cblk->fblokptr->frownum );
    assert( cblk->lcolnum == cblk->fblokptr->lrownum );

    pastix_complex64_t tolerance;
    pastix_int_t       levels, split, threshold, hodlrtree;

    char *lvls = getenv("LEVELS");
    levels     = atoi(lvls);
    char *tol  = getenv("TOLERANCE");
    tolerance  = atof(tol);
    char *splt = getenv("SPLITSIZE");
    split      = atoi(splt);
    char *thr  = getenv("THRESHOLD");
    threshold  = atoi(thr);
    char *tree = getenv("HODLRTREE");
    hodlrtree  = atoi(tree);

    (void) levels;
    (void) tolerance;
    (void) split;
    (void) threshold;
    (void) hodlrtree;

    /* TODO: remove */
    current_cblk++;

#if defined(PASTIX_WITH_HODLR)
    if (current_cblk == 0){
        printf(" Size | Level | Sparse Storage | HODLR_Storage | Dense_Storage |   Gain (Mo)   | HODLR compression+factorization | Dense factorization | TRSM on U | TRSM on L | Bloks size\n");
    }

    if (n > split && level <= levels){
        Clock timer;
        clockStart(timer);

        /* Print size, level */
        printf("%5ld  %5ld ", n, level);

        /* Get sparse matrix size */
        pastix_int_t sparse_storage = 0;
        pastix_int_t i, j;
        for (i=0; i<n; i++){
            for (j=0; j<n; j++){
                if (cabs(D[n*i+j]) != 0.0)
                    sparse_storage++;
            }
        }
        printf("%9ld   ", sparse_storage);


        /* Add sparse matrix and low-rank updates */
        /* pastix_complex64_t *D_HODLR = cblk->dcoeftab_HODLR; */
        /* core_zgeadd( CblasNoTrans, n, n, 1.0, */
        /*              D_HODLR, n, */
        /*              D, n ); */


        cHODLR schur_H;

        /* Number of partitions returned by SplitSymbol() */
        pastix_int_t split_size = cblk->split_size;

        pastix_int_t nb_parts;
        pastix_int_t *parts;

        if (split_size == 0){
            schur_H = newHODLR(n, n, D, threshold, "SVD", &nb_parts, &parts);
        }
        else{
            if (hodlrtree == 0){
                schur_H = newHODLR(n, n, D, threshold, "SVD", &nb_parts, &parts);
            }
            else{
                schur_H = newHODLR_Tree(n, n, D, threshold, "SVD",
                                        split_size, cblk->split,
                                        &nb_parts, &parts);
            }
        }

        cblk->nb_parts = nb_parts;
        cblk->parts    = parts;

        gain_D += cStoreLR(schur_H, tolerance, 1);

        cblk->is_HODLR = 1;
        cblk->cMatrix  = schur_H;

        clockStop(timer);

        /* Print compression+factorization time */
        printf("%32.3g ", (double)clockVal(timer));
    }
#endif /* defined(PASTIX_WITH_HODLR) */
    /* else */
    {
        Clock timer;
        clockStart(timer);

        core_zgetrfsp(ncols, D, n, &nbpivot, criteria);

        clockStop(timer);

#if defined(PASTIX_WITH_HODLR)
        if (cblk->is_HODLR == 1){
            /* Print dense factorisation time */
            printf("%20.3g ", (double)clockVal(timer));
        }
#endif /* defined(PASTIX_WITH_HODLR) */
    }

    /* Symmetric case */
    double local_memory = n*n*8/1000000.;
    local_memory += (cblk->stride - n)*n*8/1000000.;
    total_memory += local_memory;

    /* Unsymmetric case */
    local_memory   = n*n*8/1000000.;
    local_memory  += 2*(cblk->stride - n)*n*8/1000000.;
    total_memory2 += local_memory;
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
    (void)L;
    SolverBlok *fblok, *lblok;
    pastix_complex64_t *fU;
    pastix_int_t dima, dimb, stride;

    dima    = cblk->lcolnum - cblk->fcolnum + 1;
    stride  = cblk->stride;
    fblok = cblk->fblokptr;   /* this diagonal block */
    lblok = cblk[1].fblokptr; /* the next diagonal block */

    /* vertical dimension */
    dimb = stride - dima;

    pastix_complex64_t *D = cblk->dcoeftab;
    pastix_int_t stride_D = cblk->lcolnum - cblk->fcolnum + 1;

    /* To compress by facing cblk */
#if defined(PASTIX_WITH_HODLR)
    char *face          = getenv("FACING");
    pastix_int_t facing = atoi(face);

    if ( fblok+1 < lblok && facing == 1)
    {
        SymbolBlok *blok;
        pastix_int_t totalsize = 0;
        pastix_int_t nb_bloks  = 0;

        pastix_int_t facing    = 0;
        pastix_int_t localsize = 0;
        pastix_int_t current   = 0;

        if ( fblok+1 < lblok ){
            while (totalsize != dimb){

                blok = &fblok[nb_bloks+1];
                pastix_int_t bloksize  = blok->lrownum - blok->frownum + 1;

                if (blok->fcblknm != facing){
                    if (facing != 0){
                        fU = U + fblok[1].coefind + current;
                        if (cblk->is_HODLR == 1){
                            int rank;
                            gain_U += compress_SVD(fU, dima, localsize, &rank);
                        }
                    }

                    current  += localsize;
                    facing    = blok->fcblknm;
                    localsize = bloksize;
                }
                else{
                    localsize += bloksize;
                }

                totalsize += bloksize;
                nb_bloks++;
            }

            if (facing != 0){
                fU = U + fblok[1].coefind + current;

                if (cblk->is_HODLR == 1){
                    int rank;
                    gain_U += compress_SVD(fU, dima, localsize, &rank);
                }
            }
        }
    }
#endif /* defined(PASTIX_WITH_HODLR) */

    /* if there is an extra-diagonal bloc in column block */
    if ( fblok+1 < lblok )
    {
        /* first extra-diagonal bloc in column block address */
        fU = U + fblok[1].coefind;

#if defined(PASTIX_WITH_HODLR)
        if (cblk->is_HODLR == 1 && facing == 0){
            int rank;
            gain_U += compress_SVD(fU, dima, dimb, &rank);
        }

        /* Solve on U */
        if (cblk->is_HODLR == 0)
#endif /* defined(PASTIX_WITH_HODLR) */
        {

            /* We do not compress here, but in A cf sequential_zgetrf.c */
            if (0){
                printf("TOTO\n");
                pastix_int_t rank = -1;
                double *s;
                pastix_complex64_t *u, *v;

                pastix_int_t dim_min = pastix_imin(dima, dimb);

                s = malloc( dim_min * sizeof(double));
                u = malloc( dimb * dimb * sizeof(pastix_complex64_t));
                v = malloc( dima * dima * sizeof(pastix_complex64_t));

                memset(s, 0, dim_min     * sizeof(double));
                memset(u, 0, dimb * dimb * sizeof(pastix_complex64_t));
                memset(v, 0, dima * dima * sizeof(pastix_complex64_t));

                rank = z_compress_LR(fU, dima, dimb,
                                     s, u, dimb, v, dima, stride);

                /* Add two identical LR matrices */
                 z_add_LR(u, v,
                          u, v,
                          dimb, dima, rank,
                          dimb, dima, rank, dima,
                          0, 0);

                 z_uncompress_LR(fU, dima, dimb,
                                 u, dimb,
                                 v, dima,
                                 stride, rank);

                free(s);
                free(u);
                free(v);
            }

            pastix_int_t total = dimb;
            SolverBlok *blok   = fblok;
            while (total != 0){
                blok++;
                pastix_int_t dimb = blok->lrownum - blok->frownum + 1;
                total -= dimb;

                /* Update the data blok */
                fU = U + blok->coefind;

                pastix_int_t rank = blok->rankU;
                if (rank != -1){
                    /* printf("TRSM LR version\n"); */
                    /* printf("TRSM on a compressed blok of rank %ld (cblk %ld)\n", */
                    /*        blok->rankU, current_cblk); */

                    pastix_complex64_t *u = blok->coefU_u_LR;
                    pastix_complex64_t *v = blok->coefU_v_LR;

                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasLower,
                                CblasTrans, CblasUnit,
                                rank, dima,
                                CBLAS_SADDR(zone), D, stride_D,
                                v, dima);
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasTrans, CblasNonUnit,
                                rank, dima,
                                CBLAS_SADDR(zone), D, stride_D,
                                v, dima);
                }
                else{
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasLower,
                                CblasTrans, CblasUnit,
                                dimb, dima,
                                CBLAS_SADDR(zone), D, stride_D,
                                fU, stride);
                    cblas_ztrsm(CblasColMajor,
                                CblasRight, CblasUpper,
                                CblasTrans, CblasNonUnit,
                                dimb, dima,
                                CBLAS_SADDR(zone), D, stride_D,
                                fU, stride);
                }
            }
        }
#if defined(PASTIX_WITH_HODLR)
        else{
            cLU_TRSM(cblk->cMatrix, fU, dimb, dima, stride);
        }
#endif /* defined(PASTIX_WITH_HODLR) */
    }

    return PASTIX_SUCCESS;
}

int core_zgetrfsp1d_uncompress( SolverCblk         *cblk,
                                pastix_complex64_t *L,
                                pastix_complex64_t *U)
{
    (void)L;
    SolverBlok *fblok, *lblok;
    pastix_complex64_t *fU, *fL;
    pastix_int_t dima, dimb, stride;

    dima    = cblk->lcolnum - cblk->fcolnum + 1;
    stride  = cblk->stride;
    fblok = cblk->fblokptr;   /* this diagonal block */
    lblok = cblk[1].fblokptr; /* the next diagonal block */

    /* vertical dimension */
    dimb = stride - dima;

    pastix_complex64_t *D = cblk->dcoeftab;
    pastix_int_t stride_D = cblk->lcolnum - cblk->fcolnum + 1;

    /* if there is an extra-diagonal bloc in column block */
    if ( fblok+1 < lblok )
    {
        pastix_int_t total = dimb;
        SolverBlok *blok   = fblok;
        while (total != 0){
            blok++;
            pastix_int_t dimb = blok->lrownum - blok->frownum + 1;
            total -= dimb;

            /* Update the data blok */
            fU = U + blok->coefind;

            double mem_dense = dima*dimb*8./1000000.;
            pastix_int_t rank = blok->rankU;
            if (rank != -1){
                /* printf("Uncompress U block\n"); */
                pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
                pastix_complex64_t *u = blok->coefU_u_LR;
                pastix_complex64_t *v = blok->coefU_v_LR;

                z_uncompress_LR(fU, dima, dimb,
                                u, dimb,
                                v, dima,
                                stride, rank);

                double mem_SVD   = rank*(dima+dimb)*8./1000000.;
                if (mem_SVD < mem_dense)
                    gain_U += mem_dense - mem_SVD;
            }


            fL = L + blok->coefind;

            rank = blok->rankL;
            if (rank != -1){
                /* printf("Uncompress L block\n"); */
                pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
                pastix_complex64_t *u = blok->coefL_u_LR;
                pastix_complex64_t *v = blok->coefL_v_LR;

                z_uncompress_LR(fL, dima, dimb,
                                u, dimb,
                                v, dima,
                                stride, rank);

                double mem_SVD   = rank*(dima+dimb)*8./1000000.;
                if (mem_SVD < mem_dense)
                    gain_L += mem_dense - mem_SVD;
            }
        }
    }

    return PASTIX_SUCCESS;
}

int core_zgetrfsp1d_trsm2( SolverCblk         *cblk,
                           pastix_complex64_t *L,
                           pastix_complex64_t *U)
{
    (void)U;
    SolverBlok *fblok, *lblok;
    pastix_complex64_t *fL;
    pastix_int_t dima, dimb, stride;

    dima    = cblk->lcolnum - cblk->fcolnum + 1;
    stride  = cblk->stride;
    fblok = cblk->fblokptr;   /* this diagonal block */
    lblok = cblk[1].fblokptr; /* the next diagonal block */

    /* vertical dimension */
    dimb = stride - dima;
    (void) dimb;

    /* if there is an extra-diagonal bloc in column block */
    if ( fblok+1 < lblok )
    {
        /* first extra-diagonal bloc in column block address */
        fL = L + fblok[1].coefind;
        (void) fL;

        /* Solve on L */
#if defined(PASTIX_WITH_HODLR)
        if (cblk->is_HODLR == 1){
            int rank;
            gain_L += compress_SVD(fL, dima, dimb, &rank);
        }
#endif /* defined(PASTIX_WITH_HODLR) */

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
    pastix_complex64_t *D = cblk->dcoeftab;
    pastix_int_t nbpivot  = core_zgetrfsp1d_getrf(cblk, D, NULL, criteria);

    /* Compress + TRSM U */
    core_zgetrfsp1d_trsm(cblk, L, U);
    core_zgetrfsp1d_trsm2(cblk, L, U);

    /* Compress L */
#if defined(PASTIX_WITH_HODLR)
    if (cblk->is_HODLR)
        core_zgetrfsp1d_trsm2(cblk, L, U);
#endif /* defined(PASTIX_WITH_HODLR) */

    return nbpivot;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgetrfsp1d_gemm - Computes the Cholesky factorization of one panel and apply
 * all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          The pointer to the data structure that describes the panel from
 *          which we compute the contributions. Next column blok must be
 *          accessible through cblk[1].
 *
 * @param[in] blok
 *          The pointer to the data structure that describes the blok from which
 *          we compute the contributions.
 *
 * @param[in] fcblk
 *          The pointer to the data structure that describes the panel on
 *          which we compute the contributions. Next column blok must be
 *          accessible through fcblk[1].
 *
 * @param[in,out] L
 *          The pointer to the lower matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] U
 *          The pointer to the upper matrix storing the coefficients of the
 *          panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] Cl
 *          The pointer to the lower matrix storing the coefficients of the
 *          updated panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in,out] Cu
 *          The pointer to the upper matrix storing the coefficients of the
 *          updated panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in] work
 *          Temporary memory buffer.
 *
 *******************************************************************************
 *
 * @return
 *          The number of static pivoting during factorization of the diagonal block.
 *
 *******************************************************************************/
void core_zgetrfsp1d_gemm( SolverCblk         *cblk,
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
    pastix_complex64_t *wtmp;
    pastix_int_t stride, stridefc, indblok;
    pastix_int_t dimi, dimj, dima, dimb;

    stride  = cblk->stride;
    dima = cblk->lcolnum - cblk->fcolnum + 1;

    pastix_complex64_t *Cd = fcblk->dcoeftab;
    pastix_int_t stride_D  = fcblk->lcolnum - fcblk->fcolnum + 1;

    /* First blok */
    indblok = blok->coefind;

    dimj = blok->lrownum - blok->frownum + 1;
    dimi = stride - indblok;

    /* Matrix A = Aik */
    Aik = L + indblok;
    Akj = U + indblok;

    /*
     * Compute update on L
     */
    wtmp = work;
    cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                 dimi, dimj, dima,
                 CBLAS_SADDR(zone),  Aik,  stride,
                                     Akj,  stride,
                 CBLAS_SADDR(zzero), wtmp, dimi  );

    /*
     * Add contribution to facing cblk
     */

    /* Get the first block of the distant panel */
    fblok = fcblk->fblokptr;

    /* Move the pointer to the top of the right column */
    stridefc = fcblk->stride;
    C = Cl + (blok->frownum - fcblk->fcolnum) * stridefc;

    lblok = cblk[1].fblokptr;

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

        /* If the blok modifies a diagonal block */
        if (fblok->coefind + iterblok->frownum - fblok->frownum < stride_D){

            char *lvls             = getenv("LEVELS");
            pastix_int_t levels    = atoi(lvls);
            char *splt             = getenv("SPLITSIZE");
            pastix_int_t split     = atoi(splt);
            char *thr              = getenv("THRESHOLD");
            pastix_int_t threshold = atoi(thr);
            char *tree             = getenv("HODLRTREE");
            pastix_int_t hodlrtree = atoi(tree);
            char *comp             = getenv("COMPRESS");
            pastix_int_t compress  = atoi(comp);

            (void) levels;
            (void) split;
            (void) threshold;
            (void) hodlrtree;
            (void) compress;

#if defined(PASTIX_WITH_HODLR)
            /* COMPRESS / UNCOMPRESS */
            /* pastix_int_t local_level = compute_cblklevel(current_cblk); */

            pastix_int_t n = fcblk->lcolnum - fcblk->fcolnum + 1;
            if (compress == 1 && n > split){
                pastix_int_t i;
                pastix_int_t  nb_parts = fcblk->nb_parts;
                pastix_int_t *parts    = fcblk->parts;

                pastix_int_t current = 0;
                while (parts[current+1] < (iterblok->frownum - fblok->frownum)){
                    current += 2;
                }

                while (parts[current] <= (iterblok->lrownum - fblok->frownum)
                       && current < 2*nb_parts){
                    pastix_int_t start, end;
                    start = pastix_imax(parts[current]   - iterblok->frownum + fblok->frownum, 0);
                    end   = pastix_imin(parts[current+1] - iterblok->frownum + fblok->frownum,
                                        iterblok->lrownum - iterblok->frownum);

                    Aij = Cd + (blok->frownum - fcblk->fcolnum) * stride_D
                        + fblok->coefind + iterblok->frownum - fblok->frownum;

                    core_zgeadd( CblasNoTrans, end-start+1, dimj, -1.0,
                                 wtmp + start, dimi,
                                 Aij  + start, stride_D );
                    current+=2;

                    fcblk->nb_contributions++;
                    fcblk->surface += (end-start+1) * dimj;
                }
            }

            /* Contribution to a non-HODLR structure */
            else
#endif /* defined(PASTIX_WITH_HODLR) */
            {
                Aij = Cd + (blok->frownum - fcblk->fcolnum) * stride_D
                    + fblok->coefind + iterblok->frownum - fblok->frownum;
                core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                             wtmp, dimi,
                             Aij,  stride_D );
            }
        }

        /* If the block modifies an off-diagonal blok */
        else{

            Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;
            dimb = iterblok->lrownum - iterblok->frownum + 1;
            dimj = blok->lrownum - blok->frownum + 1;

            core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                         wtmp, dimi,
                         Aij,  stridefc );

            /* No more LR, we received a dense contribution */
            fblok->rankU = -1;
        }

        wtmp += dimb;

        pastix_cblk_unlock( fcblk );
    }

    /*
     * Compute update on U
     */
    Aik = U + indblok;
    Akj = L + indblok;

    /* Restore data */
    wtmp  = work;
    fblok = fcblk->fblokptr;

    cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                 dimi, dimj, dima,
                 CBLAS_SADDR(zone),  Aik,  stride,
                                     Akj,  stride,
                 CBLAS_SADDR(zzero), wtmp, dimi  );

    wtmp += blok->lrownum - blok->frownum + 1;

    /*
     * Add contribution to facing cblk
     */
    C  = Cd + (blok->frownum - fcblk->fcolnum);
    Cu = Cu + (blok->frownum - fcblk->fcolnum) * stridefc;

    /*
     * Update on L part (blocks facing diagonal block)
     */
    for (iterblok=blok+1; iterblok<lblok; iterblok++) {

        /* Find facing bloknum */
        if (!is_block_inside_fblock( iterblok, fblok ))
             break;

        Aij = C + (iterblok->frownum - fblok->frownum)*stride_D;
        dimb = iterblok->lrownum - iterblok->frownum + 1;

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasTrans, dimj, dimb, -1.0,
                     wtmp, dimi,
                     Aij,  stride_D);
        pastix_cblk_unlock( fcblk );

        /* Displacement to next block */
        wtmp += dimb;
    }

    C = Cu;

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

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                     wtmp, dimi,
                     Aij,  stridefc );
        pastix_cblk_unlock( fcblk );

        /* Displacement to next block */
        wtmp += dimb;
    }
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

    pastix_complex64_t *Cd = fcblk->dcoeftab;
    pastix_int_t stride_D  = fcblk->lcolnum - fcblk->fcolnum + 1;

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

            /* Uncompress U and L factors if those blocks are compressed */
            pastix_int_t rank = blok->rankU;
            if (rank != -1){
                pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
                pastix_complex64_t *u = blok->coefU_u_LR;
                pastix_complex64_t *v = blok->coefU_v_LR;

                z_uncompress_LR(Akj, dima, dimb,
                                u, dimb,
                                v, dima,
                                cblk->stride, rank);
            }

            rank = iterblok->rankL;
            if (rank != -1){
                pastix_int_t dimb     = iterblok->lrownum - iterblok->frownum + 1;
                pastix_complex64_t *u = iterblok->coefL_u_LR;
                pastix_complex64_t *v = iterblok->coefL_v_LR;

                z_uncompress_LR(Aik, dima, dimb,
                                u, dimb,
                                v, dima,
                                cblk->stride, rank);
            }

            cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                         dimb, dimj, dima,
                         CBLAS_SADDR(zone),  Aik,  stride,
                         Akj,  stride,
                         CBLAS_SADDR(zzero), work, dimi  );

            Aij = Cd + (blok->frownum - fcblk->fcolnum) * stride_D
                + fblok->coefind + iterblok->frownum - fblok->frownum;
            core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                         work, dimi,
                         Aij,  stride_D );
        }

        /* If the block modifies an off-diagonal blok */
        else{
            /* Aik for iterblok */
            /* Akj for blok */
            pastix_complex64_t *uL, *vL, *uU, *vU, *uF, *vF;
            pastix_int_t rankL, rankU, rankF;

            rankL = iterblok->rankL;
            rankU = blok->rankU;
            rankF = fblok->rankL;

            uL = iterblok->coefL_u_LR;
            vL = iterblok->coefL_v_LR;
            uU = blok->coefU_u_LR;
            vU = blok->coefU_v_LR;
            uF = fblok->coefL_u_LR;
            vF = fblok->coefL_v_LR;

            if (rankL != -1 && rankU != -1 && rankF != -1){

                /* printf("Simple operation LR <- LR * LR %ld %ld %ld\n", dima, dimb, dimj); */
                /* printf("Decalage %ld %ld on blok %ld %ld contrib %ld %ld\n", */
                /*        iterblok->frownum - fblok->frownum, */
                /*        blok->frownum - fcblk->fcolnum, */
                /*        sizeF, stride_D, */
                /*        dimb, dimj); */

                pastix_complex64_t *tmp;
                tmp = malloc(dimi * dimi * sizeof(pastix_complex64_t *));
                memset(tmp, 0, dimi * dimi * sizeof(pastix_complex64_t *));

                cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                             rankL, rankU, dima,
                             CBLAS_SADDR(zone),  vL,   dima,
                             vU,   dima,
                             CBLAS_SADDR(zzero), work, dimi );

                cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                             rankL, dimj, rankU,
                             CBLAS_SADDR(zone),  work, dimi,
                             uU,   dimj,
                             CBLAS_SADDR(zzero), tmp,  dimi );

                pastix_int_t sizeF = fblok->lrownum - fblok->frownum + 1;
                pastix_int_t ldv2  = dimi;
                pastix_int_t ret   = -1;

                ret = z_add_LR(uF, vF,
                               uL, tmp,
                               sizeF, stride_D, rankF,
                               dimb, dimj, rankL, ldv2,
                               iterblok->frownum - fblok->frownum,
                               blok->frownum - fcblk->fcolnum);

                if (ret == -1){
                    cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                                 dimb, dimj, rankL,
                                 CBLAS_SADDR(zone),  uL,   dimb,
                                 tmp,  dimi,
                                 CBLAS_SADDR(zzero), work, dimi  );

                    /* Uncompress: dense contribution */
                    if (fblok->rankL != -1){
                        Aij = Cl + fblok->coefind;
                        z_uncompress_LR(Aij, stride_D, sizeF,
                                        uF, sizeF,
                                        vF, stride_D,
                                        stridefc, fblok->rankL);
                        fblok->rankL = -1;
                    }
                    Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;

                    core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                                 work, dimi,
                                 Aij,  stridefc );

                    fblok->rankL = -1;
                }
                else{
                    fblok->rankL = ret;
                }
                free(tmp);
            }
            else{

                /* Uncompress U factor */
                pastix_int_t rank = blok->rankU;
                if (rank != -1){
                    pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
                    pastix_complex64_t *u = blok->coefU_u_LR;
                    pastix_complex64_t *v = blok->coefU_v_LR;

                    z_uncompress_LR(Akj, dima, dimb,
                                    u, dimb,
                                    v, dima,
                                    cblk->stride, rank);
                }

                /* Uncompress L factor */
                rank = iterblok->rankL;
                if (rank != -1){
                    pastix_int_t dimb     = iterblok->lrownum - iterblok->frownum + 1;
                    pastix_complex64_t *u = iterblok->coefL_u_LR;
                    pastix_complex64_t *v = iterblok->coefL_v_LR;

                    z_uncompress_LR(Aik, dima, dimb,
                                    u, dimb,
                                    v, dima,
                                    cblk->stride, rank);
                }

                /* Uncompress because we receive a dense contribution */
                if (fblok->rankL != -1){
                    pastix_int_t dimb = fblok->lrownum - fblok->frownum + 1;
                    Aij = Cl + fblok->coefind;

                    z_uncompress_LR(Aij, stride_D, dimb,
                                    uF, dimb,
                                    vF, stride_D,
                                    stridefc, fblok->rankL);
                    fblok->rankL = -1;
                }

                cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                             dimb, dimj, dima,
                             CBLAS_SADDR(zone),  Aik,  stride,
                             Akj,  stride,
                             CBLAS_SADDR(zzero), work, dimi  );

                Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;

                core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                             work, dimi,
                             Aij,  stridefc );
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
    Cu = Cu + (blok->frownum - fcblk->fcolnum) * stridefc;

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

        /* Uncompress, we are doing a dense operations */
        pastix_int_t rank = iterblok->rankU;
        if (rank != -1){
            pastix_int_t dimb     = iterblok->lrownum - iterblok->frownum + 1;
            pastix_complex64_t *u = iterblok->coefU_u_LR;
            pastix_complex64_t *v = iterblok->coefU_v_LR;

            z_uncompress_LR(Aik, dima, dimb,
                            u, dimb,
                            v, dima,
                            cblk->stride, rank);
        }

        rank = blok->rankL;
        if (rank != -1){
            pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
            pastix_complex64_t *u = blok->coefL_u_LR;
            pastix_complex64_t *v = blok->coefL_v_LR;

            z_uncompress_LR(Akj, dima, dimb,
                            u, dimb,
                            v, dima,
                            cblk->stride, rank);
        }

        cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                     dimb, dimj, dima,
                     CBLAS_SADDR(zone),  Aik,  stride,
                     Akj,  stride,
                     CBLAS_SADDR(zzero), work, dimi  );

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasTrans, dimj, dimb, -1.0,
                     work, dimi,
                     Aij,  stride_D);
        pastix_cblk_unlock( fcblk );
    }

    C = Cu;

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

        /* Uncompress, we are doing a dense operations */
        pastix_int_t rank = iterblok->rankU;
        if (rank != -1){
            pastix_int_t dimb     = iterblok->lrownum - iterblok->frownum + 1;
            pastix_complex64_t *u = iterblok->coefU_u_LR;
            pastix_complex64_t *v = iterblok->coefU_v_LR;

            z_uncompress_LR(Aik, dima, dimb,
                            u, dimb,
                            v, dima,
                            cblk->stride, rank);
        }

        rank = blok->rankL;
        if (rank != -1){
            pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
            pastix_complex64_t *u = blok->coefL_u_LR;
            pastix_complex64_t *v = blok->coefL_v_LR;

            z_uncompress_LR(Akj, dima, dimb,
                            u, dimb,
                            v, dima,
                            cblk->stride, rank);
        }

        /* rank = fblok->rankU; */
        /* if (rank != -1){ */
        /*     pastix_int_t dima     = fcblk->lcolnum - fcblk->fcolnum + 1; */
        /*     pastix_int_t dimb     = fblok->lrownum - fblok->frownum + 1; */
        /*     pastix_complex64_t *u = fblok->coefU_u_LR; */
        /*     pastix_complex64_t *v = fblok->coefU_v_LR; */

        /*     Aij = C + fblok->coefind; */
        /*     z_uncompress_LR(Aij, dima, dimb, */
        /*                     u, dimb, */
        /*                     v, dima, */
        /*                     fcblk->stride, rank); */
        /* } */

        Aij = C + fblok->coefind + iterblok->frownum - fblok->frownum;
        cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                     dimb, dimj, dima,
                     CBLAS_SADDR(zone),  Aik,  stride,
                     Akj,  stride,
                     CBLAS_SADDR(zzero), work, dimi  );

        pastix_cblk_lock( fcblk );
        core_zgeadd( CblasNoTrans, dimb, dimj, -1.0,
                     work, dimi,
                     Aij,  stridefc );
        pastix_cblk_unlock( fcblk );

        fblok->rankU = -1;
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
 *          threshold, its value is replaced by the threshold and the nu,ber of
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

    printf("CBLK %ld STRIDE %ld\n", current_cblk, cblk->stride);
    nbpivot = core_zgetrfsp1d_panel(cblk, L, U, criteria);

    blok = cblk->fblokptr + 1; /* this diagonal block */
    lblk = cblk[1].fblokptr;   /* the next diagonal block */

    char *splt             = getenv("SPLITSIZE");
    pastix_int_t split     = atoi(splt);
    char *thr              = getenv("THRESHOLD");
    pastix_int_t threshold = atoi(thr);
    char *tree             = getenv("HODLRTREE");
    pastix_int_t hodlrtree = atoi(tree);

    (void) split;
    (void) threshold;
    (void) hodlrtree;

    blok = cblk->fblokptr+1;
    for( ; blok < lblk; blok++ )
    {
        fcblk = (solvmtx->cblktab + blok->fcblknm);

        core_zgetrfsp1d_gemm_LR( cblk, blok, fcblk,
                                 L, U, fcblk->lcoeftab, fcblk->ucoeftab, work );

    }

    /* Uncompress for sequential_ztrsm.c still implemented in dense fashion */
    core_zgetrfsp1d_uncompress(cblk, L, U);


#if defined(PASTIX_WITH_HODLR)
        pastix_int_t n = fcblk->lcolnum - fcblk->fcolnum + 1;
        if (n > split && fcblk->is_HODLR == 0){
            printf ("\033[34;01mFirst time we touch cblk of size %ld (contrib from %ld)\033[00m\n", n, current_cblk-1);

            pastix_int_t nb_parts;
            pastix_int_t *parts;
            cHODLR schur_H;

            if (fcblk->split_size == 0){
                /* printf ("\033[34;01mNo user tree used\033[00m\n"); */
                schur_H = newHODLR(n, n, fcblk->dcoeftab, threshold,
                                   "SVD", &nb_parts, &parts);
            }
            else{
                if (hodlrtree == 0){
                    /* printf ("\033[34;01mNo user tree used\033[00m\n"); */
                    schur_H = newHODLR(n, n, fcblk->dcoeftab, threshold,
                                       "SVD", &nb_parts, &parts);
                }
                else{
                    /* printf ("\033[32;01mUser tree with %ld parts used\033[00m\n", */
                    /*         fcblk->split_size); */
                    schur_H = newHODLR_Tree(n, n, fcblk->dcoeftab, threshold,
                                            "SVD", fcblk->split_size, fcblk->split,
                                            &nb_parts, &parts);
                }
            }

            fcblk->is_HODLR         = 1;
            fcblk->cMatrix          = schur_H;
            fcblk->nb_parts         = nb_parts;
            fcblk->parts            = parts;
            fcblk->nb_contributions = 0;
            fcblk->surface          = 0;
            /* delHODLR(schur_H); */
        }
#endif /* defined(PASTIX_WITH_HODLR) */

#if defined(PASTIX_WITH_HODLR)
    if (cblk->is_HODLR){
        printf(" %ld contributions, surface %ld \n",
               cblk->nb_contributions, cblk->surface);
    }
#endif /* defined(PASTIX_WITH_HODLR) */

    return nbpivot;
}

