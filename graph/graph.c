/**
 *
 * @file graph.c
 *
 * PaStiX graph structure routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.2
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2022-08-23
 *
 * @addtogroup pastix_graph
 * @{
 *
 **/
#include "common.h"
#include "graph/graph.h"

#define assert_graph(_graph_)                   \
    assert(_graph_->fmttype == SpmCSC);         \
    assert(_graph_->flttype == SpmPattern);     \
    assert(_graph_->values  == NULL);           \

/**
 *******************************************************************************
 *
 * @brief Free the content of the graph structure.
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          The pointer graph structure to free.
 *
 *******************************************************************************/
void
graphExit( pastix_graph_t *graph )
{
    /* Parameter checks */
    if ( graph == NULL ) {
        errorPrint("graphExit: graph pointer is NULL");
        return;
    }
    assert_graph( graph );

    spmExit( graph );

    return;
}

/**
 *******************************************************************************
 *
 * @brief Rebase the graph to the given value.
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          The graph to rebase.
 *
 * @param[in] baseval
 *          The base value to use in the graph (0 or 1).
 *
 *******************************************************************************/
void
graphBase( pastix_graph_t *graph,
           pastix_int_t    baseval )
{
    /* Parameter checks */
    if ( graph == NULL ) {
        errorPrint("graphBase: graph pointer is NULL");
        return;
    }
    if ( (graph->colptr == NULL) ||
         (graph->rowptr == NULL) )
    {
        errorPrint("graphBase: graph pointer is not correctly initialized");
        return;
    }
    if ( (baseval != 0) &&
         (baseval != 1) )
    {
        errorPrint("graphBase: baseval is incorrect, must be 0 or 1");
        return;
    }

    assert_graph(graph);

    spmBase( graph, baseval );

    return;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief This routine copy a given ordering in a new one.
 *
 * This function copies a graph structure into another one. If all subpointers
 * are NULL, then they are all allocated and contains the original graphsrc
 * values on exit. If one or more array pointers are not NULL, then, only those
 * are copied to the graphdst structure.
 *
 *******************************************************************************
 *
 * @param[inout] graphdst
 *          The destination graph
 *
 * @param[in] graphsrc
 *          The source graph
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
graphCopy( pastix_graph_t       *graphdst,
           const pastix_graph_t *graphsrc )
{
    pastix_graph_t *graph_tmp;

    /* Parameter checks */
    if ( graphdst == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( graphsrc == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( graphsrc == graphdst ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    assert_graph(graphsrc);
    assert_graph(graphdst);

    /* Clear the prexisting graph */
    graphExit( graphdst );

    /* Copy the source graph */
    graph_tmp = spmCopy( graphsrc );
    memcpy( graphdst, graph_tmp, sizeof(pastix_graph_t) );

    memFree_null( graph_tmp );

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief This routine sortes the subarray of edges of each vertex.
 *
 * WARNING: The sort is always performed, do not call this routine
 * when it is not required.
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          On entry, the pointer to the graph structure.
 *          On exit, the same graph with subarrays of edges sorted by ascending
 *          order.
 *
 *******************************************************************************/
void
graphSort( pastix_graph_t *graph )
{
    assert_graph(graph);
    spmSort(graph);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Symmetrize a given graph
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          The initialized graph structure that will be symmetrized.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success,
 * @retval PASTIX_ERR_BADPARAMETER if incorrect parameters are given.
 *
 *******************************************************************************/
int
graphSymmetrize( pastix_graph_t *graph )
{
    /* Parameter checks */
    if ( graph == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    assert_graph(graph);

    spmSymmetrize( graph );
    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Update dofs, nnz, nnzexp, gnnz, n, nexp, gN of a given graph
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          The initialized graph structure that will be updated.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success,
 * @retval PASTIX_ERR_BADPARAMETER if incorrect parameters are given.
 *
 *******************************************************************************/
int
graphUpdateComputedFields( pastix_graph_t *graph )
{
    /* Parameter checks */
    if ( graph == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    assert_graph(graph);

    spmUpdateComputedFields( graph );
    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief This routine build a graph thanks to an spm;
 *
 * This function copies an spm structure into a graph one. If all subpointers
 * are NULL, then they are all allocated and contains the original spm
 * values on exit. If one or more array pointers are not NULL, then, only those
 * are copied to the graphdst structure.
 * We will take care that our graph does not contain coefficients, therefore has
 * SpmPattern floating type and is a SpmCSC format type.
 *
 *******************************************************************************
 *
 * @param[inout] graph
 *          The destination graph.
 *
 * @param[in] graphsrc
 *          The source Sparse Matrix.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect.
 *
 *******************************************************************************/
int
graphSpm2Graph( pastix_graph_t   *graph,
                const spmatrix_t *spm )
{
    spmatrix_t *spmtmp;

    /* Parameter checks */
    if ( graph == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }
    if ( spm == NULL ) {
        return PASTIX_ERR_BADPARAMETER;
    }

    /*
     * Clear the prexisting graph
     * Might be uninitialized, so we call spmExit instead of graphExit.
     */
    spmExit( graph );

    /* Copy existing datas */
    memcpy( graph, spm, sizeof(pastix_graph_t) );

    /* A graph does not contain values */
    graph->flttype = SpmPattern;
    graph->values  = NULL;

    /* Reallocate the arrays in case of modifications */
    spmtmp = spmCopy( graph );
    memcpy( graph, spmtmp, sizeof(pastix_graph_t) );
    memFree_null( spmtmp );

    /* Make sure the graph is in CSC format */
    spmConvert( SpmCSC, graph );

    return PASTIX_SUCCESS;
}

/**
 * @}
 */
