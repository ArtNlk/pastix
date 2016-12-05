/**
 *
 * @file core_zgelrops.c
 *
 *  PaStiX kernel routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Gregoire Pichon
 * @date 2016-23-03
 * @precisions normal z -> c d s
 *
 **/
#include "common.h"
#include <cblas.h>
#include <lapacke.h>
#include "blend/solver.h"
#include "pastix_zcores.h"

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t zzero =  0.;

#define PASTIX_DEBUG_LR

//#define PASTIX_LR_CHECKNAN
#if defined(PASTIX_LR_CHECKNAN)
#define LAPACKE_zlacpy_work LAPACKE_zlacpy
#define LAPACKE_zlaset_work LAPACKE_zlaset

#define LAPACKE_zunmlq_work( _layout_, _side_, _trans_, _m_, _n_, _k_, _a_, _lda_, _tau_, _c_, _ldc_, _w_, _ldw_ ) \
    LAPACKE_zunmlq( _layout_, _side_, _trans_, _m_, _n_, _k_, _a_, _lda_, _tau_, _c_, _ldc_ )
#define LAPACKE_zunmqr_work( _layout_, _side_, _trans_, _m_, _n_, _k_, _a_, _lda_, _tau_, _c_, _ldc_, _w_, _ldw_ ) \
    LAPACKE_zunmqr( _layout_, _side_, _trans_, _m_, _n_, _k_, _a_, _lda_, _tau_, _c_, _ldc_ )

#define LAPACKE_zgeqrf_work( _layout_, _m_, _n_, _a_, _lda_, _tau_, _w_, _ldw_ ) \
    LAPACKE_zgeqrf( _layout_, _m_, _n_, _a_, _lda_, _tau_ )
#define LAPACKE_zgelqf_work( _layout_, _m_, _n_, _a_, _lda_, _tau_, _w_, _ldw_ ) \
    LAPACKE_zgelqf( _layout_, _m_, _n_, _a_, _lda_, _tau_ )

#if defined(PRECISION_z) || defined(PRECISION_c)
#define MYLAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_, _rw_ ) \
    LAPACKE_zgesvd( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, (double*)(_w_) )
#else
#define MYLAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_, _rw_ ) \
    LAPACKE_zgesvd( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, (double*)(_w_) )
#endif

#else

#if defined(PRECISION_z) || defined(PRECISION_c)
#define MYLAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_, _rw_ ) \
    LAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_, _rw_ )
#else
#define MYLAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_, _rw_ ) \
    LAPACKE_zgesvd_work( _layout_, _jobu_, jobv_, _m_, _n_, _a_, _lda_, _s_, _u_, _ldu_, _v_, _ldv_, _w_, _ldw_ )
#endif

#endif /* defined(PASTIX_LR_CHECKNAN) */

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_ztolerance - Compute the relative tolerance
 *
 *******************************************************************************
 *
 * @param[in] tol
 *          Absolute tolerance.
 *
 * @param[in] norm
 *          Norm of the matrix.
 *
 *******************************************************************************
 *
 * @return
 *          This routine will return the relative tolerance.
 *
 *******************************************************************************/
inline double
core_ztolerance(double tol, double norm)
{
    /* There is maybe an issue with rank-0 matrices */
    if (norm != 0.0)
        return tol * norm;
    return tol;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlralloc - Allocate a low-rank matrix.
 *
 *******************************************************************************
 *
 * @param[in] M
 *          Number of rows of the matrix A.
 *
 * @param[in] N
 *          Number of columns of the matrix A.
 *
 * @param[in] rkmax
 *         @arg -1   : the matrix is full
 *         @arg k>0  : the matrix is low-rank of rank rkmax
 *
 * @param[out] A
 *          The allocated low-rank matrix
 *
 *******************************************************************************/
void
core_zlralloc( pastix_int_t M, pastix_int_t N, pastix_int_t rkmax,
               pastix_lrblock_t *A )
{
    pastix_complex64_t *u, *v;

    if ( rkmax == -1 ) {
        u = malloc( M * N * sizeof(pastix_complex64_t) );
        memset(u, 0, M * N * sizeof(pastix_complex64_t) );
        A->rk = -1;
        A->rkmax = M;
        A->u = u;
        A->v = NULL;
    }
    else {
#if defined(PASTIX_DEBUG_LR)
        u = malloc( M * rkmax * sizeof(pastix_complex64_t) );
        v = malloc( N * rkmax * sizeof(pastix_complex64_t) );

        /* To avoid uninitialised values in valgrind. Lapacke doc (xgesvd) is not correct */
        memset(u, 0, M * rkmax * sizeof(pastix_complex64_t));
        memset(v, 0, N * rkmax * sizeof(pastix_complex64_t));
#else
        u = malloc( (M+N) * rkmax * sizeof(pastix_complex64_t));

        /* To avoid uninitialised values in valgrind. Lapacke doc (xgesvd) is not correct */
        memset(u, 0, (M+N) * rkmax * sizeof(pastix_complex64_t));

        v = u + M * rkmax;
#endif

        A->rk = 0;
        A->rkmax = rkmax;
        A->u = u;
        A->v = v;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrfree - Destroy a low-rank matrix.
 *
 *******************************************************************************
 * @param[in, out] A
 *          The low-rank matrix that will be desallocated.
 *
 *******************************************************************************/
void
core_zlrfree( pastix_lrblock_t *A )
{
    if ( A->rk == -1 ) {
        free(A->u);
        A->u = NULL;
    }
    else {
        free(A->u);
#if defined(PASTIX_DEBUG_LR)
        free(A->v);
#endif
        A->u = NULL;
        A->v = NULL;
    }
    A->rk = 0;
    A->rkmax = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrsze - Resizes a low-rank matrix
 *
 *******************************************************************************
 *
 * @param[in] copy
 *          Copy the data contained in A->u and A->v in the new low-rank
 *          representation
 *
 * @param[in] M
 *          The number of rows of the matrix A.
 *
 * @param[in] N
 *          The number of columns of the matrix A.
 *
 * @param[in, out] A
 *          The low-rank representation of the matrix. At exit, this structure
 *          is modified with the new low-rank representation of A, is the rank
 *          is small enough
 *
 * @param[in] newrk
 *          The new rank of the matrix A.
 *
 * @param[in] newrkmax
 *          The new maximum rank of the matrix A. Useful if the low-rank
 *          structure was allocated with more data than the rank.
 *
 *******************************************************************************
 *
 * @return
 *          The new rank of A
 *
 *******************************************************************************/
int
core_zlrsze( int copy, pastix_int_t M, pastix_int_t N,
             pastix_lrblock_t *A, int newrk, int newrkmax )
{
    pastix_int_t minmn = pastix_imin( M, N );

    newrkmax = (newrkmax == -1) ? newrk : newrkmax;

    /**
     * It is not interesting to compress, so we alloc space to store the full matrix
     */
    if ( (newrk * 2) > minmn )
    {
        A->u = realloc( A->u, M * N * sizeof(pastix_complex64_t) );
#if defined(PASTIX_DEBUG_LR)
        free(A->v);
#endif
        A->v = NULL;
        A->rk = -1;
        A->rkmax = M;
        return -1;
    }
    /**
     * The rank is null, we free everything
     */
    else if (newrkmax == 0)
    {
        /**
         * The rank is nul, we free everything
         */
        free(A->u);
#if defined(PASTIX_DEBUG_LR)
        free(A->v);
#endif
        A->u = NULL;
        A->v = NULL;
        A->rkmax = newrkmax;
        A->rk = newrk;
    }
    /**
     * The rank is non null, we allocate the correct amount of space, and
     * compress the stored information if necessary
     */
    else {
        pastix_complex64_t *u, *v;
        int ret;

        if ( newrkmax != A->rkmax ) {
#if defined(PASTIX_DEBUG_LR)
            u = malloc( M * newrkmax * sizeof(pastix_complex64_t) );
            v = malloc( N * newrkmax * sizeof(pastix_complex64_t) );
#else
            u = malloc( (M+N) * newrkmax * sizeof(pastix_complex64_t) );
            v = u + M * newrkmax;
#endif
            if ( copy ) {
                ret = LAPACKE_zlacpy_work( LAPACK_COL_MAJOR, 'A', M, newrk,
                                           A->u, M, u, M );
                assert(ret == 0);
                ret = LAPACKE_zlacpy_work( LAPACK_COL_MAJOR, 'A', newrk, N,
                                           A->v, A->rkmax, v, newrkmax );
                assert(ret == 0);
            }
            free(A->u);
#if defined(PASTIX_DEBUG_LR)
            free(A->v);
#endif
            A->u = u;
            A->v = v;
        }
        A->rk = newrk;
        A->rkmax = newrkmax;

        (void)ret;
    }
    assert( A->rk <= A->rkmax);
    return 0;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlr2ge - Convert a low rank matrix into a dense matrix.
 *
 *******************************************************************************
 *
 * @param[in] m
 *          Number of rows of the matrix A, and of the low rank matrix Alr.
 *
 * @param[in] n
 *          Number of columns of the matrix A, and of the low rank matrix Alr.
 *
 * @param[in] Alr
 *          The low rank matrix to be converted into a dense matrix
 *
 * @param[out] A
 *          The matrix of dimension lda-by-n in which to store the uncompressed
 *          version of Alr.
 *
 * @param[in] lda
 *          The leading dimension of the matrix A. lda >= max(1, m)
 *
 *******************************************************************************/
int
core_zlr2ge( pastix_int_t m, pastix_int_t n,
             const pastix_lrblock_t *Alr,
             pastix_complex64_t *A, pastix_int_t lda )
{
    int ret;

#if !defined(NDEBUG)
    if ( m < 0 ) {
        return -1;
    }
    if ( n < 0 ) {
        return -2;
    }
    if (Alr == NULL || Alr->rk > Alr->rkmax) {
        return -3;
    }
    if ( lda < m ) {
        return -5;
    }
    if ( Alr->rk == -1 ) {
        if (Alr->u == NULL || Alr->v != NULL || Alr->rkmax < m) {
            return -6;
        }
    }
    else if ( Alr->rk != 0){
        if (Alr->u == NULL || Alr->v == NULL) {
            return -6;
        }
    }
#endif

    if ( Alr->rk == -1 ) {
        ret = LAPACKE_zlacpy_work( LAPACK_COL_MAJOR, 'A', m, n,
                                   Alr->u, Alr->rkmax, A, lda );
        assert( ret == 0 );
    }
    else if ( Alr->rk == 0 ) {
        ret = LAPACKE_zlaset_work( LAPACK_COL_MAJOR, 'A', m, n,
                                   0., 0., A, lda );
        assert( ret == 0 );
    }
    else {
        cblas_zgemm(CblasColMajor, CblasNoTrans, CblasNoTrans,
                    m, n, Alr->rk,
                    CBLAS_SADDR(zone),  Alr->u, m,
                                        Alr->v, Alr->rkmax,
                    CBLAS_SADDR(zzero), A, lda);
    }

    return 0;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zgradd - Adds the dense matrix A to the low-rank matrix B.
 *
 *******************************************************************************
 *
 * @param[in] lowrank
 *          The structure with low-rank parameters.
 *
 * @param[in] alpha
 *          alpha * A is add to B
 *
 * @param[in] M1
 *          The number of rows of the matrix A.
 *
 * @param[in] N1
 *          The number of columns of the matrix A.
 *
 * @param[in] A
 *          The matrix A (dense)
 *
 * @param[in] lda
 *          The leading dimension of the matrix A.
 *
 * @param[in] M2
 *          The number of rows of the matrix B.
 *
 * @param[in] N2
 *          The number of columns of the matrix B.
 *
 * @param[in] B
 *          The low-rank representation of the matrix B.
 *
 * @param[in] offx
 *          The horizontal offset of A with respect to B.
 *
 * @param[in] offy
 *          The vertical offset of A with respect to B.
 *
 *******************************************************************************
 *
 * @return
 *          The new rank of B or -1 if ranks are too large for recompression
 *
 *******************************************************************************/
int
core_zgradd( pastix_lr_t lowrank, pastix_complex64_t alpha,
             pastix_int_t M1, pastix_int_t N1, pastix_complex64_t *A, pastix_int_t lda,
             pastix_int_t M2, pastix_int_t N2, pastix_lrblock_t *B,
             pastix_int_t offx, pastix_int_t offy)
{
    pastix_lrblock_t lrA;
    pastix_int_t rmax = pastix_imin( M2, N2 );
    pastix_int_t rank, ldub;
    double tol = lowrank.tolerance;

    assert( B->rk <= B->rkmax);

    if ( B->rk == -1 ) {
        pastix_complex64_t *tmp = B->u;
        ldub = B->rkmax;
        tmp += ldub * offy + offx;
        core_zgeadd( CblasNoTrans, M1, N1,
                     alpha, A,   lda,
                       1.0, tmp, ldub );
        return 0;
    }

    ldub = M2;

    /**
     * The rank is too big, we need to uncompress/compress B
     */
    if ( ((B->rk + M1) > rmax) &&
         ((B->rk + N1) > rmax) )
    {
        pastix_complex64_t *work = malloc( M2 * N2 * sizeof(pastix_complex64_t) );
        assert(B->rk > 0);

        cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                     M2, N2, B->rk,
                     CBLAS_SADDR(zone),  B->u, ldub,
                                         B->v, B->rkmax,
                     CBLAS_SADDR(zzero), work, M2 );

        core_zgeadd( PastixNoTrans, M1, N1,
                     alpha, A, lda,
                     1.,    work + M2 * offy + offx, M2 );

        core_zlrfree(B);
        lowrank.core_ge2lr( tol, M2, N2, work, M2, B );
        rank = B->rk;
        free(work);
    }
    /**
     * We consider the A matrix as Id * A or A *Id
     */
    else {
        lrA.rk = -1;
        lrA.rkmax = lda;
        lrA.u = A;
        lrA.v = NULL;
        rank = lowrank.core_rradd( tol, PastixNoTrans, &alpha,
                                   M1, N1, &lrA,
                                   M2, N2, B,
                                   offx, offy );
    }

    assert( B->rk <= B->rkmax);
    return rank;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrm2 - Computes the product of two possible low-rank matrices and
 * returns the result in AB
 *
 *******************************************************************************
 * @param[in] transA
 *         @arg CblasNoTrans   :  No transpose, op( A ) = A;
 *         @arg CblasTrans     :  Transpose, op( A ) = A';
 *
 * @param[in] transB
 *         @arg CblasNoTrans   :  No transpose, op( B ) = B;
 *         @arg CblasTrans     :  Transpose, op( B ) = B';
 *
 * @param[in] M
 *          The number of rows of the matrix A.
 *
 * @param[in] N
 *          The number of columns of the matrix B.
 *
 * @param[in] K
 *          The number of columns of the matrix A and the number of rows of the
 *          matrix B.
 *
 * @param[in] A
 *          The low-rank representation of the matrix A.
 *
 * @param[in] B
 *          The low-rank representation of the matrix B.
 *
 * @param[out] AB
 *          The low-rank representation of the matrix AB.
 *
 * @param[in] work
 *          The workspace used to store temporary data
 *
 * @param[in] ldwork
 *          The leading dimension of work
 *
 *******************************************************************************
 *
 * @return
 *          The transposition (CblasTrans or CblasNoTrans) of the product AB
 *
 *******************************************************************************/
int core_zlrm2( int transA, int transB,
                pastix_int_t M, pastix_int_t N, pastix_int_t K,
                const pastix_lrblock_t *A,
                const pastix_lrblock_t *B,
                pastix_lrblock_t *AB,
                pastix_complex64_t *work,
                pastix_int_t ldwork )
{
    pastix_int_t ldau, ldav, ldbu, ldbv;
    int transV = PastixNoTrans;

    assert( A->rk  <= A->rkmax);
    assert( B->rk  <= B->rkmax);
    assert(transA == PastixNoTrans);
    assert(transB != PastixNoTrans);

    /* Quick return if multiplication by 0 */
    if ( A->rk == 0 || B->rk == 0 ) {
        AB->rk = 0;
        AB->rkmax = 0;
        AB->u = NULL;
        AB->v = NULL;
        return transV;
    }

    ldau = (A->rk == -1) ? A->rkmax : M;
    ldav = A->rkmax;
    ldbu = (B->rk == -1) ? B->rkmax : N;
    ldbv = B->rkmax;

    if ( A->rk != -1 ) {
        /**
         * A and B are both low rank
         */
        if ( B->rk != -1 ) {
            /**
             * Let's compute A * B' = Au Av^h (Bu Bv^h)' with the smallest ws
             */
            if ( (A->rk * N) <= (B->rk * M) ) {
                /**
                 *    ABu = Au
                 *    ABv = (Av^h Bv^h') * Bu'
                 */
                assert( (A->rk * ( N + B->rk )) <= ldwork );
                AB->rk = A->rk;
                AB->rkmax = A->rk;
                AB->u = A->u;
                AB->v = work + A->rk * B->rk;

                cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                             A->rk, B->rk, K,
                             CBLAS_SADDR(zone),  A->v, ldav,
                                                 B->v, ldbv,
                             CBLAS_SADDR(zzero), work, A->rk );

                cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                             A->rk, N, B->rk,
                             CBLAS_SADDR(zone),  work,  A->rk,
                                                 B->u,  ldbu,
                             CBLAS_SADDR(zzero), AB->v, AB->rkmax );
            }
            else {
                /**
                 *    ABu = Au * (Av^h Bv^h')
                 *    ABv = Bu'
                 */
                assert( (B->rk * ( M + A->rk )) <= ldwork );
                AB->rk = B->rk;
                AB->rkmax = B->rk;
                AB->u = work + A->rk * B->rk;
                AB->v = B->u;

                cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                             A->rk, B->rk, K,
                             CBLAS_SADDR(zone),  A->v, ldav,
                                                 B->v, ldbv,
                             CBLAS_SADDR(zzero), work, A->rk );

                cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                             M, B->rk, A->rk,
                             CBLAS_SADDR(zone),  A->u,  ldau,
                                                 work,  A->rk,
                             CBLAS_SADDR(zzero), AB->u, M );

                transV = transB;
            }
        }
        /**
         * A is low rank and not B
         */
        else {
            /**
             * Let's compute A * B' = Au Av^h B' by computing only Av^h * B'
             *   ABu = Au
             *   ABv = Av^h B'
             */
            assert( (A->rk * N) <= ldwork );
            AB->rk = A->rk;
            AB->rkmax = A->rk;
            AB->u = A->u;
            AB->v = work;

            cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                         A->rk, N, K,
                         CBLAS_SADDR(zone),  A->v,  ldav,
                                             B->u,  ldbu,
                         CBLAS_SADDR(zzero), AB->v, AB->rkmax );
        }
    }
    else {
        /**
         * B is low rank and not A
         */
        if ( B->rk != -1 ) {
            /**
             * Let's compute A * B' = A * (Bu Bv^h)' by computing only A * Bv^h'
             *   ABu = A * Bv^h'
             *   ABv = Bu'
             */
            assert( (B->rk * M) <= ldwork );
            AB->rk = B->rk;
            AB->rkmax = B->rk;
            AB->u = work;
            AB->v = B->u;

            cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                         M, B->rk, K,
                         CBLAS_SADDR(zone),  A->u,  ldau,
                                             B->v,  ldbv,
                         CBLAS_SADDR(zzero), AB->u, M );

            transV = transB;
        }
        /**
         * A and B are both full rank
         */
        else {
            /**
             * A and B are both full
             *  Let's compute the product to add the full matrix
             * TODO: return low rank matrix:
             *             AB.u = A, AB.v = B' when K is small
             */
            /* if ( 2*K < pastix_imin( M, N ) ) { */
            /*     AB->rk = K; */
            /*     AB->rkmax = B->rkmax; */
            /*     AB->u = A->u; */
            /*     AB->v = B->u; */
            /*     transV = transB; */
            /* } */
            /* else { */
                assert( (M * N) <= ldwork );
                AB->rk = -1;
                AB->rkmax = M;
                AB->u = work;
                AB->v = NULL;

                cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                             M, N, K,
                             CBLAS_SADDR(zone),  A->u, ldau,
                                                 B->u, ldbu,
                             CBLAS_SADDR(zzero), work, M );
            /* } */
        }
    }
    assert( AB->rk <= AB->rkmax);
    return transV;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrm3 - Computes the product of two low-rank matrices (rank != -1) and
 * returns the result in AB
 *
 *******************************************************************************
 * @param[in] lowrank
 *          The structure with low-rank parameters.
 *
 * @param[in] transA
 *         @arg CblasNoTrans   :  No transpose, op( A ) = A;
 *         @arg CblasTrans     :  Transpose, op( A ) = A';
 *
 * @param[in] transB
 *         @arg CblasNoTrans   :  No transpose, op( B ) = B;
 *         @arg CblasTrans     :  Transpose, op( B ) = B';
 *
 * @param[in] M
 *          The number of rows of the matrix A.
 *
 * @param[in] N
 *          The number of columns of the matrix B.
 *
 * @param[in] K
 *          The number of columns of the matrix A and the number of rows of the
 *          matrix B.
 *
 * @param[in] A
 *          The low-rank representation of the matrix A.
 *
 * @param[in] B
 *          The low-rank representation of the matrix B.
 *
 * @param[out] AB
 *          The low-rank representation of the matrix AB.
 *
 *******************************************************************************
 *
 * @return
 *          The transposition (CblasTrans or CblasNoTrans) of the product AB
 *
 *******************************************************************************/
int core_zlrm3( pastix_lr_t lowrank,
                int transA, int transB,
                pastix_int_t M, pastix_int_t N, pastix_int_t K,
                const pastix_lrblock_t *A,
                const pastix_lrblock_t *B,
                pastix_lrblock_t *AB )
{
    pastix_int_t ldau, ldav, ldbu, ldbv;
    int transV = PastixNoTrans;
    pastix_complex64_t *work2;
    pastix_lrblock_t rArB;

    assert( A->rk  <= A->rkmax);
    assert( B->rk  <= B->rkmax);
    assert(transA == PastixNoTrans);
    assert(transB != PastixNoTrans);

    /* Quick return if multiplication by 0 */
    if ( A->rk == 0 || B->rk == 0 ) {
        AB->rk = 0;
        AB->rkmax = 0;
        AB->u = NULL;
        AB->v = NULL;
        return transV;
    }

    ldau = (A->rk == -1) ? A->rkmax : M;
    ldav = A->rkmax;
    ldbu = (B->rk == -1) ? B->rkmax : N;
    ldbv = B->rkmax;

    work2 = malloc( A->rk * B->rk * sizeof(pastix_complex64_t));


    /**
     * Let's compute A * B' = Au Av^h (Bu Bv^h)' with the smallest ws
     */
    cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                 A->rk, B->rk, K,
                 CBLAS_SADDR(zone),  A->v, ldav,
                                     B->v, ldbv,
                 CBLAS_SADDR(zzero), work2, A->rk );

    /**
     * Try to compress (Av^h Bv^h')
     */
    lowrank.core_ge2lr( lowrank.tolerance, A->rk, B->rk, work2, A->rk, &rArB );

    /**
     * The rank of AB is not smaller than min(rankA, rankB)
     */
    if (rArB.rk == -1){
        if ( A->rk < B->rk ) {
            /**
             *    ABu = Au
             *    ABv = (Av^h Bv^h') * Bu'
             */
            pastix_complex64_t *work = malloc( A->rk * N * sizeof(pastix_complex64_t));

            //assert( (A->rk * ( N + B->rk )) <= lwork );
            AB->rk = A->rk;
            AB->rkmax = A->rk;
            AB->u = A->u;
            AB->v = work;

            cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                         A->rk, N, B->rk,
                         CBLAS_SADDR(zone),  work2,  A->rk,
                         B->u,  ldbu,
                         CBLAS_SADDR(zzero), AB->v, AB->rkmax );
        }
        else {
            /**
             *    ABu = Au * (Av^h Bv^h')
             *    ABv = Bu'
             */
            pastix_complex64_t *work = malloc( B->rk * M * sizeof(pastix_complex64_t));

            //assert( (B->rk * ( M + A->rk )) <= lwork );
            AB->rk = B->rk;
            AB->rkmax = B->rk;
            AB->u = work;
            AB->v = B->u;

            cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                         M, B->rk, A->rk,
                         CBLAS_SADDR(zone),  A->u,  ldau,
                         work2,  A->rk,
                         CBLAS_SADDR(zzero), AB->u, M );

            transV = transB;

            /* free(work); */
        }
    }
    else if (rArB.rk == 0){
        AB->rk    = 0;
        AB->rkmax = 0;
        AB->u = NULL;
        AB->v = NULL;
    }
    /**
     * The rank of AB is smaller than min(rankA, rankB)
     */
    else{
        pastix_complex64_t *work = malloc( (M + N) * rArB.rk * sizeof(pastix_complex64_t));

        AB->rk    = rArB.rk;
        AB->rkmax = rArB.rk;
        AB->u = work;
        AB->v = work + M * rArB.rk;

        cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                     M, rArB.rk, A->rk,
                     CBLAS_SADDR(zone),  A->u,   ldau,
                                         rArB.u, A->rk,
                     CBLAS_SADDR(zzero), AB->u,  M );

        cblas_zgemm( CblasColMajor, CblasNoTrans, transB,
                     rArB.rk, N, B->rk,
                     CBLAS_SADDR(zone),  rArB.v, rArB.rkmax,
                                         B->u, ldbu,
                     CBLAS_SADDR(zzero), AB->v, rArB.rk );

        /* free(work); */
    }
    core_zlrfree(&rArB);
    free(work2);
    return transV;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrmm - A * B + C with three low-rank matrices
 *
 *******************************************************************************
 * @param[in] transA
 *         @arg CblasNoTrans   :  No transpose, op( A ) = A;
 *         @arg CblasTrans     :  Transpose, op( A ) = A';
 *
 * @param[in] transB
 *         @arg CblasNoTrans   :  No transpose, op( B ) = B;
 *         @arg CblasTrans     :  Transpose, op( B ) = B';
 *
 * @param[in] M
 *          The number of rows of the matrix A.
 *
 * @param[in] N
 *          The number of columns of the matrix B.
 *
 * @param[in] K
 *          The number of columns of the matrix A and the number of rows of the
 *          matrix B.
 *
 * @param[in] Cm
 *          The number of rows of the matrix C.
 *
 * @param[in] Cn
 *          The number of columns of the matrix C.
 *
 * @param[in] offx
 *          The horizontal offset of A with respect to B.
 *
 * @param[in] offy
 *          The vertical offset of A with respect to B.
 *
 * @param[in] alpha
 *          The multiplier parameter: C = beta * C + alpha * AB
 *
 * @param[in] A
 *          The low-rank representation of the matrix A.
 *
 * @param[in] B
 *          The low-rank representation of the matrix B.
 *
 * @param[in] beta
 *          The addition parameter: C = beta * C + alpha * AB
 *
 * @param[out] C
 *          The low-rank representation of the matrix C
 *
 * @param[in] work
 *          The workspace used to store temporary data
 *
 * @param[in] ldwork
 *          The leading dimension of work
 *
 * @param[in] fcblk
 *          The facing supernode
 *
 *******************************************************************************/
void
core_zlrmm( pastix_lr_t lowrank, int transA, int transB,
            pastix_int_t M, pastix_int_t N, pastix_int_t K,
            pastix_int_t Cm, pastix_int_t Cn,
            pastix_int_t offx, pastix_int_t offy,
            pastix_complex64_t alpha, const pastix_lrblock_t *A,
                                      const pastix_lrblock_t *B,
            pastix_complex64_t beta,        pastix_lrblock_t *C,
            pastix_complex64_t *work, pastix_int_t ldwork,
            SolverCblk *fcblk )
{
    pastix_complex64_t *tmp = NULL;
    pastix_lrblock_t AB;
    pastix_int_t ldabu, ldabv, ldcu, ldcv;
    pastix_int_t required = 0;
    int transV;
    int allocated = 0;
    double tol = lowrank.tolerance;

    assert(transA == PastixNoTrans);
    assert(transB != PastixNoTrans);
    assert( A->rk <= A->rkmax);
    assert( B->rk <= B->rkmax);
    assert( C->rk <= C->rkmax);

    /* Quick return if multiplication by 0 */
    if ( A->rk == 0 || B->rk == 0 ) {
        return;
    }

    if ( A->rk != -1 ) {
        if ( B->rk != -1 ) {
            required = 0;
            /* required = pastix_imin( A->rk * ( N + B->rk ), */
            /*                         B->rk * ( M + A->rk ) ); */
        }
        else {
            required = A->rk * N;
        }
    }
    else {
        if ( B->rk != -1 ) {
            required = B->rk * M;
        }
        else {
            required = M * N;
        }
    }

    if ( required <= ldwork ) {
        tmp = work;
    }
    else {
        tmp = malloc( required * sizeof(pastix_complex64_t));
        allocated = 1;
    }

    if (A->rk != -1 && B->rk != -1){
        /* Note that in this case tmp is not used anymore */
        /* For instance, AB.rk != -1 */
        transV = core_zlrm3( lowrank, transA, transB, M, N, K,
                             A, B, &AB );

        if (AB.rk == 0){
            if ( allocated ) {
                free(tmp);
            }
            return;
        }
    }
    else{
        transV = core_zlrm2( transA, transB, M, N, K,
                             A, B, &AB, tmp, required );
    }

    pastix_cblk_lock( fcblk );

    ldabu = (AB.rk == -1) ? AB.rkmax : M;
    ldabv = (transV == PastixNoTrans) ? AB.rkmax : N;
    ldcu = (C->rk == -1) ? C->rkmax : Cm;
    ldcv = C->rkmax;

    /**
     * The destination matrix is full rank
     */
    if (C->rk == -1) {
        pastix_complex64_t *Cptr = C->u;
        Cptr += ldcu * offy + offx;

        if ( AB.rk == -1 ) {
            core_zgeadd( PastixNoTrans, M, N,
                         alpha, AB.u, ldabu,
                         beta,  Cptr, ldcu );
        }
        else {
            cblas_zgemm( CblasColMajor, CblasNoTrans, transV,
                         M, N, AB.rk,
                         CBLAS_SADDR(alpha), AB.u, ldabu,
                                             AB.v, ldabv,
                         CBLAS_SADDR(beta),  Cptr, ldcu );
        }
    }
     /**
     * The destination matrix is low rank
     */
    else {
        if ( AB.rk == -1 ) {
            assert(beta == 1.);
            core_zgradd( lowrank, alpha,
                         M, N, tmp, AB.rkmax,
                         Cm, Cn, C,
                         offx, offy );
        }
        else {
            if ( AB.rk + C->rk > pastix_imin(M, N) ) {
                pastix_complex64_t *work = malloc( Cm * Cn * sizeof(pastix_complex64_t) );

                /* Do not uncompress a null LR structure */
                if (C->rk > 0){
                    /* Uncompress C */
                    cblas_zgemm( CblasColMajor, CblasNoTrans, CblasNoTrans,
                                 Cm, Cn, C->rk,
                                 CBLAS_SADDR(beta),  C->u, ldcu,
                                                     C->v, ldcv,
                                 CBLAS_SADDR(zzero), work, Cm );
                }
                else{
                    memset(work, 0, Cm * Cn * sizeof(pastix_complex64_t) );
                }

                /* Add A*B */
                cblas_zgemm( CblasColMajor, CblasNoTrans, transV,
                             M, N, AB.rk,
                             CBLAS_SADDR(alpha), AB.u, ldabu,
                                                 AB.v, ldabv,
                             CBLAS_SADDR(zone), work + Cm * offy + offx, Cm );

                core_zlrfree(C);
                lowrank.core_ge2lr( tol, Cm, Cn, work, Cm, C );
                free(work);
            }
            else {
                /* Need to handle correctly this case */
                lowrank.core_rradd( tol, transV, &alpha,
                                    M, N, &AB,
                                    Cm, Cn, C,
                                    offx, offy );
            }
        }
    }
    pastix_cblk_unlock( fcblk );

    /* Free memory from zlrm3 */
    if (A->rk != -1 && B->rk != -1){
        if (AB.rk == A->rk || AB.rk == B->rk){
            if ( A->rk < B->rk ) {
                free(AB.v);
            }
            else{
                free(AB.u);
            }
        }
        else if (AB.rk == 0 || A->rk == 0 || B->rk == 0){

        }
        else {
            free(AB.u);
        }
    }

    if ( allocated ) {
        free(tmp);
    }

    assert( C->rk <= C->rkmax);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_kernel
 *
 * core_zlrmge - A * B + C with A, and B low-rank matrices, and C full rank
 *
 *******************************************************************************
 * @param[in] transA
 *         @arg CblasNoTrans   :  No transpose, op( A ) = A;
 *         @arg CblasTrans     :  Transpose, op( A ) = A';
 *
 * @param[in] transB
 *         @arg CblasNoTrans   :  No transpose, op( B ) = B;
 *         @arg CblasTrans     :  Transpose, op( B ) = B';
 *
 * @param[in] M
 *          The number of rows of the matrix A.
 *
 * @param[in] N
 *          The number of columns of the matrix B.
 *
 * @param[in] K
 *          The number of columns of the matrix A and the number of rows of the
 *          matrix B.
 *
 * @param[in] alpha
 *          The multiplier parameter: C = beta * C + alpha * AB
 *
 * @param[in] A
 *          The low-rank representation of the matrix A.
 *
 * @param[in] B
 *          The low-rank representation of the matrix B.
 *
 * @param[in] beta
 *          The addition parameter: C = beta * C + alpha * AB
 *
 * @param[out] C
 *          The matrix C (full).
 *
 * @param[in] ldc
 *          The leading dimension of the matrix C.
 *
 * @param[in] work
 *          The workspace used to store temporary data
 *
 * @param[in] ldwork
 *          The leading dimension of work
 *
 * @param[in] fcblk
 *          The facing supernode
 *
 *******************************************************************************/
void
core_zlrmge( pastix_lr_t lowrank, int transA, int transB,
             pastix_int_t M, pastix_int_t N, pastix_int_t K,
             pastix_complex64_t alpha, const pastix_lrblock_t *A,
                                       const pastix_lrblock_t *B,
             pastix_complex64_t beta, pastix_complex64_t *C, int ldc,
             pastix_complex64_t *work, pastix_int_t ldwork,
             SolverCblk *fcblk )
{
    pastix_lrblock_t lrC;

    lrC.rk = -1;
    lrC.rkmax = ldc;
    lrC.u = C;
    lrC.v = NULL;

    core_zlrmm( lowrank, transA, transB, M, N, K,
                M, N, 0, 0,
                alpha, A, B, beta, &lrC,
                work, ldwork,
                fcblk );
}
