#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "cost.h"



pastix_int_t
costMatrixInit( CostMatrix *costmtx )
{
    costmtx->bloktab = NULL;
    return 1;
}

void
costMatrixExit( CostMatrix *costmtx )
{
    if(costmtx->bloktab != NULL)
	memFree_null(costmtx->bloktab);
}


void
costMatrixBuild( CostMatrix *costmtx,
                 const SymbolMatrix * symbmtx,
                 const Dof * dofptr)
{
    pastix_int_t i;

    MALLOC_INTERN(costmtx->bloktab, symbmtx->bloknbr, CostBlok);
    for(i=0;i<symbmtx->cblknbr;i++)
        cblkComputeCost(i, costmtx, symbmtx, dofptr);
}


