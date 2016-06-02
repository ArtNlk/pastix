/**
 *
 * @file pastix_task_sopalin.c
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
#include <cblas.h>
#include "common.h"
#include "solver.h"
#include "bcsc.h"
#include "sopalin_data.h"

static pastix_complex64_t zone  =  1.0;
static pastix_complex64_t mzone = -1.0;
static pastix_complex64_t zzero =  0.0;

void
sequential_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag,
                  sopalin_data_t *sopalin_data,
                  int nrhs, pastix_complex64_t *b, int ldb )
{
    SolverMatrix *datacode = sopalin_data->solvmtx;
    SolverCblk *cblk, *fcbk;
    SolverBlok *blok;
    pastix_complex64_t *coeftab;
    pastix_int_t i, j, tempm, tempn;
    (void)pastix_data;

    /*
     *  Left / Upper / NoTrans
     */
    if (side == PastixLeft) {
        if (uplo == PastixUpper) {
            /*  We store U^t, so we swap uplo and trans */
            if (trans == PastixNoTrans) {
                cblk = datacode->cblktab + datacode->cblknbr - 1;
                for (i=0; i<datacode->cblknbr; i++, cblk--){
                    tempn = cblk->lcolnum - cblk->fcolnum + 1;

                    /* Solve the diagonal block */
                    cblas_ztrsm(
                        CblasColMajor, CblasLeft, CblasLower,
                        CblasTrans, (enum CBLAS_DIAG)diag,
                        tempn, nrhs, CBLAS_SADDR(zone),
                        cblk->ucoeftab,    cblk->stride,
                        b + cblk->lcolidx, ldb );

                    /* Apply the update */
                    for (j = cblk[1].brownum-1; j>=cblk[0].brownum; j--) {
                        blok = datacode->bloktab + datacode->browtab[j];
                        fcbk = datacode->cblktab + blok->lcblknm;
                        tempm = fcbk->lcolnum - fcbk->fcolnum + 1;
                        tempn = blok->lrownum - blok->frownum + 1;
                        coeftab = (pastix_complex64_t*)(fcbk->ucoeftab);

                        if ( ! (fcbk->cblktype & CBLK_DENSE)) {
                            pastix_lrblock_t LR = blok->LRblock[1];
                            pastix_complex64_t *tmp;
                            switch (LR.rk){
                            case 0:
                                break;
                            case -1:
                                cblas_zgemm(
                                    CblasColMajor, CblasTrans, CblasNoTrans,
                                    tempm, nrhs, tempn,
                                    CBLAS_SADDR(mzone), coeftab + blok->coefind, fcbk->stride,
                                    b + cblk->lcolidx + blok->frownum - cblk->fcolnum, ldb,
                                    CBLAS_SADDR(zone),  b + fcbk->lcolidx, ldb );
                                break;
                            default:
                                MALLOC_INTERN( tmp, LR.rk * nrhs, pastix_complex64_t);

                                cblas_zgemm(
                                    CblasColMajor, CblasTrans, CblasNoTrans,
                                    LR.rk, nrhs, tempn,
                                    CBLAS_SADDR(zone), LR.u, tempn,
                                    b + cblk->lcolidx + blok->frownum - cblk->fcolnum, ldb,
                                    CBLAS_SADDR(zzero),  tmp,  LR.rk );

                                cblas_zgemm(
                                    CblasColMajor, CblasTrans, CblasNoTrans,
                                    tempm, nrhs, LR.rk,
                                    CBLAS_SADDR(mzone), LR.v, LR.rkmax,
                                    tmp, LR.rk,
                                    CBLAS_SADDR(zone),  b + fcbk->lcolidx, ldb );

                                memFree_null(tmp);
                                break;
                            }
                        }
                        else{
                            cblas_zgemm(
                                CblasColMajor, CblasTrans, CblasNoTrans,
                                tempm, nrhs, tempn,
                                CBLAS_SADDR(mzone), coeftab + blok->coefind, fcbk->stride,
                                b + cblk->lcolidx + blok->frownum - cblk->fcolnum, ldb,
                                CBLAS_SADDR(zone),  b + fcbk->lcolidx, ldb );
                        }
                    }
                }
            }
        }
        else {
            /*
             *  Left / Lower / NoTrans
             */
            if (trans == PastixNoTrans) {
                cblk = datacode->cblktab;
                for (i=0; i<datacode->cblknbr; i++, cblk++){

                    tempn = cblk->lcolnum - cblk->fcolnum + 1;
                    coeftab = (pastix_complex64_t*)(cblk->lcoeftab);

                    /* In sequential */
                    assert( cblk->fcolnum == cblk->lcolidx );

                    /* Solve the diagonal block */
                    cblas_ztrsm(
                        CblasColMajor, CblasLeft, CblasLower,
                        CblasNoTrans, (enum CBLAS_DIAG)diag,
                        tempn, nrhs, CBLAS_SADDR(zone),
                        coeftab, cblk->stride,
                        b + cblk->lcolidx, ldb );

                    /* Apply the update */
                    for (blok = cblk[0].fblokptr+1; blok < cblk[1].fblokptr; blok++ ) {
                        fcbk  = datacode->cblktab + blok->fcblknm;
                        tempm = blok->lrownum - blok->frownum + 1;

                        assert( blok->frownum >= fcbk->fcolnum );
                        assert( tempm <= (fcbk->lcolnum - fcbk->fcolnum + 1));

                        if ( ! (cblk->cblktype & CBLK_DENSE)) {
                            pastix_lrblock_t LR = blok->LRblock[0];
                            pastix_complex64_t *tmp;
                            switch (LR.rk){
                            case 0:
                                break;
                            case -1:
                                cblas_zgemm(
                                    CblasColMajor, CblasNoTrans, CblasNoTrans,
                                    tempm, nrhs, tempn,
                                    CBLAS_SADDR(mzone), coeftab + blok->coefind, cblk->stride,
                                    b + cblk->lcolidx, ldb,
                                    CBLAS_SADDR(zone),  b + fcbk->lcolidx + blok->frownum - fcbk->fcolnum, ldb );
                                break;
                            default:
                                MALLOC_INTERN( tmp, LR.rk * nrhs, pastix_complex64_t);

                                cblas_zgemm(
                                    CblasColMajor, CblasNoTrans, CblasNoTrans,
                                    LR.rk, nrhs, tempn,
                                    CBLAS_SADDR(zone), LR.v, LR.rkmax,
                                    b + cblk->lcolidx, ldb,
                                    CBLAS_SADDR(zzero), tmp, LR.rk);

                                cblas_zgemm(
                                    CblasColMajor, CblasNoTrans, CblasNoTrans,
                                    tempm, nrhs, LR.rk,
                                    CBLAS_SADDR(mzone), LR.u, tempm,
                                    tmp, LR.rk,
                                    CBLAS_SADDR(zone),  b + fcbk->lcolidx + blok->frownum - fcbk->fcolnum, ldb );

                                memFree_null(tmp);
                                break;
                            }
                        }
                        else{
                            cblas_zgemm(
                                CblasColMajor, CblasNoTrans, CblasNoTrans,
                                tempm, nrhs, tempn,
                                CBLAS_SADDR(mzone), coeftab + blok->coefind, cblk->stride,
                                b + cblk->lcolidx, ldb,
                                CBLAS_SADDR(zone),  b + fcbk->lcolidx + blok->frownum - fcbk->fcolnum, ldb );
                        }
                    }
                }
            }
            /*
             *  Left / Lower / [Conj]Trans
             */
            else {
                cblk = datacode->cblktab + datacode->cblknbr - 1;
                for (i=0; i<datacode->cblknbr; i++, cblk--){

                    tempn = cblk->lcolnum - cblk->fcolnum + 1;

                    /* Solve the diagonal block */
                    cblas_ztrsm(
                        CblasColMajor, CblasLeft, CblasLower,
                        (enum CBLAS_TRANSPOSE)trans, (enum CBLAS_DIAG)diag,
                        tempn, nrhs, CBLAS_SADDR(zone),
                        cblk->lcoeftab,    cblk->stride,
                        b + cblk->lcolidx, ldb );

                    /* Apply the update */
                    for (j = cblk[1].brownum-1; j>=cblk[0].brownum; j-- ) {
                        blok = datacode->bloktab + datacode->browtab[j];
                        fcbk = datacode->cblktab + blok->lcblknm;
                        tempm = fcbk->lcolnum - fcbk->fcolnum + 1;
                        tempn = blok->lrownum - blok->frownum + 1;
                        coeftab = (pastix_complex64_t*)(fcbk->lcoeftab);

                        cblas_zgemm(
                            CblasColMajor, (enum CBLAS_TRANSPOSE)trans, CblasNoTrans,
                            tempm, nrhs, tempn,
                            CBLAS_SADDR(mzone), coeftab + blok->coefind, fcbk->stride,
                                                b + cblk->lcolidx + blok->frownum - cblk->fcolnum, ldb,
                            CBLAS_SADDR(zone),  b + fcbk->lcolidx, ldb );
                    }
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
