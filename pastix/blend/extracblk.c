#include "common.h"
#include "symbol.h"
#include "cost.h"
#include "elimin.h"
#include "cand.h"
#include "extracblk.h"

static inline void
extraCblkAlloc( ExtraCblk_t *extracblk )
{
    pastix_int_t i;

    MALLOC_INTERN( extracblk->sptcblk, extracblk->cblknbr, pastix_int_t);
    MALLOC_INTERN( extracblk->sptcbnb, extracblk->cblknbr, pastix_int_t);

    /*
     * Unsplitted cblk will keep sptcblk to -1 and sptcbnb to 1
     * Splitted cblk will have sptcbnb to the number of cblk replacing the original one,
     * and sptcblk will contain the cblktab index of the first generated cblk
     */
    for(i=0; i<extracblk->cblknbr;i++)
    {
        extracblk->sptcblk[i] = -1;
        extracblk->sptcbnb[i] =  1;
    }

    /* We choose an arbitrary size for initial allocation of bloktab and cblktab (5%) */
    extracblk->sizcblk = (extracblk->cblknbr + 20 ) / 20;
    MALLOC_INTERN( extracblk->cblktab, extracblk->cblknbr, SymbolCblk );

    return;
}

static inline pastix_int_t
extraCblkInc( ExtraCblk_t *extracblk )
{
    /* First cblk added */
    if ( extracblk->sizcblk == 0 ) {
        extraCblkAlloc( extracblk );
    }

    extracblk->curcblk++;

    /* Check that we have enough space and make it bigger if required */
    if( extracblk->curcblk >= extracblk->sizcblk )
    {
        pastix_int_t extrasize = (extracblk->cblknbr + 20 ) / 20;
        SymbolCblk *tmp;

        assert( extracblk->curcblk == extracblk->sizcblk);
        tmp = extracblk->cblktab;
        extracblk->sizcblk += extrasize;

        /* Add 5% of original cblknbr to the cblktab */
        MALLOC_INTERN( extracblk->cblktab, extracblk->cblknbr, SymbolCblk );
        memcpy(extracblk->cblktab, tmp, sizeof(SymbolCblk)*extracblk->curcblk);

        memFree_null(tmp);
    }

    return extracblk->curcblk;
}

void
extraCblkInit( pastix_int_t cblknbr,
               ExtraCblk_t *extracblk )
{
    extracblk->cblknbr = cblknbr;
    extracblk->addcblk = 0;
    extracblk->addblok = 0;
    extracblk->addblof = 0;
    extracblk->sptcblk = NULL;
    extracblk->sptcbnb = NULL;
    extracblk->curcblk = -1;
    extracblk->sizcblk = 0;
    extracblk->cblktab = NULL;
    return;
}

void
extraCblkExit( ExtraCblk_t *extracblk )
{
    if ( extracblk->sizcblk > 0 ) {
        memFree_null( extracblk->sptcblk );
        memFree_null( extracblk->sptcbnb );
        memFree_null( extracblk->cblktab );
    }
    extracblk->curcblk = -1;
    extracblk->sizcblk = 0;
    return;
}

void
extraCblkAdd( ExtraCblk_t *extracblk,
              pastix_int_t fcolnum,
              pastix_int_t lcolnum )
{
    pastix_int_t curcblk = extraCblkInc( extracblk );

    extracblk->cblktab[ curcblk ].fcolnum = fcolnum;
    extracblk->cblktab[ curcblk ].lcolnum = lcolnum;
    extracblk->cblktab[ curcblk ].bloknum = -1;
}

void
extraCblkMerge( ExtraCblk_t  *extracblk,
                SymbolMatrix *newsymb,
                Cand        **candtab )
{
    pastix_int_t  i, j, k, l;
    pastix_int_t  curbloknum, curcblknum;
    pastix_int_t  lastcblksplit;
    pastix_int_t  addblok = 0;
    pastix_int_t *newnum      = NULL;
    pastix_int_t *extranewnum = NULL;
    pastix_int_t  facing_splitted_cnt = 0;

    SymbolMatrix *oldsymb;
    Cand         *oldcand = *candtab;
    Cand         *newcand;

    SymbolCblk *curcblk;
    SymbolBlok *curblok;

    /* No splitted cblk: partition remains the same */
    if( extracblk->addcblk == 0 ) {
        return;
    }

    /* Backup the old symbol */
    MALLOC_INTERN(oldsymb, 1, SymbolMatrix);
    memcpy( oldsymb, newsymb, sizeof(SymbolMatrix) );

    /* Allocate new cblktab */
    newsymb->cblknbr = oldsymb->cblknbr + extracblk->addcblk;
    MALLOC_INTERN(newsymb->cblktab, newsymb->cblknbr+1, SymbolCblk);

    newsymb->browtab = NULL;

    /* Allocate new candtab */
    MALLOC_INTERN(newcand, newsymb->cblknbr, Cand);

    /*
     * We use the sptcbnb array to get the new numbering of the former cblk
     * in the new symbolic matrix
     * newnum[i+1] becomes the new number of the first cblk generated from the
     * split of former cblk number i.
     */
    MALLOC_INTERN(newnum,      oldsymb->cblknbr+1,   pastix_int_t);
    MALLOC_INTERN(extranewnum, extracblk->curcblk+1, pastix_int_t);
    newnum[0] = 0;
    memcpy(newnum+1, extracblk->sptcbnb, (oldsymb->cblknbr) * sizeof(pastix_int_t));

    /* Compute number of blocks that will be generated,
     * and copy main information of cblktab and candtab */
    lastcblksplit = -1;
    for(i=0; i<oldsymb->cblknbr; i++)
    {
        pastix_int_t fbloknum = oldsymb->cblktab[i  ].bloknum;
        pastix_int_t lbloknum = oldsymb->cblktab[i+1].bloknum;
        pastix_int_t sptcbnbw = extracblk->sptcbnb[i];

        /*
         * First we compute the number of extra blocks that will be generated
         */

        /* Diagonal block */
        addblok += (((sptcbnbw+1) * sptcbnbw) / 2) - 1;
        for(j=fbloknum+1; j<lbloknum; j++)
        {
            pastix_int_t fcblknum = oldsymb->bloktab[j].fcblknm;
            pastix_int_t sptfcbnb = extracblk->sptcbnb[fcblknum];
	    pastix_int_t sptcbnbh = 0;

            /* If facing cblk is splitted */
            if ( sptfcbnb > 1 )
            {
                SymbolCblk  *newfcblk =  &(extracblk->cblktab[ extracblk->sptcblk[fcblknum] ]);
                pastix_int_t frownum  = oldsymb->bloktab[j].frownum;
                pastix_int_t lrownum  = oldsymb->bloktab[j].lrownum;

                /* Compute how many times the block is splitted horizontally */
                for(k = 0; k < sptfcbnb; k++, newfcblk++)
                {
                    /* This block doesn't face this new cblk */
                    if ( frownum > newfcblk->lcolnum )
                        continue;

                    /* No more facing cblk will be found */
                    if ( lrownum < newfcblk->fcolnum )
                        break;

                    assert( frownum <= lrownum );
                    sptcbnbh++;
                    frownum = newfcblk->lcolnum+1;
                }
            }
	    else
	      sptcbnbh = 1;

            /*
             * The number of extra blocks is the number of times the block
             * is psplitted horizontally times the number of time the cblk
             * is splitted vertically minu itself
             */
            addblok += sptcbnbw * sptcbnbh - 1;
        }

        /*
         * Second, we create newnum/extranewnum arrays and copy information into
         * cblktab and candtab
         */
        {
            /* This cblk is splitted, we generate new cblktab from extra */
            pastix_int_t newcblknum = newnum[i];
            if (sptcbnbw > 1) {
                pastix_int_t nbcblk2copy = (i - lastcblksplit - 1);
                pastix_int_t sptcblk = extracblk->sptcblk[i];

                /* Copy the previous unchanged cblks from oldsymb */
                if ( nbcblk2copy > 0 ) {
                    lastcblksplit++;
                    memcpy( newsymb->cblktab + newnum[ lastcblksplit ],
                            oldsymb->cblktab + lastcblksplit,
                            nbcblk2copy * sizeof(SymbolCblk) );

                    memcpy( newcand + newnum[ lastcblksplit ],
                            oldcand + lastcblksplit,
                            nbcblk2copy * sizeof(Cand) );
                }

                /* Copy the new cblk from extracblk */
                assert( (sptcblk >= 0) && (sptcblk <= extracblk->curcblk) );
                memcpy( newsymb->cblktab   + newcblknum,
                        extracblk->cblktab + sptcblk,
                        sptcbnbw * sizeof(SymbolCblk) );

                /* Initialize extranewnum and duplicate the cand for each new cblk */
                for(j=0; j<sptcbnbw; j++, sptcblk++) {
                    extranewnum[sptcblk] = newcblknum+j;

                    assert( (extranewnum[sptcblk] >= 0) &&
                            (extranewnum[sptcblk] <  newsymb->cblknbr) );

                    memcpy( newcand + extranewnum[ sptcblk ],
                            oldcand + i, sizeof(Cand) );
                }

                lastcblksplit = i;
            }
            /* Update newnum of following cblk (newnum is allocated with one extra space) */
            newnum[i+1] += newcblknum;
        }
    }

    /* Copy last unsplitted block */
    if ( lastcblksplit < (oldsymb->cblknbr-1) )
    {
        pastix_int_t nbcblk2copy = oldsymb->cblknbr - lastcblksplit - 1;
        lastcblksplit++;
        memcpy( newsymb->cblktab + newnum[ lastcblksplit ],
                oldsymb->cblktab + lastcblksplit,
                nbcblk2copy * sizeof(SymbolCblk) );

        memcpy( newcand + newnum[ lastcblksplit ],
                oldcand + lastcblksplit,
                nbcblk2copy * sizeof(Cand) );
    }
    memFree_null(oldcand);

    /* Allocate new bloktab */
    newsymb->bloknbr = oldsymb->bloknbr + addblok;
    MALLOC_INTERN(newsymb->bloktab, newsymb->bloknbr,   SymbolBlok);

    /* Fill in the new symbolic matrix resulting from the splitting of the former one */
    curbloknum = 0;
    curcblknum = 0;
    curcblk = newsymb->cblktab;
    curblok = newsymb->bloktab;
    for(i=0; i<oldsymb->cblknbr; i++)
    {
        pastix_int_t fbloknum = oldsymb->cblktab[i  ].bloknum;
        pastix_int_t lbloknum = oldsymb->cblktab[i+1].bloknum;
        pastix_int_t sptcbnbw = extracblk->sptcbnb[i];

        /* For each new column created by the cblk split */
        for(j=0; j<sptcbnbw; j++, curcblknum++, curcblk++)
        {
            /* Store diagonal bloknum */
            curcblk->bloknum = curbloknum;

            /* Create odb due to the splitting of the diag blok */
            for(k=0; k<(sptcbnbw-j); k++, curbloknum++, curblok++)
            {
                curblok->frownum = curcblk[k].fcolnum;
                curblok->lrownum = curcblk[k].lcolnum;
                curblok->lcblknm = curcblknum;
                curblok->fcblknm = curcblknum + k;
            }
            /* Next cblk will have one block less on the diagonal */

            /* Create other off diagonal blocks */
            for(k=fbloknum+1; k<lbloknum; k++)
            {
                pastix_int_t frownum  = oldsymb->bloktab[k].frownum;
                pastix_int_t lrownum  = oldsymb->bloktab[k].lrownum;
                pastix_int_t fcblknum = oldsymb->bloktab[k].fcblknm;
                pastix_int_t sptfcblk = extracblk->sptcblk[fcblknum];
                pastix_int_t sptfcbnb = extracblk->sptcbnb[fcblknum];

                /* If facing cblk is splitted */
                if ( sptfcbnb > 1 )
                {
                    pastix_int_t newfcblknum = extranewnum[ sptfcblk ];
                    SymbolCblk  *newfcblk =  &(extracblk->cblktab[ sptfcblk ]);

                    /* Create new blocks facing this cblk */
                    for(l=0; l<sptfcbnb; l++, newfcblk++)
                    {
                        /* This block doesn't face this new cblk */
                        if ( frownum > newfcblk->lcolnum )
                            continue;

                        /* No more facing cblk will be found */
                        if ( lrownum < newfcblk->fcolnum )
                            break;

                        assert( frownum <= lrownum );
                        assert( frownum >= newfcblk->fcolnum );

                        curblok->frownum = frownum;
                        curblok->lrownum = pastix_imin( lrownum, newfcblk->lcolnum );
                        curblok->lcblknm = curcblknum;
                        curblok->fcblknm = newfcblknum + l;
                        curblok++; curbloknum++;

                        frownum = newfcblk->lcolnum+1;
                        facing_splitted_cnt++;
                    }
                    /* Remove one for previously existing block */
                    facing_splitted_cnt--;
                }
                else
                {
                    curblok->frownum = frownum;
                    curblok->lrownum = lrownum;
                    curblok->lcblknm = curcblknum;
                    curblok->fcblknm = newnum[fcblknum];
                    curblok++; curbloknum++;
                }
            }
        }
    }

    assert(curcblknum == newsymb->cblknbr);
    assert(curbloknum == newsymb->bloknbr);
    assert((curcblk - newsymb->cblktab) == newsymb->cblknbr);
    assert((curblok - newsymb->bloktab) == newsymb->bloknbr);

    /* Free old versions and temporary buffer */
    symbolExit(oldsymb);
    memFree_null(oldsymb);
    memFree_null(newnum);
    memFree_null(extranewnum);

    /* Virtual cblk to avoid side effect in the loops on cblk bloks */
    newsymb->cblktab[newsymb->cblknbr].fcolnum = newsymb->cblktab[newsymb->cblknbr-1].lcolnum+1;
    newsymb->cblktab[newsymb->cblknbr].lcolnum = newsymb->cblktab[newsymb->cblknbr-1].lcolnum+1;
    newsymb->cblktab[newsymb->cblknbr].bloknum = curbloknum;

    symbolBuildRowtab( newsymb );

    extracblk->addblok = addblok;
    extracblk->addblof = facing_splitted_cnt;

    *candtab = newcand;

    return;
}
