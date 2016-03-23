/**
 *
 * @file core_zgelrops.c
 *
 *  PaStiX kernel routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Gr�goire Pichon
 * @date 2016-23-03
 * @precisions normal z -> c d s
 *
 **/
#include "common.h"
#include "pastix_zcores.h"
#include <cblas.h>
#include <lapacke.h>
#include "solver.h"

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t zzero =  0.;

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_z_compress_LR - Compresses a dense block into a u v^T LR structure.
 *
 *******************************************************************************
 *
 * @param[in] fL
 *          Pointer to the dense structure of size dimb * dima
 *          Leading dimension is stride
 *
 * @param[out] u
 *          Pointer to the u factor of LR representation of size dimb * rank
 *          Leading dimension is ldu
 *
 * @param[out] v
 *          Pointer to the v factor of LR representation of size dima * rank
 *          Leading dimension is ldv
 *          Note that due to LAPACKE_zgesvd this block is stored transposed
 *
 *
 *******************************************************************************
 *
 * @return
 *          The rank of the compressed structure.
 *
 *******************************************************************************/
pastix_int_t core_z_compress_LR(pastix_complex64_t *fL,
                                pastix_int_t stride,
                                pastix_int_t dimb,
                                pastix_int_t dima,
                                pastix_complex64_t *u,
                                pastix_int_t ldu,
                                pastix_complex64_t *v,
                                pastix_int_t ldv){

    pastix_complex64_t *block;
    pastix_int_t        ret;
    pastix_int_t        i;
    double             *s;
    double             *superb;

    pastix_int_t dim_min = dima;
    if (dimb < dim_min)
        dim_min = dimb;

    /* Note that we have to copy fL because LAPACKE_zgesvd erases the matrix */
    block  = malloc( dima * dimb * sizeof(pastix_complex64_t) );
    s      = malloc( dim_min * sizeof(double) );
    superb = malloc( dimb * sizeof(double) );

    for (i=0; i<dima; i++){
        memcpy( block + i * dimb, fL + i * stride, dimb * sizeof(pastix_complex64_t) );
    }

    ret = LAPACKE_zgesvd( CblasColMajor, 'S', 'S',
                          dimb, dima,
                          block, dimb,
                          s, u, ldu, v, ldv,
                          superb );

    if( ret != 0 ){
        printf("SVD FAILED %ld\n\n", ret);
        exit(1);
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

    for (i=0; i<rank; i++){
        cblas_dscal(dimb, s[i], &(u[i*ldu]), 1);
    }

    free(block);
    free(s);
    free(superb);
    return rank;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_z_uncompress_LR - Uncompresses a u v^T LR structure into a dense block.
 *
 *******************************************************************************
 *
 * @param[out] fL
 *          Pointer to the dense structure of size dimb * dima
 *          Leading dimension is stride
 *
 * @param[in] u
 *          Pointer to the u factor of LR representation of size dimb * rank
 *          Leading dimension is ldu
 *
 * @param[in] v
 *          Pointer to the v factor of LR representation of size dima * rank
 *          Leading dimension is ldv
 *          Note that due to LAPACKE_zgesvd this block is stored transposed
 *
 *
 *******************************************************************************
 *
 * @return
 *          The rank of the compressed structure.
 *
 *******************************************************************************/
void core_z_uncompress_LR(pastix_complex64_t *fL,
                          pastix_int_t stride,
                          pastix_int_t dimb,
                          pastix_int_t dima,
                          pastix_complex64_t *u,
                          pastix_int_t ldu,
                          pastix_complex64_t *v,
                          pastix_int_t ldv,
                          pastix_int_t rank){

    cblas_zgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                dimb, dima, rank,
                CBLAS_SADDR(zone),  u,  ldu,
                                    v,  ldv,
                CBLAS_SADDR(zzero), fL, stride);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_z_add_LR - Adds two LR structure u1 v1^T and (-u2) v2^T into u1 v1^T
 *
 *    u1v1^T + u2v2^T = (u1 u2) (v1 v2)^T
 *    Compute QR decomposition of (u1 u2) = Q1 R1
 *    Compute QR decomposition of (v1 v2) = Q2 R2
 *    Compute SVD of R1 R2^T = u \sigma v^T
 *    Final solution is (Q1 u \sigma^[1/2]) (Q2 v \sigma^[1/2])^T
 *
 *******************************************************************************
 *
 * @param[in, out] u1 v1
 *          LR structure where v1 is stored transposed
 *          u1 factor of size dim_u1 * rank_1 with ld_u1 as leading dimension
 *          v1 factor of size dim_v1 * rank_1 with ld_v1 as leading dimension
 *
 * @param[in] u2 v2
 *          Pointer to the u factor of LR representation of size dimb * rank
 *          Leading dimension is ldu
 *
 * @param[in] x2, y2
 *          Position where u2 v2 is added into u1 v1 (which is larger)
 *
 *
 *******************************************************************************
 *
 * @return
 *          The new rank of u1 v1^T or -1 if ranks are too large for recompression
 *
 *******************************************************************************/
pastix_int_t core_z_add_LR(pastix_complex64_t *u1,
                           pastix_complex64_t *v1,
                           pastix_int_t dim_u1,
                           pastix_int_t dim_v1,
                           pastix_int_t rank_1,
                           pastix_int_t ld_u1,
                           pastix_int_t ld_v1,
                           pastix_complex64_t *u2,
                           pastix_complex64_t *v2,
                           pastix_int_t dim_u2,
                           pastix_int_t dim_v2,
                           pastix_int_t rank_2,
                           pastix_int_t ld_u2,
                           pastix_int_t ld_v2,
                           pastix_int_t x2,
                           pastix_int_t y2){

    /* Unused parameters right now */
    (void)ld_u1;
    (void)ld_v1;

    pastix_int_t dim_u = pastix_imax(dim_u1, dim_u2);
    pastix_int_t dim_v = pastix_imax(dim_v1, dim_v2);
    pastix_int_t rank  = rank_1 + rank_2;
    pastix_int_t i, j;

    if (dim_u2 > dim_u1 || dim_v2 > dim_v1){
        printf("nDimensions are not correct\n");
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

    pastix_complex64_t *u1u2, *v1v2;
    u1u2 = malloc( dim_u * rank * sizeof(pastix_complex64_t));
    v1v2 = malloc( dim_v * rank * sizeof(pastix_complex64_t));

    /* Complete with zeroes if adding a small block in a larger structure */
    if (dim_u1 != dim_u2)
        memset(u1u2 + dim_u1 * rank_1, 0, dim_u1 * rank_2 * sizeof(pastix_complex64_t));
    if (dim_v1 != dim_v2)
        memset(v1v2, 0, dim_v * rank * sizeof(pastix_complex64_t));

    memcpy(u1u2, u1, dim_u1 * rank_1 * sizeof(pastix_complex64_t));
    for (i=0; i<rank_2; i++){
        memcpy(u1u2 + dim_u * (rank_1 + i) + x2, u2 + i * ld_u2, dim_u2 * sizeof(pastix_complex64_t));
    }

    for (i=0; i<dim_v1; i++){
        for (j=0; j<rank_1; j++){
            v1v2[dim_v * j + i]                 = v1[i * dim_v1 + j];
        }
    }

    /* WARNING: minus because of the extend add */
    for (i=0; i<dim_v2; i++){
        for (j=0; j<rank_2; j++){
            v1v2[dim_v * (rank_1 + j) + i + y2] = -v2[i * ld_v2 + j];
        }
    }

    pastix_int_t ret;
    pastix_complex64_t *tau1 = malloc( minMN_1 * sizeof(pastix_complex64_t));
    ret = LAPACKE_zgeqrf( CblasColMajor, dim_u, rank,
                          u1u2, dim_u, tau1 );


    pastix_complex64_t *tau2 = malloc( minMN_2 * sizeof(pastix_complex64_t));
    ret = LAPACKE_zgeqrf( CblasColMajor, dim_v, rank,
                          v1v2, dim_v, tau2 );

    pastix_complex64_t *R1, *R2, *R;
    R1 = malloc(rank * rank * sizeof(pastix_complex64_t));
    R2 = malloc(rank * rank * sizeof(pastix_complex64_t));
    R  = malloc(rank * rank * sizeof(pastix_complex64_t));
    memset(R1, 0, rank * rank * sizeof(pastix_complex64_t));
    memset(R2, 0, rank * rank * sizeof(pastix_complex64_t));

    for (i=0; i<rank; i++){
        memcpy(R1 + rank * i, u1u2 + dim_u * i, (i+1) * sizeof(pastix_complex64_t));
        memcpy(R2 + rank * i, v1v2 + dim_v * i, (i+1) * sizeof(pastix_complex64_t));
    }

    /* Compute R1 R2^T */
    cblas_zgemm(CblasColMajor, CblasNoTrans, CblasTrans,
                rank, rank, rank,
                CBLAS_SADDR(zone),  R1, rank,
                                    R2, rank,
                CBLAS_SADDR(zzero), R,  rank);

    double *superb;
    double *s;
    pastix_complex64_t *u, *v;
    s = malloc( rank * sizeof(double));
    u = malloc( rank * rank * sizeof(pastix_complex64_t));
    v = malloc( rank * rank * sizeof(pastix_complex64_t));
    superb = malloc( rank * sizeof(double));

    ret = LAPACKE_zgesvd( CblasColMajor, 'A', 'A',
                          rank, rank, R, rank,
                          s, u, rank, v, rank, superb );

    if (ret != 0){
        printf("LAPACKE_zgesvd FAILED\n");
        exit(1);
    }

    pastix_int_t new_rank = rank;
    char *tol             = getenv("TOLERANCE");
    double tolerance      = atof(tol);

    for (i=0; i<rank-1; i++){
        if (s[i] / s[0] < tolerance){
            new_rank = i+1;
            break;
        }
    }

    /* Scal u as before to take into account singular values */
    for (i=0; i<rank; i++){
        cblas_dscal(rank, s[i], &(u[rank * i]), 1);
    }

    for (i=0; i<rank; i++){
        memcpy(u1 + dim_u * i, u + rank * i, rank * sizeof(pastix_complex64_t));
        memset(u1 + dim_u * i + rank, 0, (dim_u1 - rank) * sizeof(pastix_complex64_t));
    }

    /* We need non-transposed version of v */
    pastix_complex64_t *v3;
    v3 = malloc( dim_v * rank * sizeof(pastix_complex64_t) );

    for (i=0; i<rank; i++){
        for (j=0; j<rank; j++){
            v3[dim_v * j + i] = v[rank * i + j];
        }
    }
    for (j=0; j<rank; j++){
        memset(v3 + dim_v * j + rank, 0, (dim_v - rank) * sizeof(pastix_complex64_t));
    }

    ret = LAPACKE_dormqr(CblasColMajor, 'L', 'N',
                         dim_u, rank, minMN_1,
                         u1u2, dim_u, tau1,
                         u1, dim_u1);

    ret = LAPACKE_dormqr(CblasColMajor, 'L', 'N',
                         dim_v, rank, minMN_2,
                         v1v2, dim_v, tau2,
                         v3, dim_v1);


    for (i=0; i<rank; i++){
        for (j=0; j<dim_v1; j++){
            v1[dim_v1 * j + i] = v3[dim_v1 * i + j];
        }
    }

    /* printf("Rank was OK to add the two LR structures %ld %ld %ld %ld\n", */
    /*        dim_u, rank_1, rank_2, new_rank); */

    free(u1u2);
    free(v1v2);
    free(u);
    free(s);
    free(v);
    free(superb);
    free(tau1);
    free(tau2);
    free(R);
    free(R1);
    free(R2);
    free(v3);
    return new_rank;
}


void core_z_lr2dense(SolverBlok *blok,
                     pastix_complex64_t *A,
                     pastix_int_t stride,
                     pastix_int_t width,
                     pastix_int_t side){

    switch (side){
    case L_side:
        if (blok->rankL != -1){
            pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
            pastix_complex64_t *u = blok->coefL_u_LR;
            pastix_complex64_t *v = blok->coefL_v_LR;

            core_z_uncompress_LR(A, stride,
                                 dimb, width,
                                 u, dimb,
                                 v, width,
                                 blok->rankL);
        }
        break;
    case U_side:
        if (blok->rankU != -1){
            pastix_int_t dimb     = blok->lrownum - blok->frownum + 1;
            pastix_complex64_t *u = blok->coefU_u_LR;
            pastix_complex64_t *v = blok->coefU_v_LR;

            core_z_uncompress_LR(A, stride,
                                 dimb, width,
                                 u, dimb,
                                 v, width,
                                 blok->rankU);
        }
        break;
    default:
        printf("Wrong operation\n");
        exit(1);
        break;
    }
}

void core_zproduct_lr(SolverBlok *blok1,
                      pastix_complex64_t *A1,
                      pastix_int_t stride1,
                      pastix_int_t width1,
                      pastix_int_t side1,
                      SolverBlok *blok2,
                      pastix_complex64_t *A2,
                      pastix_int_t stride2,
                      pastix_int_t width2,
                      pastix_int_t side2,
                      pastix_complex64_t *work,
                      pastix_int_t ldwork){

    assert(width1 == width2);

    pastix_int_t dimb = blok1->lrownum - blok1->frownum + 1;
    pastix_int_t dimj = blok2->lrownum - blok2->frownum + 1;
    pastix_int_t dima = width1;

    core_z_lr2dense(blok1, A1, stride1,
                    width1, side1);

    core_z_lr2dense(blok2, A2, stride2,
                    width2, side2);

    cblas_zgemm( CblasColMajor, CblasNoTrans, CblasTrans,
                 dimb, dimj, dima,
                 CBLAS_SADDR(zone),  A1, stride1,
                                     A2, stride2,
                 CBLAS_SADDR(zzero), work, ldwork  );
}
