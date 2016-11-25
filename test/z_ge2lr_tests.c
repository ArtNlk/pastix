/**
 *
 * @file z_ge2lr_tests.c
 *
 * Tests and validate the Xge2lr routine.
 *
 * @version 5.1.0
 * @author Gregoire Pichon
 * @date 2016-11-24
 *
 * @precisions normal z -> c d s
 *
 **/
#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pastix.h>
#include "../common/common.h"
#include <lapacke.h>
#include <cblas.h>
#include "../blend/solver.h"
#include "../kernels/pastix_zcores.h"

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
z_ge2lr_test( double tolerance, pastix_int_t rank,
              pastix_int_t m, pastix_int_t n, pastix_int_t lda )
{

    pastix_complex64_t *A, *A_RRQR, *A_SVD;
    pastix_lrblock_t    LR_RRQR, LR_SVD;

    double norm_dense, norm_LR_RRQR, norm_LR_SVD;
    double norm_diff_RRQR, norm_diff_SVD;
    double res_SVD, res_RRQR;

    pastix_int_t minMN = pastix_imin(m, n);
    int mode           = 0;
    double rcond       = (double) minMN;
    double dmax        = 1.0;
    int ISEED[4]       = {0,0,0,1};   /* initial seed for zlarnv() */

    pastix_complex64_t *work;
    double *S;

    double alpha;

    MALLOC_INTERN(A,      n * lda, pastix_complex64_t);
    MALLOC_INTERN(A_RRQR, n * lda, pastix_complex64_t);
    MALLOC_INTERN(A_SVD,  n * lda, pastix_complex64_t);

    MALLOC_INTERN(S, minMN, double);
    MALLOC_INTERN(work, 3*pastix_imax(m, n), pastix_complex64_t);

    if ((!A)||(!A_SVD)||(!A_RRQR)||(!S)||(!work)){
        printf("Out of Memory \n ");
        return -2;
    }

    if (lda < m || lda < n){
        printf("Invalid lda parameter\n");
        return -3;
    }

    /* Chose alpha such that alpha^rank = tolerance */
    alpha = exp(log(tolerance) / rank);

    if (mode == 0){
        pastix_int_t i;
        S[0] = 1;
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
    core_zge2lr_RRQR( tolerance,
                      m, n,
                      A, lda,
                      &LR_RRQR );

    core_zge2lr_SVD( tolerance,
                      m, n,
                      A, lda,
                      &LR_SVD );

    core_zlr2ge( m, n,
                 &LR_RRQR,
                 A_RRQR, lda );

    core_zlr2ge( m, n,
                 &LR_SVD,
                 A_SVD, lda );

    printf(" The rank of A is: RRQR %d SVD %d\n", LR_RRQR.rk, LR_SVD.rk);

    /* Compute norm of dense and LR matrices */
    norm_LR_RRQR = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                        A_RRQR, lda, NULL );
    norm_LR_SVD  = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                        A_SVD, lda, NULL );

    core_zgeadd( PastixNoTrans, m, n,
                 -1., A, lda,
                  1., A_RRQR, lda );

    core_zgeadd( PastixNoTrans, m, n,
                 -1., A, lda,
                  1., A_SVD, lda );

    norm_diff_RRQR = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                          A_RRQR, lda, NULL );

    norm_diff_SVD = LAPACKE_zlange_work( LAPACK_COL_MAJOR, 'f', m, n,
                                         A_SVD, lda, NULL );

    res_RRQR = norm_diff_RRQR / ( tolerance * norm_dense );
    res_SVD  = norm_diff_SVD  / ( tolerance * norm_dense );

    memFree_null(A);
    memFree_null(A_SVD);
    memFree_null(A_RRQR);
    memFree_null(S);
    memFree_null(work);

    if ((res_RRQR < 10) && (res_SVD < 10) && (LR_RRQR.rk >= LR_SVD.rk))
        return 0;
    return 1;
}

int main (int argc, char **argv)
{
    int err = 0;
    int ret;
    pastix_int_t m, r;
    double tolerance = 0.001;

    m = 500;
    for (r=10; r<50; r+=10){
        printf("   -- Test GE2LR M=N=LDA=%ld R=%ld\n", m, r);

        ret = z_ge2lr_test(tolerance, r, m, m, m);
        PRINT_RES(ret);
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
