/**
 *
 * @file pastix_task_solve.c
 *
 *  PaStiX solve routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Theophile Terraz
 * @author Tony Delarue
 * @date 2021-07-02
 *
 **/
#define _GNU_SOURCE 1
#include "common.h"
#include "bcsc/bcsc.h"
#include "pastix/order.h"
#include "blend/solver.h"
#include "sopalin/sopalin_data.h"

#include "bcsc/bcsc_z.h"
#include "bcsc/bcsc_c.h"
#include "bcsc/bcsc_d.h"
#include "bcsc/bcsc_s.h"

#if defined(PASTIX_DEBUG_SOLVE)
#include <z_spm.h>
#include <c_spm.h>
#include <d_spm.h>
#include <s_spm.h>

static volatile int32_t step = 0;

static inline void
dump_rhs( pastix_data_t *pastix_data, const char *name,
          spm_coeftype_t flttype, int m, int n, const void *b, int ldb )
{
    FILE *f;
    f = pastix_fopenw( (pastix_data->dir_local == NULL) ? "." : pastix_data->dir_local,
                       name, "w" );

    switch( flttype ) {
    case SpmComplex64:
        z_spmDensePrint( f, m, n, b, ldb );
        break;
    case SpmComplex32:
        c_spmDensePrint( f, m, n, b, ldb );
        break;
    case SpmDouble:
        d_spmDensePrint( f, m, n, b, ldb );
        break;
    case SpmFloat:
        s_spmDensePrint( f, m, n, b, ldb );
        break;
    case SpmPattern:
        ;
    }

    fclose(f);
}
#else
static inline void
dump_rhs( pastix_data_t *pastix_data, const char *name,
          spm_coeftype_t flttype, int m, int n, const void *b, int ldb )
{
    (void)pastix_data;
    (void)name;
    (void)m;
    (void)n;
    (void)b;
    (void)flttype;
    (void)ldb;
}
#endif

/**
 *******************************************************************************
 *
 * @ingroup pastix_solve
 *
 * @brief Apply a permutation on the right-and-side vector before the solve step.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION, IPARM_APPLYPERM_WS.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] flttype
 *          This arithmetic of the sparse matrix.
 *
 * @param[in] dir
 *          Forward or backword application of the permutation.
 *
 * @param[in] m
 *          Size of the right-and-side vectors.
 *
 * @param[in] n
 *          Number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vectors (can be multiple RHS).
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_applyorder( pastix_data_t *pastix_data,
                           pastix_coeftype_t flttype, pastix_dir_t dir,
                           pastix_int_t m, pastix_int_t n, void *b, pastix_int_t ldb )
{
    pastix_int_t *perm;
    int ts;

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_applyorder: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (b == NULL) {
        errorPrint("pastix_subtask_applyorder: wrong b parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( !(pastix_data->steps & STEP_CSC2BCSC) ) {
        errorPrint("pastix_subtask_applyorder: All steps from pastix_task_init() to pastix_subtask_csc2bcsc() have to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    /* Make sure ordering is 0 based */
    if ( pastix_data->ordemesh->baseval != 0 ) {
        errorPrint("pastix_subtask_applyorder: ordermesh must be 0-based");
        return PASTIX_ERR_BADPARAMETER;
    }

    ts   = pastix_data->iparm[IPARM_APPLYPERM_WS];
    perm = pastix_data->ordemesh->peritab;

    /* See also xlapmr and xlapmt */
    switch( flttype ) {
    case PastixComplex64:
        bvec_zlapmr( ts, dir, m, n, b, ldb, perm );
        break;

    case PastixComplex32:
        bvec_clapmr( ts, dir, m, n, b, ldb, perm );
        break;

    case PastixFloat:
        bvec_slapmr( ts, dir, m, n, b, ldb, perm );
        break;

    case PastixDouble:
    default:
        bvec_dlapmr( ts, dir, m, n, b, ldb, perm );
    }

#if defined(PASTIX_DEBUG_SOLVE)
    {
        char *fullname;
        int32_t lstep = pastix_atomic_inc_32b( &step );
        asprintf( &fullname, "solve_%02d_applyorder_%s.rhs", lstep,
                  ( dir == PastixDirForward ) ? "Forward" : "Backward" );

        dump_rhs( pastix_data, fullname,
                  flttype, m, n, b, ldb );
        free(fullname);
    }
#endif

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_solve
 *
 * @brief Apply a triangular solve on the right-and-side vectors.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] flttype
 *          This arithmetic of the sparse matrix.
 *
 * @param[in] side
 *          Left or right application.
 *
 * @param[in] uplo
 *          Upper or Lower part.
 *
 * @param[in] trans
 *          With or without transposition (or conjugate transposition).
 *
 * @param[in] diag
 *          Diagonal terms are unit or not.
 *
 * @param[in] nrhs
 *          The number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vector (can be multiple RHS).
 *          On exit, the solution is stored in place of the right-hand-side vector.
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_trsm( pastix_data_t *pastix_data,
                     pastix_coeftype_t flttype, pastix_side_t side,
                     pastix_uplo_t uplo, pastix_trans_t trans, pastix_diag_t diag,
                     pastix_int_t nrhs, void *b, pastix_int_t ldb )
{
    sopalin_data_t sopalin_data;
    int i, bs = nrhs;

#if defined(PASTIX_WITH_MPI)
    bs = 1;
#endif

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_trsm: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (b == NULL) {
        errorPrint("pastix_subtask_trsm: wrong b parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( !(pastix_data->steps & STEP_NUMFACT) ) {
        errorPrint("pastix_subtask_trsm: All steps from pastix_task_init() to pastix_task_numfact() have to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    /*
     * Ensure that the scheduler is correct and is in the same
     * family that the one used for the factorization.
     */
    pastix_check_and_correct_scheduler( pastix_data );

    sopalin_data.solvmtx = pastix_data->solvmatr;

    switch (flttype) {
    case PastixComplex64:
    {
        pastix_complex64_t *lb = b;
        for( i = 0; i < nrhs; i+=bs, lb += ldb ) {
            sopalin_ztrsm( pastix_data, side, uplo, trans, diag,
                           &sopalin_data, bs, lb, ldb );
        }
    }
    break;
    case PastixComplex32:
    {
        pastix_complex32_t *lb = b;
        for( i = 0; i < nrhs; i+=bs, lb += ldb ) {
            sopalin_ctrsm( pastix_data, side, uplo, trans, diag,
                           &sopalin_data, bs, lb, ldb );
        }
    }
    break;
    case PastixDouble:
    {
        double *lb = b;
        trans = (trans == PastixConjTrans) ? PastixTrans : trans;
        for( i = 0; i < nrhs; i+=bs, lb += ldb ) {
            sopalin_dtrsm( pastix_data, side, uplo, trans, diag,
                           &sopalin_data, bs, lb, ldb );
        }
    }
    break;
    case PastixFloat:
    {
        float *lb = b;
        trans = (trans == PastixConjTrans) ? PastixTrans : trans;
        for( i = 0; i < nrhs; i+=bs, lb += ldb ) {
            sopalin_strsm( pastix_data, side, uplo, trans, diag,
                           &sopalin_data, bs, lb, ldb );
        }
    }
    break;
    default:
        fprintf(stderr, "Unknown floating point arithmetic\n" );
    }

#if defined(PASTIX_DEBUG_SOLVE)
    {
        char *fullname;
        int32_t lstep = pastix_atomic_inc_32b( &step );
        asprintf( &fullname, "solve_%02d_trsm_%c%c%c%c.rhs", lstep,
                  (side  == PastixLeft)  ? 'L' : 'R',
                  (uplo  == PastixUpper) ? 'U' : 'L',
                  (trans == PastixConjTrans) ? 'C' : (trans == PastixTrans ? 'T' : 'N'),
                  (diag  == PastixUnit)  ? 'U' : 'N' );

        dump_rhs( pastix_data, fullname,
                  flttype, pastix_data->bcsc->gN, nrhs, b, ldb );
        free(fullname);
    }
#endif

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_solve
 *
 * @brief Apply a diagonal operation on the right-and-side vectors.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] flttype
 *          This arithmetic of the sparse matrix.
 *
 * @param[in] nrhs
 *          The number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vector (can be multiple RHS).
 *          On exit, the solution is stored in place of the right-hand-side vector.
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_diag( pastix_data_t *pastix_data, pastix_coeftype_t flttype,
                     pastix_int_t nrhs, void *b, pastix_int_t ldb )
{
    sopalin_data_t sopalin_data;

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_diag: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if (b == NULL) {
        errorPrint("pastix_subtask_diag: wrong b parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( !(pastix_data->steps & STEP_NUMFACT) ) {
        errorPrint("pastix_subtask_trsm: All steps from pastix_task_init() to pastix_task_numfact() have to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    /*
     * Ensure that the scheduler is correct and is in the same
     * family that the one used for the factorization.
     */
    pastix_check_and_correct_scheduler( pastix_data );

    sopalin_data.solvmtx = pastix_data->solvmatr;

    switch (flttype) {
    case PastixComplex64:
        sopalin_zdiag( pastix_data, &sopalin_data, nrhs, (pastix_complex64_t *)b, ldb );
        break;
    case PastixComplex32:
        sopalin_cdiag( pastix_data, &sopalin_data, nrhs, (pastix_complex32_t *)b, ldb );
        break;
    case PastixDouble:
        sopalin_ddiag( pastix_data, &sopalin_data, nrhs, (double *)b, ldb );
        break;
    case PastixFloat:
        sopalin_sdiag( pastix_data, &sopalin_data, nrhs, (float *)b, ldb );
        break;
    default:
        fprintf(stderr, "Unknown floating point arithmetic\n" );
    }

#if defined(PASTIX_DEBUG_SOLVE)
    {
        char *fullname;
        int32_t lstep = pastix_atomic_inc_32b( &step );
        asprintf( &fullname, "solve_%02d_diag.rhs", lstep );

        dump_rhs( pastix_data, fullname,
                  flttype, pastix_data->bcsc->gN, nrhs, b, ldb );
        free(fullname);
    }
#endif

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_solve
 *
 * @brief Solve the given problem without applying the permutation.
 *
 * @warning The input vector is considered already permuted. For a solve step
 * with permutation, see pastix_task_solve()
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] transA
 *          PastixNoTrans:   A is not transposed (CSC matrix)
 *          PastixTrans:     A is transposed (CSR of symmetric/general matrix)
 *          PastixConjTrans: A is conjugate transposed (CSR of hermitian matrix)
 *
 * @param[in] nrhs
 *          The number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vectors (can be multiple RHS).
 *          On exit, the solution is stored in place of the right-hand-side vector.
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_solve_adv( pastix_data_t *pastix_data, pastix_trans_t transA,
                          pastix_int_t nrhs, void *b, pastix_int_t ldb )
{
    pastix_bcsc_t     *bcsc;
    pastix_factotype_t factotype;

    pastix_uplo_t  uplo;
    pastix_diag_t  diag;
    pastix_trans_t trans;
    pastix_trans_t transfact = PastixTrans;

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_task_solve: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( !(pastix_data->steps & STEP_NUMFACT) ) {
        errorPrint("pastix_task_solve: All steps from pastix_task_init() to pastix_task_numfact() have to be called before calling this function");
        return PASTIX_ERR_BADPARAMETER;
    }

    bcsc      = pastix_data->bcsc;
    factotype = pastix_data->iparm[IPARM_FACTORIZATION];

    /**
     *
     * Summary of the operations to perform the full solve if the original A was
     * either transposed or conjugate transposed.
     *
     * op(A)     | Factorization       | Step 1    | Step 2
     * ----------+---------------------+-----------+-----------
     * NoTrans   | L U                 | L   y = b | U   x = y
     * NoTrans   | L L^t               | L   y = b | L^t x = y
     * NoTrans   | L L^h               | L   y = b | L^h x = y
     * Trans     |(L U  )^t = U^t  L^t | U^t y = b | L^t x = y
     * Trans     |(L L^t)^t = L    L^t | L   y = b | L^t x = y
     * Trans     |(L L^h)^t = c(L) L^t | Not handled (c(L))
     * ConjTrans |(L U  )^h = U^h  L^h | Not handled (U^h)
     * ConjTrans |(L L^t)^h = c(L) L^h | Not handled (c(L))
     * ConjTrans |(L L^h)^h = L    L^h | L   y = b | L^h x = y
     *
     */
    /* Value of the transpose case */
    if ( ((bcsc->flttype == PastixComplex32) || (bcsc->flttype == PastixComplex64)) &&
         ((factotype == PastixFactLLH ) || ( factotype == PastixFactLDLH )) )
    {
        transfact = PastixConjTrans;
    }

    if ( (transA != PastixNoTrans) &&
         (transA != transfact) )
    {
        errorPrint("pastix_task_solve: transA incompatible with the factotype used (require extra conj(L) not handled)");
        return PASTIX_ERR_BADPARAMETER;
    }

    {
        double timer;
        /* Start timer */
        clockSyncStart( timer, pastix_data->inter_node_comm );

        /*
         * Solve the first step
         */
        uplo  = PastixLower;
        trans = PastixNoTrans;
        diag  = PastixNonUnit;

        if (( transA != PastixNoTrans ) && ( factotype == PastixFactLU )) {
            uplo  = PastixUpper;
            trans = transA;
        }
        if (( transA == PastixNoTrans ) && ( factotype == PastixFactLU )) {
            diag = PastixUnit;
        }

        if (( factotype == PastixFactLDLT ) ||
            ( factotype == PastixFactLDLH ) )
        {
            diag = PastixUnit;
        }

        pastix_subtask_trsm( pastix_data, bcsc->flttype,
                             PastixLeft, uplo, trans, diag,
                             nrhs, b, ldb );

        /*
         * Solve the diagonal step
         */
        if( (factotype == PastixFactLDLT) ||
            (factotype == PastixFactLDLH) )
        {
            /* Solve y = D z with z = ([L^t | L^h] P x) */
            pastix_subtask_diag( pastix_data, bcsc->flttype, nrhs, b, ldb );
        }

        /*
         * Solve the second step
         */
        uplo  = PastixLower;
        trans = transfact;
        diag  = PastixNonUnit;

        if (( transA == PastixNoTrans ) && ( factotype == PastixFactLU )) {
            uplo  = PastixUpper;
            trans = PastixNoTrans;
        }
        if (( transA != PastixNoTrans ) && ( factotype == PastixFactLU )) {
            diag = PastixUnit;
        }

        if (( factotype == PastixFactLDLT ) ||
            ( factotype == PastixFactLDLH ) )
        {
            diag = PastixUnit;
        }

        pastix_subtask_trsm( pastix_data, bcsc->flttype,
                             PastixLeft, uplo, trans, diag,
                             nrhs, b, ldb );

        /* Stop Timer */
        clockSyncStop( timer, pastix_data->inter_node_comm );

        pastix_data->dparm[DPARM_SOLV_TIME] = clockVal(timer);
        if ( pastix_data->iparm[IPARM_VERBOSE] > PastixVerboseNot ) {
            pastix_print( pastix_data->inter_node_procnum, 0, OUT_TIME_SOLV,
                          pastix_data->dparm[DPARM_SOLV_TIME] );
        }
    }

    return EXIT_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_solve
 *
 * @brief Solve the given problem without applying the permutation.
 *
 * @warning The input vector is considered already permuted. For a solve step
 * with permutation, see pastix_task_solve()
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION, IPARM_TRANSPOSE_SOLVE.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] nrhs
 *          The number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vectors (can be multiple RHS).
 *          On exit, the solution is stored in place of the right-hand-side vector.
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_solve( pastix_data_t *pastix_data,
                      pastix_int_t nrhs, void *b, pastix_int_t ldb )
{
    return pastix_subtask_solve_adv( pastix_data,
                                     pastix_data->iparm[IPARM_TRANSPOSE_SOLVE],
                                     nrhs, b, ldb );
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_users
 *
 * @brief Solve the given problem.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_FACTORIZATION, IPARM_TRANSPOSE_SOLVE.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *
 * @param[in] nrhs
 *          The number of right-and-side vectors.
 *
 * @param[inout] b
 *          The right-and-side vectors (can be multiple RHS).
 *          On exit, the solution is stored in place of the right-hand-side vector.
 *
 * @param[in] ldb
 *          The leading dimension of the right-and-side vectors.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_task_solve( pastix_data_t *pastix_data,
                   pastix_int_t nrhs, void *b, pastix_int_t ldb )
{
    pastix_bcsc_t *bcsc;
    void          *bglob = NULL;
    void          *tmp   = NULL;

    /*
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_task_solve: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }

    if ( !(pastix_data->steps & STEP_NUMFACT) ) {
        errorPrint("pastix_task_solve: Numerical factorization hasn't been done.");
        return PASTIX_ERR_BADPARAMETER;
    }

    bcsc  = pastix_data->bcsc;

    /* The spm is distributed, so we have to gather the RHS */
    if ( pastix_data->csc->loc2glob != NULL ) {
        if( pastix_data->iparm[IPARM_VERBOSE] > PastixVerboseNo ) {
            pastix_print( pastix_data->procnum, 0, "pastix_task_solve: the RHS has to be centralized for the moment\n" );
        }

        tmp = b;
        spmGatherRHS( nrhs, pastix_data->csc, b, ldb, &bglob, -1 );
        b   = bglob;
        ldb = pastix_data->csc->gNexp;
    }

    /* Compute P * b */
    pastix_subtask_applyorder( pastix_data, bcsc->flttype,
                               PastixDirForward, bcsc->gN, nrhs, b, ldb );

    /* Solve A x = b */
    pastix_subtask_solve( pastix_data, nrhs, b, ldb );

    /* Compute P^t * b */
    pastix_subtask_applyorder( pastix_data, bcsc->flttype,
                               PastixDirBackward, bcsc->gN, nrhs, b, ldb );

    if( tmp != NULL ) {
        pastix_int_t ldbglob = ldb;
        ldb = pastix_data->csc->nexp;
        b   = tmp;

        spmExtractLocalRHS( nrhs, pastix_data->csc, bglob, ldbglob, b, ldb );
        memFree_null( bglob );
    }

    return EXIT_SUCCESS;
}
