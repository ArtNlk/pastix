/**
 *
 * @file elimin_tree.c
 *
 *  PaStiX analyse routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * Contains basic functions to manipulate elimination tree structure.
 *
 * @version 5.1.0
 * @author Pascal Henon
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "symbol.h"
#include "elimin.h"

pastix_int_t
eTreeInit(EliminTree *etree)
{
    etree->baseval = 0;
    etree->nodenbr = 0;
    etree->nodetab = NULL;
    etree->sonstab = NULL;
    return 1;
}

void
eTreeExit(EliminTree *etree)
{
    memFree_null(etree->nodetab);
    memFree_null(etree->sonstab);
    memFree_null(etree);
}

pastix_int_t
eTreeLeaveNbr(const EliminTree *etree)
{
    pastix_int_t i;
    pastix_int_t leavenbr;
    leavenbr = 0;
    for(i=0;i<etree->nodenbr;i++)
        if(etree->nodetab[i].sonsnbr == 0)
            leavenbr++;

    return leavenbr;
}

pastix_int_t
eTreeLevel(const EliminTree *etree)
{
    pastix_int_t maxlevel;
    pastix_int_t nodelevel;
    pastix_int_t i;
    maxlevel = 0;
    for(i=0;i<etree->nodenbr;i++)
    {
        nodelevel = eTreeNodeLevel(etree, i);
        if(nodelevel>maxlevel)
            maxlevel = nodelevel;
    }

    return maxlevel;
}

pastix_int_t
eTreeNodeLevel(const EliminTree *etree, pastix_int_t nodenum )
{
    pastix_int_t level;

    level = 1;
    if(nodenum == eTreeRoot(etree))
        return level;
    level++;
    while(etree->nodetab[nodenum].fathnum != eTreeRoot(etree))
    {
        level++;
        nodenum = etree->nodetab[nodenum].fathnum;
    }
    return level;
}

void
eTreeGenDot(const EliminTree *etree, FILE *out)
{
    pastix_int_t i;

    fprintf(out,
            "digraph G {\n"
            "\tcolor=white\n"
            "rankdir=BT;\n");

    for (i=0;  i < etree->nodenbr; i++)
    {
        if ((etree->nodetab[i]).fathnum == -1)
            continue;
        fprintf(out, "\t\"%ld\"->\"%ld\"\n", (long)i, (long)((etree->nodetab[i]).fathnum));
    }

    fprintf(out, "}\n");
}

void
eTreeBuild(EliminTree *etree, const SymbolMatrix *symbmtx)
{
    pastix_int_t i;
    pastix_int_t totalsonsnbr;
    pastix_int_t sonstabcur;

    etree->nodenbr = symbmtx->cblknbr;
    MALLOC_INTERN(etree->nodetab, etree->nodenbr, TreeNode);

    /* Initialize the structure fields */
    for(i=0;i < symbmtx->cblknbr;i++)
    {
        etree->nodetab[i].sonsnbr =  0;
        etree->nodetab[i].fathnum = -1;
        etree->nodetab[i].fsonnum = -1;
    }

    totalsonsnbr = 0;
    for(i=0;i < symbmtx->cblknbr;i++)
    {
        /* If the cblk has at least one extra diagonal block,          */
        /* the father of the node is the facing block of the first odb */
        if( (symbmtx->cblktab[i+1].bloknum - symbmtx->cblktab[i].bloknum) > 1 )
        {
            etree->nodetab[i].fathnum = symbmtx->bloktab[ symbmtx->cblktab[i].bloknum+1 ].cblknum;
            eTreeFather( etree, i ).sonsnbr++;
            totalsonsnbr++;
        }
#if defined(PASTIX_DEBUG_BLEND)
        else
        {
            if(i != (symbmtx->cblknbr-1)) {
                fprintf(stderr, "Cblk %ld has no extradiagonal %ld %ld !! \n", (long)i,
                        (long)symbmtx->cblktab[i].bloknum, (long)symbmtx->cblktab[i+1].bloknum);
                assert( 0 );
            }
        }
#endif
    }

    /* Check that we have only one root */
    assert(totalsonsnbr == (symbmtx->cblknbr-1));

    if( totalsonsnbr > 0 ) {
        MALLOC_INTERN(etree->sonstab, totalsonsnbr, pastix_int_t);
    }

    /* Set the index of the first sons */
    sonstabcur = 0;
    for(i=0;i < symbmtx->cblknbr; i++)
    {
        etree->nodetab[i].fsonnum = sonstabcur;
        sonstabcur += etree->nodetab[i].sonsnbr;
    }
    assert(sonstabcur == totalsonsnbr);

    /* Fill the sonstab */
    /* No need to go to the root */
    for(i=0;i < symbmtx->cblknbr-1;i++)
    {
        etree->sonstab[ eTreeFather(etree, i).fsonnum++] = i;
    }

    /* Restore fsonnum fields */
    sonstabcur = 0;
    for(i=0;i < symbmtx->cblknbr; i++)
    {
        etree->nodetab[i].fsonnum = sonstabcur;
        sonstabcur += etree->nodetab[i].sonsnbr;
    }
    assert(sonstabcur == totalsonsnbr);
}


