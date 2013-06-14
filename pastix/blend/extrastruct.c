#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "cost.h"
#include "symbol.h"
#include "extrastruct.h"




pastix_int_t extrasymbolInit(ExtraSymbolMatrix *extrasymb)
{
  extrasymb->baseval = 0;
  extrasymb->addcblk = 0;
  extrasymb->addblok = 0;
  extrasymb->sptcblk = NULL;
  extrasymb->sptblok = NULL;
  extrasymb->sptblnb = NULL;
  extrasymb->sptcbnb = NULL;
  extrasymb->subtreeblnbr = NULL;
  extrasymb->curcblk = 0;
  extrasymb->sizcblk = 0;
  extrasymb->curblok = 0;
  extrasymb->sizblok = 0;
  extrasymb->cblktab = NULL;
  extrasymb->bloktab = NULL;
  return 1;
}

void extrasymbolExit(ExtraSymbolMatrix *extrasymb)
{
  memFree_null(extrasymb->sptcblk);
  memFree_null(extrasymb->sptcbnb);
  memFree_null(extrasymb->sptblok);
  memFree_null(extrasymb->sptblnb);
  memFree_null(extrasymb->subtreeblnbr);
  if(extrasymb->sizcblk > 0)
    memFree_null(extrasymb->cblktab);
  if(extrasymb->sizblok > 0)
    memFree_null(extrasymb->bloktab);
  memFree_null(extrasymb);
}

pastix_int_t extracostInit(ExtraCostMatrix *extracost)
{
    extracost->cblktab = NULL;
    extracost->bloktab = NULL;
    return 1;
}

void extracostExit(ExtraCostMatrix *extracost)
{
    if(extracost->cblktab != NULL)
	memFree_null(extracost->cblktab);
    if(extracost->bloktab != NULL)
	memFree_null(extracost->bloktab);
    memFree_null(extracost);
}


void extra_inc_blok(ExtraSymbolMatrix *extrasymb, ExtraCostMatrix *extracost)
{
    extrasymb->curblok++;
    
    /** if extra-blokktab is not big enough, make it bigger !! **/
    if(extrasymb->curblok + 1 >= extrasymb->sizblok)
	{
	    SymbolBlok *tmp;
	    CostBlok   *tmp2;
	    tmp2 = extracost->bloktab;
	    tmp  = extrasymb->bloktab;
	    /* add memory space to extra symbol matrix */ 
	    MALLOC_INTERN(extrasymb->bloktab, 
			  extrasymb->sizblok + extrasymb->sizblok/2 + 1, 
			  SymbolBlok);
	    memcpy(extrasymb->bloktab, tmp, sizeof(SymbolBlok)*extrasymb->curblok);
	    /* add memory space to extra cost matrix */
	    MALLOC_INTERN(extracost->bloktab, 
			  extrasymb->sizblok + extrasymb->sizblok/2 + 1, 
			  CostBlok);
	    /*fprintf(stderr, "Size %ld curblok %ld NewSize %ld \n", extrasymb->sizblok, extrasymb->curblok, (extrasymb->sizblok + extrasymb->sizblok/2 +1));
	    ASSERT( extracost->bloktab != NULL,MOD_BLEND);*/
	    memCpy(extracost->bloktab, tmp2, sizeof(CostBlok)*extrasymb->curblok);

	    extrasymb->sizblok = extrasymb->sizblok + extrasymb->sizblok/2 + 1;
	    memFree_null(tmp);
	    memFree_null(tmp2);
	}
}

void extra_inc_cblk(ExtraSymbolMatrix *extrasymb, ExtraCostMatrix *extracost)
{
    extrasymb->curcblk++;
    /** if extra-cblktab is not big enough, make it bigger !! **/
    if(extrasymb->curcblk + 1 >= extrasymb->sizcblk)
	{
	    SymbolCblk *tmp;
	    CostCblk   *tmp2;
	    tmp  = extrasymb->cblktab;
	    tmp2 = extracost->cblktab;
	    /* add memory space to extra symbol matrix */ 
	    MALLOC_INTERN(extrasymb->cblktab,
			  extrasymb->sizcblk + extrasymb->sizcblk/5 + 1,
			  SymbolCblk);
	    memcpy(extrasymb->cblktab, tmp, sizeof(SymbolCblk)*extrasymb->curcblk);
	    /* add memory space to extra cost matrix */
	    MALLOC_INTERN(extracost->cblktab, 
			  extrasymb->sizcblk + extrasymb->sizcblk/5 + 1,
			  CostCblk);
	    memcpy(extracost->cblktab, tmp2, sizeof(CostCblk)*extrasymb->curcblk);

	    extrasymb->sizcblk = extrasymb->sizcblk + extrasymb->sizcblk/5 + 1;
	    memFree_null(tmp);
	    memFree_null(tmp2);
	}
}
