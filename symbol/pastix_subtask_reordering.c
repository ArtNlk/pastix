/**
 *
 * @file pastix_subtask_reordering.c
 *
 * PaStiX reordering task
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2015-04
 *
 */
#include "common.h"
#include "order.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_analyze
 *
 * @brief Apply the reordering step to compact off-diagonal blocks
 *
 * During this step, unknowns are reordered within each supernode to compact
 * off-diagonal blocks.  It takes as input the ordering provided by
 * pastix_subtask_order(), and the symbolic factorization computed by
 * pastix_subtask_symbfact(). It returns boths structures Order and Symbol updated.
 *
 * This function reorders the unknowns of the problem based on traveller
 * salesman problem to gather together the contibutions facing each supernodes.
 *
 * See reordering paper
 * @todo Put link to the paper when published
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_IO_STRATEGY, IPARM_REORDERING_SPLIT,
 *   IPARM_REORDERING_STOP
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.  On
 *          exit, the field symbmtx is updated with the new symbol matrix, and
 *          the field ordemesh is updated with the new ordering.
 *          - IPARM_IO_STRATEGY will enable to load/store the result to files.
 *          If set to PastixIOSave, the symbmtx and the generated ordemesh are
 *          dump to file.
 *          If set to PastixIOLoad, the symbmtx (only) is loaded from the files.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
pastix_subtask_reordering( pastix_data_t *pastix_data )
{
    Clock         timer;
    pastix_int_t *iparm;
    Order        *ordemesh;
    pastix_int_t  procnum;
    int verbose;

    /**
     * Check parameters
     */
    if (pastix_data == NULL) {
        errorPrint("pastix_subtask_reordering: wrong pastix_data parameter");
        return PASTIX_ERR_BADPARAMETER;
    }
    iparm    = pastix_data->iparm;
    procnum  = pastix_data->procnum;
    ordemesh = pastix_data->ordemesh;

    assert(ordemesh->rangtab);
    assert(ordemesh->treetab);

    /* Start the step */
    if (iparm[IPARM_VERBOSE] > PastixVerboseNot ) {
        pastix_print(procnum, 0, OUT_STEP_REORDER,
                     (long)iparm[IPARM_REORDERING_SPLIT],
                     (long)iparm[IPARM_REORDERING_STOP]);
    }

    /* Print the reordering complexity */
    if (iparm[IPARM_VERBOSE] > PastixVerboseYes)
        symbolReorderingPrintComplexity( pastix_data->symbmtx );

    clockStart(timer);

    /**
     * Reorder the rows of each supernode in order to compact coupling blocks
     */
    symbolReordering( pastix_data->symbmtx, ordemesh,
                      iparm[IPARM_REORDERING_SPLIT],
                      iparm[IPARM_REORDERING_STOP] );

    /* Backup the new ordering */
    if ( iparm[IPARM_IO_STRATEGY] & PastixIOSave )
    {
        if (procnum == 0) {
            orderSave( ordemesh, NULL );
        }
    }

    symbolExit(pastix_data->symbmtx);
    memFree_null(pastix_data->symbmtx);
    pastix_data->symbmtx = NULL;

    /* Re-build the symbolic structure */
    /* TODO: Create a function to update the symbolic factorization, instead of computing it from scratch,
     this can be easily made in // per column, as opposed to the symbolic factorization itself */
    verbose = iparm[IPARM_VERBOSE];
    iparm[IPARM_VERBOSE] = pastix_imax( 0, verbose-2 );

    pastix_subtask_symbfact( pastix_data, NULL, NULL );

    iparm[IPARM_VERBOSE] = verbose;

#if !defined(NDEBUG)
    if ( orderCheck( ordemesh ) != 0) {
        errorPrint("pastix_subtask_reordering: orderCheck on final ordering failed !!!");
        assert(0);
    }
    if( symbolCheck(pastix_data->symbmtx) != 0 ) {
        errorPrint("pastix_subtask_reordering: symbolCheck on final symbol matrix failed !!!");
        assert(0);
    }
#endif

    clockStop(timer);
    if ( iparm[IPARM_VERBOSE] > PastixVerboseNot ) {
        pastix_print(procnum, 0, OUT_REORDERING_TIME,
                     (double)clockVal(timer));
    }
    return PASTIX_SUCCESS;
}
