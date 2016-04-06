/**
 *
 * @file order_check.c
 *
 *  PaStiX order routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * Contains function to check correctness of a ordering structure.
 *
 * @version 5.1.0
 * @author Francois Pellegrini
 * @date 2013-06-24
 *
 **/
#include "common.h"
#include "order.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_ordering
 *
 * orderCheck - This routine checks the correctness of the ordering stucture.
 *
 *******************************************************************************
 *
 * @param[in] ordeptr
 *          The ordering structure to check.
 *
 *******************************************************************************
 *
 * @return
 *          \retval PASTIX_SUCESS on successful exit.
 *          \retval PASTIX_ERR_BADPARAMETER if the ordering structure is incorrect.
 *
 *******************************************************************************/
int
orderCheck (const Order * const  ordeptr)
{
    pastix_int_t         baseval;                  /* Node base value            */
    pastix_int_t         cblkmax;                  /* Maximum supernode value    */
    pastix_int_t         vnodmax;                  /* Maximum node value         */
    pastix_int_t         vnodnum;                  /* Number of current node     */
    pastix_int_t         rangnum;                  /* Current column block index */
    const pastix_int_t * peritax;                  /* Based access to peritab    */
    const pastix_int_t * permtax;                  /* Based access to permtab    */

    /* Parameter checks */
    if ( ordeptr == NULL ) {
        errorPrint ("orderCheck: invalid ordeptr pointer");
        return PASTIX_ERR_BADPARAMETER;
    }

    if (ordeptr->cblknbr < 0) {
        errorPrint ("orderCheck: invalid nunber of column blocks");
        return PASTIX_ERR_BADPARAMETER;
    }

    baseval = ordeptr->baseval;
    if (baseval < 0) {
        errorPrint ("orderCheck: invalid vertex node base number");
        return PASTIX_ERR_BADPARAMETER;
    }

    assert(baseval == ordeptr->rangtab[0]);

    peritax = ordeptr->peritab - baseval; /* Set based accesses */
    vnodmax = ordeptr->rangtab[ordeptr->cblknbr] - 1;

    assert((ordeptr->rangtab[ordeptr->cblknbr] - baseval) == ordeptr->vertnbr);

    /**
     * Check the values in rangtab
     */
    for (rangnum = 0; rangnum < ordeptr->cblknbr; rangnum ++)
    {
        if ((ordeptr->rangtab[rangnum] <  baseval) ||
            (ordeptr->rangtab[rangnum] >  vnodmax) ||
            (ordeptr->rangtab[rangnum] >= ordeptr->rangtab[rangnum + 1]))
        {
            errorPrint ("orderCheck: invalid range array");
            return PASTIX_ERR_BADPARAMETER;
        }
    }

    permtax = ordeptr->permtab - baseval;

    /**
     * Check perm and invp, as well as the symmetry between the two
     */
    for (vnodnum = baseval;
         vnodnum <= vnodmax; vnodnum ++)
    {
        pastix_int_t vnodold;

        vnodold = peritax[vnodnum];
        if ((vnodold < baseval) ||
            (vnodold > vnodmax) ||
            (permtax[vnodold] != vnodnum))
        {
            errorPrint ("orderCheck: invalid permutation arrays");
            return PASTIX_ERR_BADPARAMETER;
        }
    }

    /**
     * Check the treetab
     */
    cblkmax = ordeptr->cblknbr - baseval;
    for (rangnum = 0; rangnum < ordeptr->cblknbr-1; rangnum ++)
    {
        if ((ordeptr->treetab[rangnum] <  baseval) ||
            (ordeptr->treetab[rangnum] >  cblkmax) ||
            (ordeptr->treetab[rangnum] <  rangnum+baseval) )
        {
            errorPrint ("orderCheck: invalid range array in treetab");
            return PASTIX_ERR_BADPARAMETER;
        }
    }
    if (ordeptr->treetab[rangnum] != -1)
    {
        errorPrint ("orderCheck: invalid father for cblknbr-1 node");
        return PASTIX_ERR_BADPARAMETER;
    }

    return PASTIX_SUCESS;
}
