/**
 *
 * @file symbol.c
 *
 *  PaStiX symbol structure routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author David Goudin
 * @author Francois Pelegrin
 * @author Mathieu Faverge
 * @author Pascal Henon
 * @author Pierre Ramet
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "symbol.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_symbol
 *
 * symbolInit - Initialize the symbol structure.
 *
 *******************************************************************************
 *
 * @param[in,out] symbptr
 *          The symbol structure to initialize.
 *
 *******************************************************************************/
int
symbolInit ( SymbolMatrix *symbptr )
{
    memset (symbptr, 0, sizeof (SymbolMatrix));
#ifdef STARPU_GET_TASK_CTX
    symbptr->starpu_subtree_nbr=1;
#endif
    return (0);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_symbol
 *
 * symbolExit - Frees the contents of the given symbolic block matrix.Clean up
 * the symbol structure.
 *
 *******************************************************************************
 *
 * @param[in,out] symbptr
 *          The pointer to the structure to free.
 *
 *******************************************************************************/
void
symbolExit( SymbolMatrix *symbptr )
{
    if (symbptr->cblktab != NULL)
        memFree_null (symbptr->cblktab);
    if (symbptr->bloktab != NULL)
        memFree_null (symbptr->bloktab);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_symbol
 *
 * symbolRealloc - Reallocates the data structure to optimize the memory
 * alignment.
 *
 *******************************************************************************
 *
 * @param[in,out] symbptr
 *          The pointer to the structure that needs to be reallocated.
 *
 *******************************************************************************/
void
symbolRealloc( SymbolMatrix *symbptr )
{
    SymbolCblk *cblktab = NULL;
    SymbolBlok *bloktab = NULL;

    /* Move column block array */
    MALLOC_INTERN( cblktab, symbptr->cblknbr+1, SymbolCblk );
    memcpy(cblktab, symbptr->cblktab, (symbptr->cblknbr + 1) * sizeof (SymbolCblk));
    memFree(symbptr->cblktab);
    symbptr->cblktab = cblktab;

    /* Move block array */
    MALLOC_INTERN( bloktab, symbptr->bloknbr+1, SymbolBlok );
    memcpy(bloktab, symbptr->bloktab, (symbptr->bloknbr + 1) * sizeof (SymbolBlok));
    memFree(symbptr->bloktab);
    symbptr->bloktab = bloktab;
}

/** Get face block for task E2 **/
pastix_int_t
symbolGetFacingBloknum(const SymbolMatrix *symbptr,
                       pastix_int_t bloksrc,
                       pastix_int_t bloknum,
                       pastix_int_t startsearch,
                       int ricar)
{
    SymbolBlok *bsrc;
    SymbolBlok *bdst;
    pastix_int_t i, fcblknum, fbloknum, lbloknum;

    fcblknum = symbptr->bloktab[bloksrc].cblknum;
    fbloknum = symbptr->cblktab[fcblknum].bloknum;
    lbloknum = symbptr->cblktab[fcblknum+1].bloknum;

    if(startsearch < fbloknum )
        startsearch = fbloknum;

    assert( startsearch < lbloknum );

    /* Block in original column block */
    bsrc = (symbptr->bloktab) + bloknum;

    /* Search for the facing block in the facing column block */
    bdst = (symbptr->bloktab) + startsearch;

    if(ricar == 0)
    {
        for(i=startsearch; i<lbloknum; i++, bdst++ )
            if( bdst->lrownum >= bsrc->frownum)
                break;

        /* We should always exit the loop in non ilu(k) mode */
        assert( (bdst->frownum <= bsrc->frownum) &&
                (bdst->lrownum >= bsrc->lrownum) );

        return i;
    }
    else
    {
        for(i=startsearch; i<lbloknum; i++, bdst++)
        {
            if( ((bsrc->frownum >= bdst->frownum) && (bsrc->frownum <= bdst->lrownum)) ||
                ((bsrc->lrownum >= bdst->frownum) && (bsrc->lrownum <= bdst->lrownum)) ||
                ((bsrc->frownum <= bdst->frownum) && (bsrc->lrownum >= bdst->lrownum)) )
                return i;  /** We found the first block that matches **/

            if(bsrc->lrownum < bdst->frownum)
            {
                return -1;
            }
        }
    }
    return -1;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_symbol
 *
 * symbolGetNNZ - Computes the number of non-zero elements stored in the
 * symbol matrix in order to compute the fill-in. This routines returns the
 * number of non-zero of the strictly lower part of the matrix.
 *
 *******************************************************************************
 *
 * @param[in,out] symbptr
 *          The symbol structure to study.
 *
 *******************************************************************************
 *
 * @return
 *          The number of non zero elements in the strictly lower part of the
 *          full symbol matrix.
 *
 *******************************************************************************/
pastix_int_t
symbolGetNNZ(const SymbolMatrix *symbptr)
{
    SymbolCblk *cblk;
    SymbolBlok *blok;
    pastix_int_t itercblk;
    pastix_int_t cblknbr;
    pastix_int_t nnz = 0;

    cblknbr = symbptr->cblknbr;
    cblk = symbptr->cblktab;
    blok = symbptr->bloktab;

    for(itercblk=0; itercblk<cblknbr; itercblk++, cblk++)
    {
        pastix_int_t iterblok = cblk[0].bloknum + 1;
        pastix_int_t lbloknum = cblk[1].bloknum;

        pastix_int_t colnbr = cblk->lcolnum - cblk->fcolnum + 1;

        /* Diagonal block */
        blok++;
        nnz += ( colnbr * (colnbr+1) ) / 2 - colnbr;

        /* Off-diagonal blocks */
        for( ; iterblok < lbloknum; iterblok++, blok++)
        {
            pastix_int_t rownbr = blok->lrownum - blok->frownum + 1;

            nnz += rownbr * colnbr;
        }
    }

    return nnz;
}

void
symbolPrintStats( const SymbolMatrix *symbptr )
{
    SymbolCblk *cblk;
    SymbolBlok *blok;
    pastix_int_t itercblk;
    pastix_int_t cblknbr, bloknbr;
    pastix_int_t cblkmin, cblkmax;
    pastix_int_t blokmin, blokmax;
    double cblkavg, blokavg;

    cblknbr = symbptr->cblknbr;
    bloknbr = symbptr->bloknbr - cblknbr;
    cblkmin = 99999999999;
    cblkmax = 0;
    cblkavg = 0;
    blokmin = 99999999999;
    blokmax = 0;
    blokavg = 0;

    cblk = symbptr->cblktab;
    blok = symbptr->bloktab;

    for(itercblk=0; itercblk<cblknbr; itercblk++, cblk++)
    {
        pastix_int_t iterblok = cblk[0].bloknum + 1;
        pastix_int_t lbloknum = cblk[1].bloknum;

        pastix_int_t colnbr = cblk->lcolnum - cblk->fcolnum + 1;

        cblkmin = pastix_imin( cblkmin, colnbr );
        cblkmax = pastix_imax( cblkmax, colnbr );
        cblkavg += colnbr;
        blok++;

        /* Only extra diagonal */
        for( ; iterblok < lbloknum; iterblok++, blok++)
        {
            pastix_int_t rownbr = blok->lrownum - blok->frownum + 1;

            blokmin = pastix_imin( blokmin, rownbr );
            blokmax = pastix_imax( blokmax, rownbr );
            blokavg += rownbr;
        }
    }

    cblkavg = cblkavg / (double)cblknbr;
    blokavg = blokavg / (double)bloknbr;

    fprintf(stdout,
            "------ Stats Symbol Matrix ----------\n"
            " Number of cblk  : %ld\n"
            " Number of blok  : %ld\n"
            " Cblk min width  : %ld\n"
            " Cblk max width  : %ld\n"
            " Cblk avg width  : %lf\n"
            " Blok min height : %ld\n"
            " Blok max height : %ld\n"
            " Blok avg height : %lf\n"
            "-------------------------------------\n",
            cblknbr, bloknbr,
            cblkmin, cblkmax, cblkavg,
            blokmin, blokmax, blokavg );

    fprintf(stdout,
            "& %ld & %ld & %ld & %lf & %ld & %ld & %ld & %lf\n",
            cblknbr, cblkmin, cblkmax, cblkavg,
            bloknbr, blokmin, blokmax, blokavg );
}

