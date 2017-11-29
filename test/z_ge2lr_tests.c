/**
 *
 * @file z_ge2lr_tests.c
 *
 * Tests and validate the Xge2lr routine.
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Gregoire Pichon
 * @date 2016-11-24
 *
 * @precisions normal z -> c d s
 *
 **/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pastix.h>
#include "common/common.h"
#include <lapacke.h>
#include <cblas.h>
#include "blend/solver.h"
#include "kernels/pastix_zcores.h"
#include "kernels/pastix_zlrcores.h"

#define PRINT_RES(_ret_)                        \
    if(_ret_ == -1) {                           \
        printf("UNDEFINED\n");                  \
    }                                           \
    else if(_ret_ > 0) {                        \
        printf("FAILED(%d)\n", _ret_);          \
        err++;                                  \
    }                                           \
    else {                                      \
        printf("SUCCESS\n");                    \
    }

int
z_ge2lr_test( int mode, double tolerance, pastix_int_t rank,
              pastix_int_t m, pastix_int_t n, pastix_int_t lda )
{

    pastix_complex64_t *A, *A_RRQR, *A_SVD;
    pastix_lrblock_t    LR_RRQR, LR_SVD;

    double norm_dense;
    double norm_diff_RRQR, norm_diff_SVD;
    double res_SVD, res_RRQR;

    pastix_int_t minMN    = pastix_imin(m, n);
    pastix_int_t rankmax  = core_get_rklimit(m, n);
    double       rcond    = (double) minMN;
    double       dmax     = 1.0;
    int          ISEED[4] = {0,0,0,1};   /* initial seed for zlarnv() */
    int          rc = 0;
    pastix_complex64_t *work;
    double *S;

    double alpha;

    if (lda < m || lda < n){
        printf("Invalid lda parameter\n");
        return -3;
    }

    A      = malloc(n * lda * sizeof(pastix_complex64_t));
    A_RRQR = malloc(n * lda * sizeof(pastix_complex64_t));
    A_SVD  = malloc(n * lda * sizeof(pastix_complex64_t));

    S    = malloc(minMN * sizeof(double));
    work = malloc(3 * pastix_imax(m, n)* sizeof(pastix_complex64_t));

    if ((!A)||(!A_SVD)||(!A_RRQR)||(!S)||(!work)){
        printf("Out of Memory \n ");
        free(A); free(A_RRQR); free(A_SVD); free(S); free(work);
        return -2;
    }

    /*
     * Choose alpha such that alpha^rank = tolerance
     */
    alpha = exp(log(tolerance) / rank);

    if (mode == 0) {
        pastix_int_t i;
        S[0] = 1;

        if (rank == 0) {
            S[0] = 0.;
        }
        for (i=1; i<minMN; i++){
            S[i] = S[i-1] * alpha;
        }
    }

    /* Initialize A */
    LAPACKE_zlatms_work( LAPACK_COL_MAJOR, m, n,
                         'U', ISEED,
                         'N', S, mode, rcond,
                         dmax, m, n,
                         'N', A, lda, work );

    norm_dense = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                      A, lda, NULL );

    /* Compress and then uncompress  */
    core_zge2lr_rrqr( tolerance, -1,
                      m, n,
                      A, lda,
                      &LR_RRQR );

    core_zge2lr_svd( tolerance, -1,
                      m, n,
                      A, lda,
                      &LR_SVD );

    core_zlr2ge( PastixNoTrans, m, n,
                 &LR_RRQR,
                 A_RRQR, lda );

    core_zlr2ge( PastixNoTrans, m, n,
                 &LR_SVD,
                 A_SVD, lda );

    printf(" The rank of A is: RRQR %d SVD %d rkmax %d\n", LR_RRQR.rk, LR_SVD.rk, (int)rankmax);

    core_zgeadd( PastixNoTrans, m, n,
                 -1., A,      lda,
                  1., A_RRQR, lda );

    core_zgeadd( PastixNoTrans, m, n,
                 -1., A,     lda,
                  1., A_SVD, lda );

    norm_diff_RRQR = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                          A_RRQR, lda, NULL );

    norm_diff_SVD = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                         A_SVD, lda, NULL );

    if (rank != 0){
        res_RRQR = norm_diff_RRQR / ( tolerance * norm_dense );
        res_SVD  = norm_diff_SVD  / ( tolerance * norm_dense );
    }
    else{
        res_RRQR = norm_diff_RRQR;
        res_SVD  = norm_diff_SVD;
    }

    free(A);
    free(A_SVD);
    free(A_RRQR);
    free(S);
    free(work);

    /*
     * Check the validity of the results
     */
    /* Check the correctness of the compression */
    if (res_RRQR > 10.0) {
        rc += 1;
    }
    if (res_SVD > 10.0) {
        rc += 2;
    }

    /* Check that SVD rank is equal to the desired rank */
    if ( ((rank >  rankmax) && (LR_SVD.rk != -1  )) ||
         ((rank <= rankmax) && ((LR_SVD.rk < (rank-2)) || (LR_SVD.rk > (rank+2)))) )
    {
        rc += 4;
    }

    /* Check that RRQR rank is larger or equal to SVD rank */
    if (LR_SVD.rk == -1) {
        if (LR_RRQR.rk != -1) {
            rc += 8;
        }
    }
    else {
        if ( (LR_RRQR.rk != -1) &&
             ((LR_RRQR.rk < LR_SVD.rk) || (LR_RRQR.rk > (LR_SVD.rk + 1.25 * rank ))) )
        {
            rc += 16;
        }
    }
    return rc;
}

int main (int argc, char **argv)
{
    (void) argc;
    (void) argv;
    int err = 0;
    int ret;
    pastix_int_t m, r;
    double tolerance = 1.e-3;

    for (m=100; m<300; m+=100){
        for (r=0; r <= (m/2); r += ( r + 1 ) ) {
            printf("   -- Test GE2LR M=N=LDA=%ld R=%ld\n", (long)m, (long)r);

            ret = z_ge2lr_test(0, tolerance, r, m, m, m);
            PRINT_RES(ret);
        }
    }

    if( err == 0 ) {
        printf(" -- All tests PASSED --\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf(" -- %d tests FAILED --\n", err);
        return EXIT_FAILURE;
    }

}
