/**
 *
 * @file graph_prepare.c
 *
 * PaStiX graph construction routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-02-15
 *
 **/
#include "common.h"
#include "graph/graph.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief This routine removes the diagonal edges from a centralized graph.
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          On entry, the pointer to the graph structure with possible diagonal
 *          edges (i,i).
 *          On exit, all entries of type (i,i) are removed from the graph.
 *
 *******************************************************************************/
void
graphNoDiag( pastix_graph_t *graph )
{
    pastix_int_t  n        = graph->n;
    pastix_int_t *colptr   = graph->colptr;
    pastix_int_t *rowptr   = graph->rowptr;
    pastix_int_t *newrow   = graph->rowptr;
    pastix_int_t *loc2glob = graph->loc2glob;
    pastix_int_t  baseval  = colptr[0];
    pastix_int_t  i, j, ig, jg, indj;

    indj = colptr[0];
    for( i = 0; i < n ; i++, colptr++, loc2glob++ )
    {
        ig = (graph->loc2glob == NULL) ? i : *loc2glob - baseval;
        for (j = colptr[0]; j < colptr[1]; j++, rowptr++ )
        {
            jg = *rowptr - baseval;
            /* If diagonal element, we skip it */
            if ( jg == ig ) {
                continue;
            }
            /* Otherwise we save it */
            else {
                *newrow = *rowptr;
                newrow++;
            }
        }
        /* Save the new colptr[i] */
        *colptr = indj;

        /* Store the new colptr[i+1] */
        indj = (newrow - graph->rowptr) + baseval;
    }
    *colptr = indj;

    graph->nnz = *colptr - *(graph->colptr);

    graph->rowptr =
        (pastix_int_t *) memRealloc ( graph->rowptr, graph->nnz * sizeof (pastix_int_t) );

    graphUpdateComputedFields( graph );
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief This routine initializes the graph.
 *
 * This routine will also symmetrize the graph, remove duplicates,
 * remove the diagonal elements, and keep only the lower part.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pointer to the solver instance. On exit, the fields n, cols,
 *          rows and loc2glob are initialized for future steps of the solver.
 *
 * @param[in] spm
 *          The initial user spm that needs to be transformed in a
 *          correct graph for future call in ordering and symbolic factorization
 *          routines.
 *
 * @param[out] graph
 *          On exit, the pointer to the allocated graph structure is returned.
 *          The graph can then be used with ordering and symbol factorization
 *          tools.
 *          The graph is symmetrized without diagonal elements and rows array is
 *          sorted.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success,
 * @retval !0 on failure.
 *
 *******************************************************************************/
int
graphPrepare(      pastix_data_t   *pastix_data,
             const spmatrix_t      *spm,
                   pastix_graph_t **graph )
{
    pastix_graph_t *tmpgraph = NULL;
    pastix_int_t   *iparm    = pastix_data->iparm;

    int io_strategy = iparm[IPARM_IO_STRATEGY];

    MALLOC_INTERN( tmpgraph, 1, pastix_graph_t );
    memset( tmpgraph, 0, sizeof(pastix_graph_t) );

    if ( iparm[IPARM_VERBOSE] > PastixVerboseNo ) {
        pastix_print( spm->clustnum, 0, "%s", OUT_SUBSTEP_GRAPH );
    }

    if (io_strategy & PastixIOLoadGraph)
    {
        graphLoad( pastix_data, tmpgraph );
    }
    else
    {
        graphSpm2Graph( tmpgraph, spm );

        /*
         * If the spm is symmetric, it only contains half of its datas.
         * We need to have all the datas for the graph.
         * An SpmGeneral has already a symmetrized pattern.
         */
        if( (spm->mtxtype == SpmSymmetric) ||
            (spm->mtxtype == SpmHermitian) ) {

            if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
                pastix_print(spm->clustnum, 0, "%s", OUT_ORDER_SYMGRAPH);
            }
            graphSymmetrize( tmpgraph );
        }

        if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
            pastix_print(spm->clustnum, 0, "%s", OUT_ORDER_SORT);
        }
        graphSort( tmpgraph );

        if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
            pastix_print(spm->clustnum, 0, "%s", OUT_ORDER_NODIAG);
        }
        graphNoDiag( tmpgraph );
    }

    assert( tmpgraph->fmttype == SpmCSC );
    assert( tmpgraph->flttype == SpmPattern );
    assert( tmpgraph->values  == NULL );

    *graph = tmpgraph;
    return PASTIX_SUCCESS;
}
