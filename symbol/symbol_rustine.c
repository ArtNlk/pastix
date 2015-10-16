#include "common.h"
#include "symbol.h"

/*
  Function: symbolRustine

  DESCRIPTION TO FILL

  Parameters:
    matrsymb  - Symbol matrix
    matrsymb2 - Symbol matrix
 */
void
symbolRustine (SymbolMatrix *       matrsymb,
               SymbolMatrix * const matrsymb2)
{
    pastix_int_t i,iter,add,cblknum,bloknum,bloknum2;
    SymbolBlok *bloktmp = NULL;
    SymbolCblk *cblktmp = NULL;

    MALLOC_INTERN(bloktmp, matrsymb->bloknbr+matrsymb->cblknbr, SymbolBlok);
    MALLOC_INTERN(cblktmp, matrsymb->cblknbr+1,                 SymbolCblk);
    for (i=0;i<matrsymb->cblknbr+1;i++)
    {
        cblktmp[i].fcolnum = matrsymb->cblktab[i].fcolnum;
        cblktmp[i].lcolnum = matrsymb->cblktab[i].lcolnum;
        cblktmp[i].bloknum = matrsymb->cblktab[i].bloknum;
        cblktmp[i].brownum = matrsymb->cblktab[i].brownum;
    }

    iter=0,add=0;
    for (cblknum=0;cblknum<matrsymb->cblknbr-1;cblknum++)
    {
        /* recopier le bloc diagonal */
        bloknum = matrsymb->cblktab[cblknum].bloknum;
        bloktmp[iter].frownum = matrsymb->bloktab[bloknum].frownum;
        bloktmp[iter].lrownum = matrsymb->bloktab[bloknum].lrownum;
        bloktmp[iter].lcblknm = matrsymb->bloktab[bloknum].lcblknm;
        bloktmp[iter].fcblknm = matrsymb->bloktab[bloknum].fcblknm;
        iter++;

        bloknum  = matrsymb->cblktab[cblknum].bloknum+1;
        bloknum2 = matrsymb2->cblktab[cblknum].bloknum+1;

        if (bloknum == matrsymb->cblktab[cblknum+1].bloknum)
        {
            /* pas d'extra diag */
            if (matrsymb == matrsymb2)
            {
                add++;
#ifdef RUSTIN_ADD_NEXT_CBLK
                bloktmp[iter].frownum = matrsymb->cblktab[cblknum+1].fcolnum;
                bloktmp[iter].lrownum = matrsymb->cblktab[cblknum+1].fcolnum;
                bloktmp[iter].lcblknm = cblknum;
                bloktmp[iter].fcblknm = cblknum+1;
#else
                bloktmp[iter].frownum = matrsymb->cblktab[matrsymb->cblknbr-1].fcolnum;
                bloktmp[iter].lrownum = matrsymb->cblktab[matrsymb->cblknbr-1].fcolnum;
                bloktmp[iter].lcblknm = cblknum;
                bloktmp[iter].fcblknm = matrsymb->cblknbr-1;
#endif
                iter++;
            }
            else
            {
                assert( bloknum2 != matrsymb2->cblktab[cblknum+1].bloknum );
                add++;
                bloktmp[iter].frownum = matrsymb2->bloktab[bloknum2].frownum;
                bloktmp[iter].lrownum = matrsymb2->bloktab[bloknum2].frownum;
                bloktmp[iter].lcblknm = matrsymb2->bloktab[bloknum2].lcblknm;
                bloktmp[iter].fcblknm = matrsymb2->bloktab[bloknum2].fcblknm;
                iter++;
            }
        }
        else
        {
            if (matrsymb->bloktab[bloknum].fcblknm !=
                matrsymb2->bloktab[bloknum2].fcblknm)
            {
                /* le premier extra diag ne va pas */
                add++;
                bloktmp[iter].frownum = matrsymb2->bloktab[bloknum2].frownum;
                bloktmp[iter].lrownum = matrsymb2->bloktab[bloknum2].frownum;
                bloktmp[iter].lcblknm = matrsymb2->bloktab[bloknum2].lcblknm;
                bloktmp[iter].fcblknm = matrsymb2->bloktab[bloknum2].fcblknm;
                iter++;
            }

            /* on recopie tous les blocs extra du bloc-colonne */
            for (bloknum = matrsymb->cblktab[cblknum].bloknum+1;
                 bloknum < matrsymb->cblktab[cblknum+1].bloknum; bloknum++)
            {
                bloktmp[iter].frownum = matrsymb->bloktab[bloknum].frownum;
                bloktmp[iter].lrownum = matrsymb->bloktab[bloknum].lrownum;
                bloktmp[iter].lcblknm = matrsymb->bloktab[bloknum].lcblknm;
                bloktmp[iter].fcblknm = matrsymb->bloktab[bloknum].fcblknm;
                iter++;
            }
        }

        cblktmp[cblknum+1].bloknum += add;
    }

    bloktmp[iter].frownum = matrsymb->cblktab[matrsymb->cblknbr-1].fcolnum;
    bloktmp[iter].lrownum = matrsymb->cblktab[matrsymb->cblknbr-1].lcolnum;
    bloktmp[iter].lcblknm = matrsymb->cblknbr-1;
    bloktmp[iter].fcblknm = matrsymb->cblknbr-1;
    cblktmp[matrsymb->cblknbr].bloknum+=add;

    memFree_null(matrsymb->bloktab);
    memFree_null(matrsymb->cblktab);
    matrsymb->bloktab = bloktmp;
    matrsymb->cblktab = cblktmp;
    matrsymb->bloknbr += add;
    assert( add < matrsymb->cblknbr );
}
