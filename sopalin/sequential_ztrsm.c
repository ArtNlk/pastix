/**
 *
 * @file sequential_ztrsm.c
 *
 *  PaStiX factorization routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 * @precisions normal z -> s d c
 *
 **/
#include "cblas.h"
#include "common.h"
#include "solver.h"
#include "bcsc.h"
#include "sopalin_data.h"
#include "pastix_zcores.h"

#if defined(PASTIX_WITH_PARSEC)
#include <dague.h>
#include <dague/data.h>
#include <dague/data_distribution.h>

int dsparse_ztrsm_sp( dague_context_t *dague,
                      sparse_matrix_desc_t *A,
                      int side, int uplo, int trans, int diag,
                      sopalin_data_t *sopalin_data,
                      int nrhs, pastix_complex64_t *b, int ldb);
#endif

void
sequential_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag,
                  sopalin_data_t *sopalin_data,
                  int nrhs, pastix_complex64_t *b, int ldb )
{
    SolverMatrix *datacode = sopalin_data->solvmtx;
    SolverCblk *cblk;
    pastix_int_t i;
    (void)pastix_data;

    if (side == PastixLeft) {
        if (uplo == PastixUpper) {
            /*
             *  Left / Upper / NoTrans
             */
            if (trans == PastixNoTrans) {
                cblk = datacode->cblktab + datacode->cblknbr - 1;
                for (i=0; i<datacode->cblknbr; i++, cblk--){
                    solve_ztrsmsp( side, uplo, trans, diag,
                                   datacode, cblk, nrhs, b, ldb );
                }
            }
            /*  We store U^t, so we swap uplo and trans */
        }
        else {
            /*
             *  Left / Lower / NoTrans
             */
            if (trans == PastixNoTrans) {
                cblk = datacode->cblktab;
                for (i=0; i<datacode->cblknbr; i++, cblk++){
                    solve_ztrsmsp( side, uplo, trans, diag,
                                   datacode, cblk, nrhs, b, ldb );
                }
            }
            /*
             *  Left / Lower / [Conj]Trans
             */
            else {
                cblk = datacode->cblktab + datacode->cblknbr - 1;
                for (i=0; i<datacode->cblknbr; i++, cblk--){
                    solve_ztrsmsp( side, uplo, trans, diag,
                                   datacode, cblk, nrhs, b, ldb );
                }
            }
        }
    }
    /**
     * Right
     */
    else {
    }
}

struct args_ztrsm_t
{
    int side, uplo, trans, diag;
    sopalin_data_t *sopalin_data;
    int nrhs;
    pastix_complex64_t *b;
    int ldb;
};

void
thread_pztrsm( int rank, void *args )
{
    struct args_ztrsm_t *arg = (struct args_ztrsm_t*)args;
    sopalin_data_t     *sopalin_data = arg->sopalin_data;
    SolverMatrix       *datacode = sopalin_data->solvmtx;
    pastix_complex64_t *b = arg->b;
    int side  = arg->side;
    int uplo  = arg->uplo;
    int trans = arg->trans;
    int diag  = arg->diag;
    int nrhs  = arg->nrhs;
    int ldb   = arg->ldb;
    SolverCblk *cblk;
    pastix_int_t i;

    /* try in sequential */
    if (!rank)
        sequential_ztrsm(NULL, side, uplo, trans, diag, sopalin_data, nrhs, b, ldb);
}

void
thread_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag,
              sopalin_data_t *sopalin_data,
              int nrhs, pastix_complex64_t *b, int ldb )
{
    struct args_ztrsm_t args_ztrsm = {side, uplo, trans, diag, sopalin_data, nrhs, b, ldb};
    isched_parallel_call( pastix_data->isched, thread_pztrsm, &args_ztrsm );
}

#if defined(PASTIX_WITH_PARSEC)
void
parsec_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag,
              sopalin_data_t *sopalin_data,
              int nrhs, pastix_complex64_t *b, int ldb )
{
    dague_context_t *ctx;

    /* Start PaRSEC */
    if (pastix_data->parsec == NULL) {
        int argc = 0;
        pastix_parsec_init( pastix_data, &argc, NULL );
    }
    ctx = pastix_data->parsec;

    /* Run the trsm */
    exit(0); /* not yet implemented */
    dsparse_ztrsm_sp( ctx, sopalin_data->solvmtx->parsec_desc, side, uplo, trans, diag, sopalin_data,
                      nrhs, b, ldb);
}
#endif

static void (*ztrsm_table[4])(pastix_data_t *, int, int, int, int, sopalin_data_t *,
                              int, pastix_complex64_t *, int) = {
    sequential_ztrsm,
    thread_ztrsm,
#if defined(PASTIX_WITH_PARSEC)
    NULL, /* parsec_ztrsm not yet implemented */
#else
    NULL,
#endif
    NULL
};

void
sopalin_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag,
               sopalin_data_t *sopalin_data,
               int nrhs, pastix_complex64_t *b, int ldb )
{
    int sched = pastix_data->iparm[IPARM_SCHEDULER];
    void (*ztrsm)(pastix_data_t *, int, int, int, int, sopalin_data_t *,
                  int, pastix_complex64_t *, int) = ztrsm_table[ sched ];

    if (ztrsm == NULL) {
        ztrsm = thread_ztrsm;
    }
    ztrsm( pastix_data, side, uplo, trans, diag, sopalin_data, nrhs, b, ldb );
}
