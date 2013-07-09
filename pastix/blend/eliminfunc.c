#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <assert.h>

#include "common.h"
#include "dof.h"
#include "queue.h"
#include "ftgt.h"
#include "cost.h"
#include "symbol.h"
#include "bulles.h"
#include "csc.h"
#include "updown.h"
#include "solver.h"
#include "elimin.h"
#include "extendVector.h"
#include "cand.h"
#include "param_blend.h"
#include "blendctrl.h"
#include "simu.h"
#include "costfunc.h"
#include "eliminfunc.h"

void eliminGraphBuild(const SymbolMatrix *symbmtx, 
		      EliminGraph        *egraph)
{
    pastix_int_t i, j;
    pastix_int_t *tmp = NULL;
    pastix_int_t cursor;

    MALLOC_INTERN(tmp, symbmtx->cblknbr, pastix_int_t);
    bzero(tmp, symbmtx->cblknbr * sizeof(pastix_int_t));
  
    egraph->vertnbr = symbmtx->cblknbr;
    MALLOC_INTERN(egraph->verttab, egraph->vertnbr, EliminVertex);
    if((symbmtx->bloknbr - symbmtx->cblknbr)>0)
      {
	MALLOC_INTERN(egraph->inbltab, 
		      symbmtx->bloknbr - symbmtx->cblknbr, pastix_int_t);
      }
    else
      {
	MALLOC_INTERN(egraph->inbltab, 1, pastix_int_t);
      }
    MALLOC_INTERN(egraph->ownetab, symbmtx->bloknbr, pastix_int_t);

    /** initialize verttab **/
    for(i=0;i < egraph->vertnbr;i++)
	{
	  egraph->verttab[i].innum = -1;
	  egraph->verttab[i].innbr = -1;
	}
  
    /* fill ownetab */
    for(i=0;i < symbmtx->cblknbr; i++)
	for(j=symbmtx->cblktab[i].bloknum;j<symbmtx->cblktab[i+1].bloknum;j++)
	    egraph->ownetab[j] = i;

    /* Compute deg in for each Vertex.
       Rq innbr start at -1 because diag blok cause
       one extra in-edge */
    for(i=0;i<symbmtx->bloknbr;i++)
	{
	    egraph->verttab[symbmtx->bloktab[i].cblknum].innbr++;
#ifdef DEBUG_BLEND
	    ASSERT(symbmtx->bloktab[i].cblknum >=0,MOD_BLEND);
	    if(!(symbmtx->bloktab[i].cblknum < symbmtx->cblknbr))
	      {
		fprintf(stdout, "i %ld cblknum %ld cblknbr %ld \n", (long)i, (long)symbmtx->bloktab[i].cblknum, (long)symbmtx->cblknbr);
		EXIT(MOD_BLEND,INTERNAL_ERR);
	      }
	    /*ASSERT(symbmtx->bloktab[i].cblknum < symbmtx->cblknbr,MOD_BLEND);*/
#endif
	}
  
    /* Compute innum for each vertex */
    cursor = 0;
    for(i=0;i<egraph->vertnbr;i++)
	{
	    egraph->verttab[i].innum = cursor;
	    cursor += egraph->verttab[i].innbr;
	}
   
#ifdef DEBUG_BLEND
    ASSERT(cursor == (symbmtx->bloknbr-symbmtx->cblknbr),MOD_BLEND);
#endif

    /* Fill inbltab */
    for(i=0;i<symbmtx->bloknbr;i++)
	{
	    /* if this bloc is not a diag bloc register it*/
	    if(symbmtx->cblktab[symbmtx->bloktab[i].cblknum].bloknum != i)
		{
		    egraph->inbltab[ egraph->verttab[symbmtx->bloktab[i].cblknum].innum 
				   + tmp[symbmtx->bloktab[i].cblknum] ] = i;
		    tmp[symbmtx->bloktab[i].cblknum]++;
		}
	}
    
    memFree_null(tmp);
}

void eliminTreeBuild(const SymbolMatrix *symbmtx, BlendCtrl *ctrl)
{
    /** Build elimination tree  **/
    etreeBuild(ctrl->etree, symbmtx);
    
    /** Compute costs for all the node **/
    /* procnbr n'est pas utilis� */
    subtreeUpdateCost(ROOT(ctrl->etree), ctrl->costmtx, ctrl->etree);
    if(ctrl->option->iparm[IPARM_VERBOSE]>API_VERBOSE_NO)
	fprintf(stdout, "Total cost of the elimination tree %g \n", ctrl->costmtx->cblktab[ROOT(ctrl->etree)].subtree);
#ifdef DEBUG_BLEND
    /*fprintf(stderr, "TreeCost %lf totalCost %lf \n", ctrl->costmtx->cblktab[ROOT(ctrl->etree)].subtree,totalCost(symbmtx->cblknbr, ctrl->costmtx)); */
/*    ASSERT(ctrl->costmtx->cblktab[ROOT(ctrl->etree)].subtree == totalCost(symbmtx->cblknbr, ctrl->costmtx),MOD_BLEND);*/
#endif
}

void etreeBuild(EliminTree *etree, const SymbolMatrix *symbmtx)
{
    pastix_int_t i;
    pastix_int_t totalsonsnbr;
    pastix_int_t sonstabcur;
    pastix_int_t *tmp = NULL;
    etree->nodenbr = symbmtx->cblknbr;
    MALLOC_INTERN(etree->nodetab, etree->nodenbr, TreeNode);
    MALLOC_INTERN(tmp,            etree->nodenbr, pastix_int_t);

    bzero(tmp, etree->nodenbr * sizeof(pastix_int_t));
    /*+ initialize the structure fields +*/
    for(i=0;i < symbmtx->cblknbr;i++)
	{
	    etree->nodetab[i].sonsnbr =  0;
	    etree->nodetab[i].fathnum = -1;
	    etree->nodetab[i].fsonnum = -1;
	}
    
    totalsonsnbr = 0;

    for(i=0;i < symbmtx->cblknbr;i++)
	{
	    /*+ if the cblk has at least one extra diagonal block           +*/
	    /*+ the father of the node is the facing block of the first odb +*/
	  if( (symbmtx->cblktab[i+1].bloknum - symbmtx->cblktab[i].bloknum) != 1 )
		{
		  etree->nodetab[i].fathnum = symbmtx->bloktab[symbmtx->cblktab[i].bloknum+1].cblknum;
		  TFATHER(etree,i).sonsnbr++;
		  totalsonsnbr++;
		}
#ifdef DEBUG_BLEND
	  else
	    if(i != (symbmtx->cblknbr-1))
	      fprintf(stderr, "Cblk %ld has no extradiagonal %ld %ld !! \n", (long)i, 
		      (long)symbmtx->cblktab[i].bloknum, (long)symbmtx->cblktab[i+1].bloknum);
#endif
	}
#ifdef DEBUG_BLEND
    ASSERT(totalsonsnbr == (symbmtx->cblknbr-1),MOD_BLEND);
#endif    

    if(totalsonsnbr>0)
      {
	MALLOC_INTERN(etree->sonstab, totalsonsnbr, pastix_int_t);
      }
    else
      {
	MALLOC_INTERN(etree->sonstab, 1,            pastix_int_t);
      }
	    
   
    /** Set the index of the first sons **/
    sonstabcur = 0;
    for(i=0;i < symbmtx->cblknbr;i++)
	{
	    etree->nodetab[i].fsonnum = sonstabcur;
	    sonstabcur += etree->nodetab[i].sonsnbr;
	}

#ifdef DEBUG_BLEND
    ASSERT(sonstabcur == totalsonsnbr,MOD_BLEND);
#endif

    /** Fill the sonstab **/
    /** root needs no treatment **/
    for(i=0;i < symbmtx->cblknbr-1;i++)
	{
	    etree->sonstab[TFATHER(etree, i).fsonnum + tmp[etree->nodetab[i].fathnum]] = i;
	    tmp[etree->nodetab[i].fathnum]++;
	}

    memFree_null(tmp);
}


pastix_int_t treeLeaveNbr(const EliminTree *etree)
{
    pastix_int_t i;
    pastix_int_t leavenbr;
    leavenbr = 0;
    for(i=0;i<etree->nodenbr;i++)
	if(etree->nodetab[i].sonsnbr == 0)
	    leavenbr++;
    
    return leavenbr;
}
    
pastix_int_t treeLevel(const EliminTree *etree)
{
    pastix_int_t maxlevel;
    pastix_int_t nodelevel;
    pastix_int_t i;
    maxlevel = 0;
    for(i=0;i<etree->nodenbr;i++)
	{
	    nodelevel =  nodeTreeLevel(i, etree);
	    if(nodelevel>maxlevel)
		maxlevel = nodelevel;
	}

    return maxlevel;
}

pastix_int_t nodeTreeLevel(pastix_int_t nodenum, const EliminTree *etree)
{
    pastix_int_t level;
    
    level = 1;
    if(nodenum == ROOT(etree))
	return level;
    level++;
    while(etree->nodetab[nodenum].fathnum != ROOT(etree))
	{
	    level++;
	    nodenum = etree->nodetab[nodenum].fathnum;
	}
    return level;

}

