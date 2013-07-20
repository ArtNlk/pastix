/**
 *
 * @file kass_build_symbol.c
 *
 *  PaStiX symbolic factorization routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Pascal Henon
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "graph.h"
#include "kass.h"

void
kassBuildSymbol(      kass_csr_t   *P,
                      pastix_int_t  cblknbr,
                const pastix_int_t *rangtab,
                      SymbolMatrix *symbmtx)
{
    pastix_int_t i, j, k, l;
    pastix_int_t cblknum;
    pastix_int_t ind;
    pastix_int_t *tmp = NULL;
    pastix_int_t *node2cblk  = NULL;
    pastix_int_t *ja         = NULL;
    pastix_int_t n, bloknbr;

    assert( cblknbr == P->n );
    n = rangtab[cblknbr];
    bloknbr = 0;

    /**** First we transform the P matrix to find the block ****/
    MALLOC_INTERN(tmp,        2*cblknbr, pastix_int_t);
    MALLOC_INTERN(node2cblk,  n,         pastix_int_t);

    for(k=0;k<cblknbr;k++)
        for(i=rangtab[k];i<rangtab[k+1];i++)
            node2cblk[i] = k;

    for(k=0; k<cblknbr; k++)
    {
        assert( P->nnz[k] >= (rangtab[k+1]-rangtab[k]) );

#if defined(PASTIX_DEBUG_SYMBOL)
        for(l=0;l<rangtab[k+1]-rangtab[k];l++)
        {
            assert( P->rows[k][l] == rangtab[k]+l );
            assert( node2cblk[P->rows[k][l]] == k );
        }
#endif

        ja  = P->rows[k];
        j   = 0;
        ind = 0;
        while( j < P->nnz[k] )
        {
            cblknum = node2cblk[ja[j]];
            l=j+1;
            while( (l < P->nnz[k])      &&
                   (ja[l] == ja[l-1]+1) &&
                   (node2cblk[ja[l]] == cblknum) )
                l++;

            tmp[ind++] = ja[j];
            tmp[ind++] = ja[l-1];
            assert( ja[l-1] == ja[j] + (l-j-1) );

            j = l;
        }

        P->nnz[k] = ind;
        bloknbr += ind / 2;

        memFree( P->rows[k] );
        MALLOC_INTERN(P->rows[k], ind, pastix_int_t);
        memcpy(P->rows[k], tmp, ind * sizeof(pastix_int_t));
    }

    memFree(tmp);

    assert( bloknbr == kass_csrGetNNZ( P ) / 2 );

#if defined(PASTIX_DEBUG_SYMBOL)
    for(k=0;k<cblknbr;k++)
    {
        assert( P->nnz[k] > 0 );
        assert( P->rows[k][0] == rangtab[k]   );
        assert( P->rows[k][0] == rangtab[k+1] );
    }
#endif

    /**********************************/
    /*** Compute the symbol matrix ****/
    /**********************************/
    symbmtx->baseval = 0;
    symbmtx->cblknbr = cblknbr;
    symbmtx->bloknbr = bloknbr;
    symbmtx->nodenbr = n;

    MALLOC_INTERN(symbmtx->cblktab, cblknbr+1,        SymbolCblk);
    MALLOC_INTERN(symbmtx->bloktab, symbmtx->bloknbr, SymbolBlok);

    ind = 0;
    for(k=0;k<cblknbr;k++)
    {
        symbmtx->cblktab[k].fcolnum = rangtab[k];
        symbmtx->cblktab[k].lcolnum = rangtab[k+1]-1;
        symbmtx->cblktab[k].bloknum = ind;

        for(i=0; i < P->nnz[k]; i+=2)
        {
            j = P->rows[k][i];
            symbmtx->bloktab[ind].frownum = j;
            symbmtx->bloktab[ind].lrownum = P->rows[k][i+1];
            symbmtx->bloktab[ind].cblknum = node2cblk[j];
            symbmtx->bloktab[ind].levfval = 0;
            ind++;

            assert( node2cblk[j] == node2cblk[ P->rows[k][i+1] ] );
        }

#ifdef DEBUG_KASS
        assert(symbmtx->bloktab[symbmtx->cblktab[k].bloknum].frownum == symbmtx->cblktab[k].fcolnum);
        assert(symbmtx->bloktab[symbmtx->cblktab[k].bloknum].lrownum == symbmtx->cblktab[k].lcolnum);
        assert(symbmtx->bloktab[symbmtx->cblktab[k].bloknum].cblknum == k);
#endif
    }

    /*  virtual cblk to avoid side effect in the loops on cblk bloks */
    symbmtx->cblktab[cblknbr].fcolnum = symbmtx->cblktab[cblknbr-1].lcolnum+1;
    symbmtx->cblktab[cblknbr].lcolnum = symbmtx->cblktab[cblknbr-1].lcolnum+1;
    symbmtx->cblktab[cblknbr].bloknum = ind;

    assert( ind == symbmtx->bloknbr );
    memFree(node2cblk);
}

void
kassPatchSymbol( SymbolMatrix *symbmtx )
{
    pastix_int_t  i, j, k;
    pastix_int_t  vroot;
    pastix_int_t *father     = NULL; /** For the cblk of the symbol matrix **/
    SymbolBlok *newbloktab = NULL;
    SymbolCblk *cblktab    = NULL;
    SymbolBlok *bloktab    = NULL;
    kass_csr_t Q;

    cblktab = symbmtx->cblktab;
    bloktab = symbmtx->bloktab;

    MALLOC_INTERN(father,     symbmtx->cblknbr,                    pastix_int_t);
    MALLOC_INTERN(newbloktab, symbmtx->cblknbr + symbmtx->bloknbr, SymbolBlok  );

    kass_csrInit( symbmtx->cblknbr, &Q );

    /*
     * Count how many extra-diagonal bloks are facing each diagonal blok
     * Double-loop to avoid diagonal blocks.
     */
    for(i=0; i<symbmtx->cblknbr; i++)
        for(j=cblktab[i].bloknum+1; j<cblktab[i+1].bloknum; j++)
            Q.nnz[ bloktab[j].cblknum ]++;

    /* Allocate nFacingBlok integer for each diagonal blok */
    for(i=0;i<symbmtx->cblknbr;i++)
    {
        MALLOC_INTERN(Q.rows[i], Q.nnz[i], pastix_int_t);
    }

    memset( Q.nnz, 0, symbmtx->cblknbr * sizeof(pastix_int_t) );

    /*
     * Q.rows[k] will contain, for each extra-diagonal facing blok
     * of the column blok k, its column blok.
     */
    for(i=0;i<symbmtx->cblknbr;i++)
    {
        for(j=cblktab[i].bloknum+1; j<cblktab[i+1].bloknum; j++)
        {
            k = bloktab[j].cblknum;
            Q.rows[k][Q.nnz[k]++] = i;
        }
    }

    for(i=0; i<Q.n; i++)
        father[i] = -1;

    for(i=0;i<Q.n;i++)
    {
        /*
         * For each blok facing diagonal blok i, belonging to column blok k.
         */
        for(j=0;j<Q.nnz[i];j++)
        {
            k = Q.rows[i][j];
            assert(k < i);

            while( (father[k] != -1) && (father[k] != i) )
                k = father[k];
            father[k] = i;
        }
    }

    for(i=0; i<Q.n; i++)
        if(father[i] == -1)
            father[i]=i+1;

    kass_csrClean(&Q);

    k = 0;
    for(i=0;i<symbmtx->cblknbr-1;i++)
    {
        pastix_int_t odb, fbloknum;

        fbloknum = cblktab[i].bloknum;
        memcpy(newbloktab+k, bloktab + fbloknum, sizeof(SymbolBlok));
        cblktab[i].bloknum = k;
        k++;
        odb = cblktab[i+1].bloknum-fbloknum;
        if(odb <= 1 || bloktab[fbloknum+1].cblknum != father[i])
        {
            /** Add a blok toward the father **/
            newbloktab[k].frownum = cblktab[ father[i] ].fcolnum;
            newbloktab[k].lrownum = cblktab[ father[i] ].fcolnum; /** OIMBE try lcolnum **/
            newbloktab[k].cblknum = father[i];
#ifdef DEBUG_KASS
            if(father[i] != i)
                assert(cblktab[father[i]].fcolnum > cblktab[i].lcolnum);
#endif

            newbloktab[k].levfval = 0;
            k++;
        }


        if( odb > 1)
        {
            memcpy(newbloktab +k, bloktab + fbloknum+1, sizeof(SymbolBlok)*(odb-1));
            k+=odb-1;
        }

    }

    /** Copy the last one **/
    memcpy(newbloktab+k, bloktab + symbmtx->cblktab[symbmtx->cblknbr-1].bloknum, sizeof(SymbolBlok));
    cblktab[symbmtx->cblknbr-1].bloknum = k;
    k++;
    /** Virtual cblk **/
    symbmtx->cblktab[symbmtx->cblknbr].bloknum = k;

#ifdef DEBUG_KASS
    assert(k >= symbmtx->bloknbr);
    assert(k < symbmtx->cblknbr+symbmtx->bloknbr);
#endif
    symbmtx->bloknbr = k;
    memFree(symbmtx->bloktab);
    MALLOC_INTERN(symbmtx->bloktab, k, SymbolBlok);
    memcpy( symbmtx->bloktab, newbloktab, sizeof(SymbolBlok)*symbmtx->bloknbr);
    /*  virtual cblk to avoid side effect in the loops on cblk bloks */
    cblktab[symbmtx->cblknbr].bloknum = k;

    memFree(father);
    memFree(newbloktab);
}
