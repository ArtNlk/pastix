/**
 *
 * @file coeftab_zdump.c
 *
 * Precision dependent sequential routines to apply operation of the full matrix.
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @date 2017-04-28
 *
 * @precisions normal z -> s d c
 *
 **/
#include "common.h"
#include "solver.h"
#include "lapacke.h"
#include "sopalin/coeftab_z.h"
#include "pastix_zcores.h"

/**
 *******************************************************************************
 *
 * @brief Dump a single column block into a FILE in a human readale format.
 *
 * All non-zeroes coefficients are dumped in the format:
 *    i j val
 * with one value per row.
 *
 *******************************************************************************
 *
 * @param[in] cblk
 *          The column block to dump into the file.
 *
 * @param[in] uplo
 *          Specify if upper or lower part is printed.
 *
 * @param[inout] stream
 *          The FILE structure opened in write mode.
 *
 *******************************************************************************/
void
cpucblk_zdump( const SolverCblk *cblk,
               pastix_uplo_t     uplo,
               FILE             *stream )
{
    const pastix_complex64_t *coeftab = uplo == PastixUpper ? cblk->ucoeftab : cblk->lcoeftab;
    SolverBlok  *blok;
    pastix_int_t itercol;
    pastix_int_t iterrow;
    pastix_int_t coefindx;

    /* We don't know how to dump the compressed block for now */
    if ( cblk->cblktype & CBLK_COMPRESSED ) {
        fprintf(stderr, "coeftab_zcblkdump: Can't dump a compressed cblk\n");
        return;
    }

    for (itercol  = cblk->fcolnum;
         itercol <= cblk->lcolnum;
         itercol++)
    {
        /* Diagonal Block */
        blok     = cblk->fblokptr;
        coefindx = blok->coefind;
        if (cblk->cblktype & CBLK_LAYOUT_2D) {
            coefindx += (itercol - cblk->fcolnum) * blok_rownbr( blok );
        }
        else {
            coefindx += (itercol - cblk->fcolnum) * cblk->stride;
        }

        for (iterrow  = blok->frownum;
             iterrow <= blok->lrownum;
             iterrow++, coefindx++)
        {
            if ((cabs( coeftab[coefindx] ) > 0.) &&
                (itercol <= iterrow))
            {
                if ( uplo == PastixUpper ) {
#if defined(PRECISION_z) || defined(PRECISION_c)
                    fprintf(stream, "%ld %ld (%13e,%13e) [U]\n",
                            (long)itercol, (long)iterrow,
                            creal(coeftab[coefindx]), cimag(coeftab[coefindx]));
#else
                    fprintf(stream, "%ld %ld %13e [U]\n",
                            (long)itercol, (long)iterrow,
                            coeftab[coefindx]);
#endif
                }
                else {
#if defined(PRECISION_z) || defined(PRECISION_c)
                    fprintf(stream, "%ld %ld (%13e,%13e) [L]\n",
                            (long)iterrow, (long)itercol,
                            creal(coeftab[coefindx]), cimag(coeftab[coefindx]));
#else
                    fprintf(stream, "%ld %ld %13e [L]\n",
                            (long)iterrow, (long)itercol,
                            coeftab[coefindx]);
#endif
                }
            }
        }

        /* Off diagonal blocks */
        blok++;
        while( blok < (cblk+1)->fblokptr )
        {
            coefindx  = blok->coefind;
            if (cblk->cblktype & CBLK_LAYOUT_2D) {
                coefindx += (itercol - cblk->fcolnum) * blok_rownbr( blok );
            }
            else {
                coefindx += (itercol - cblk->fcolnum) * cblk->stride;
            }

            for (iterrow  = blok->frownum;
                 iterrow <= blok->lrownum;
                 iterrow++, coefindx++)
            {
                if (cabs( coeftab[coefindx]) > 0.)
                {
                    if ( uplo == PastixUpper ) {
#if defined(PRECISION_z) || defined(PRECISION_c)
                        fprintf(stream, "%ld %ld (%13e,%13e) [U]\n",
                                (long)itercol, (long)iterrow,
                                creal(coeftab[coefindx]), cimag(coeftab[coefindx]));
#else
                        fprintf(stream, "%ld %ld %13e [U]\n",
                                (long)itercol, (long)iterrow,
                                coeftab[coefindx]);
#endif
                    }
                    else {
#if defined(PRECISION_z) || defined(PRECISION_c)
                        fprintf(stream, "%ld %ld (%13e,%13e) [L]\n",
                                (long)iterrow, (long)itercol,
                                creal(coeftab[coefindx]), cimag(coeftab[coefindx]));
#else
                        fprintf(stream, "%ld %ld %13e [L]\n",
                                (long)iterrow, (long)itercol,
                                coeftab[coefindx]);
#endif
                    }
                }
            }
            blok++;
        }
    }
}

/**
 *******************************************************************************
 *
 * @brief Dump the solver matrix coefficients into a file in human readable
 * format.
 *
 * All non-zeroes coefficients are dumped in the format:
 *    i j val
 * with one value per row.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data instance to access the unique directory id in which
 *          output the files.
 *
 * @param[in] solvmtx
 *          The solver matrix to print.
 *
 * @param[in] filename
 *          The filename where to store the output matrix.
 *
 *******************************************************************************/
void
coeftab_zdump( pastix_data_t      *pastix_data,
               const SolverMatrix *solvmtx,
               const char         *filename )
{
    SolverCblk *cblk = solvmtx->cblktab;
    pastix_int_t itercblk;
    FILE *stream = NULL;

    stream = pastix_fopenw( &(pastix_data->dirtemp), filename, "w" );
    if ( stream == NULL ){
        return;
    }

    /*
     * TODO: there is a problem right here for now, because there are no
     * distinctions between L and U coeffcients in the final file
     */
    for (itercblk=0; itercblk<solvmtx->cblknbr; itercblk++, cblk++)
    {
        cpucblk_zdump( cblk, PastixLower, stream );
        if ( NULL != cblk->ucoeftab )
            cpucblk_zdump( cblk, PastixUpper, stream );
    }

    fclose( stream );
}

/**
 *******************************************************************************
 *
 * @brief Compare two solver matrices full-rank format.
 *
 * The second solver matrix is overwritten by the difference of the two
 * matrices.  The frobenius norm of the difference of each column block is
 * computed and the functions returns 0 if the result for all the column blocks
 * of:
 *      || B_k - A_k || / ( || A_k || * eps )
 *
 * is below 10. Otherwise, an error message is printed and 1 is returned.
 *
 *******************************************************************************
 *
 * @param[in] solvA
 *          The solver matrix A.
 *
 * @param[inout] cblkB
 *          The solver matrix B.
 *          On exit, B coefficient arrays are overwritten by the result of
 *          (B-A).
 *
 *******************************************************************************
 *
 * @return 0 if the test is passed, >= 0 otherwise.
 *
 *******************************************************************************/
int
coeftab_zdiff( const SolverMatrix *solvA, SolverMatrix *solvB )
{
    SolverCblk *cblkA = solvA->cblktab;
    SolverCblk *cblkB = solvB->cblktab;
    pastix_int_t cblknum;
    int rc       = 0;
    int saved_rc = 0;

    for(cblknum=0; cblknum<solvA->cblknbr; cblknum++, cblkA++, cblkB++) {
        rc += cpucblk_zdiff( cblkA, cblkB );
        if ( rc != saved_rc ){
            fprintf(stderr, "CBLK %ld was not correctly compressed\n", (long)cblknum);
            saved_rc = rc;
        }
    }

    return rc;
}

/**
 *******************************************************************************
 *
 * @brief Compress all the cblks marked as valid for low-rank format.
 *
 * All the cblk in the top levels of the elimination tree markes as candidates
 * for compression are compressed if there is a gain to compress them. The
 * compression to low-rank format is parameterized by the input information
 * stored in the lowrank structure. On exit, all the cblks marked for
 * compression are stored through the low-rank structure, even if they are kept
 * in their full-rank form.
 *
 * @remark This routine is sequential
 *
 *******************************************************************************
 *
 * @param[inout] solvmtx
 *          The solver matrix of the problem to compress.
 *
 *******************************************************************************
 *
 * @return The memory gain resulting from the compression to low-rank format in
 * Bytes.
 *
 *******************************************************************************/
pastix_int_t
coeftab_zcompress( SolverMatrix *solvmtx )
{
    SolverCblk *cblk  = solvmtx->cblktab;
    pastix_coefside_t side = (solvmtx->factotype == PastixFactLU) ? PastixLUCoef : PastixLCoef;
    pastix_int_t cblknum, gain = 0;

    for(cblknum=0; cblknum<solvmtx->cblknbr; cblknum++, cblk++) {
        if ( cblk->cblktype & CBLK_COMPRESSED ) {
            gain += cpucblk_zcompress( side, cblk, solvmtx->lowrank );
        }
    }
    return gain;
}

/**
 *******************************************************************************
 *
 * @brief Uncompress all column block in low-rank format into full-rank format
 *
 *******************************************************************************
 *
 * @param[inout] solvmtx
 *          The solver matrix of the problem.
 *
 *******************************************************************************/
void
coeftab_zuncompress( SolverMatrix *solvmtx )
{
    SolverCblk  *cblk   = solvmtx->cblktab;
    pastix_int_t cblknum;
    pastix_coefside_t side = (solvmtx->factotype == PastixFactLU) ? PastixLUCoef : PastixLCoef;

    for(cblknum=0; cblknum<solvmtx->cblknbr; cblknum++, cblk++) {
        if (cblk->cblktype & CBLK_COMPRESSED) {
            cpucblk_zuncompress( side, cblk );
        }
    }
}

/**
 *******************************************************************************
 *
 * @brief Compute the memory gain of the low-rank form over the full-rank form
 * for the entire matrix.
 *
 * This function returns the memory gain in bytes for the full matrix when
 * column blocks are stored in low-rank format compared to a full rank storage.
 *
 *******************************************************************************
 *
 * @param[in] solvmtx
 *          The solver matrix of the problem.
 *
 *******************************************************************************
 *
 * @return The difference in favor of the low-rank storage against the full rank
 *         storage.
 *
 *******************************************************************************/
pastix_int_t
coeftab_zmemory( const SolverMatrix *solvmtx )
{
    pastix_coefside_t side = (solvmtx->factotype == PastixFactLU) ? PastixLUCoef : PastixLCoef;
    SolverCblk  *cblk = solvmtx->cblktab;
    pastix_int_t cblknum;
    pastix_int_t gain = 0;
    pastix_int_t original = 0;
    double       memgain, memoriginal;

    for(cblknum=0; cblknum<solvmtx->cblknbr; cblknum++, cblk++) {
        original += cblk_colnbr( cblk ) * cblk->stride;
        if (cblk->cblktype & CBLK_COMPRESSED) {
            gain += cpucblk_zmemory( side, cblk );
        }
    }

    if ( side == PastixLUCoef ) {
        original *= 2;
    }

    memgain     = gain     * pastix_size_of( PastixComplex64 );
    memoriginal = original * pastix_size_of( PastixComplex64 );
    pastix_print(0, 0,
                 OUT_LOWRANK_SUMMARY,
                 (long)gain, (long)original,
                 MEMORY_WRITE(memgain),     MEMORY_UNIT_WRITE(memgain),
                 MEMORY_WRITE(memoriginal), MEMORY_UNIT_WRITE(memoriginal));

    return gain;
}


/**
 *******************************************************************************
 *
 * @brief Extract the Schur complement
 *
 * This routine is sequential and returns the full Schur complement
 * uncommpressed in Lapack format.
 *
 *******************************************************************************
 *
 * @param[in] solvmtx
 *          The solver matrix structure describing the problem.
 *
 * @param[inout] S
 *          The pointer to the allocated matrix array that will store the Schur
 *          complement.
 *
 * @param[in] lds
 *          The leading dimension of the S array.
 *
 *******************************************************************************/
void
coeftab_zgetschur( const SolverMatrix *solvmtx,
                   pastix_complex64_t *S, pastix_int_t lds )
{
    SolverCblk *cblk = solvmtx->cblktab + solvmtx->cblkschur;
    pastix_complex64_t *localS;
    pastix_int_t itercblk, fcolnum, nbcol;
    int upper_part = (solvmtx->factotype == PastixFactLU);
    fcolnum = cblk->fcolnum;

    nbcol = solvmtx->nodenbr - fcolnum;
    assert( nbcol <= lds );

    /* Initialize the array to 0 */
    LAPACKE_zlaset_work( LAPACK_COL_MAJOR, 'A', nbcol, nbcol, 0., 0., S, lds );

    for (itercblk=solvmtx->cblkschur; itercblk<solvmtx->cblknbr; itercblk++, cblk++)
    {
        assert( cblk->cblktype & CBLK_IN_SCHUR );
        assert( lds >= cblk->stride );

        localS = S + (cblk->fcolnum - fcolnum) * lds + (cblk->fcolnum - fcolnum);

        cpucblk_zgetschur( cblk, upper_part, localS, lds );
    }
}
