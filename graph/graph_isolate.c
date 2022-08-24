/**
 *
 * @file graph_isolate.c
 *
 * PaStiX graph isolate routine
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-03-30
 *
 **/
#include "common.h"
#include "pastix/order.h"
#include "blend/extendVector.h"
#include "graph/graph.h"

/**
 * @brief Assign the new pointer to the temporary one
 */
static inline void
graph_isolate_assign_ptr( pastix_int_t **newptr,
                          pastix_int_t  *tmpptr )
{
    if ( newptr != NULL ) {
        *newptr = tmpptr;
    } else {
        memFree_null( tmpptr );
    }
}

/**
 * @brief If isolate_n == n, everything needs to be isolated.
 *        That means that all the old arrays need to be copied
 *        in the new ones.
 */
static inline void
graph_isolate_everything(       pastix_int_t   n,
                                pastix_int_t   nnz,
                          const pastix_int_t  *oldcol,
                          const pastix_int_t  *oldrow,
                                pastix_int_t **newcol,
                                pastix_int_t **newrow,
                                pastix_int_t **newperm,
                                pastix_int_t **newinvp )
{
    pastix_int_t i;

    if ( newcol != NULL ) {
        MALLOC_INTERN( *newcol, (n+1), pastix_int_t );
        memcpy( *newcol, oldcol, (n+1) * sizeof(pastix_int_t) );
    }
    if( newrow != NULL ) {
        MALLOC_INTERN( *newrow, nnz, pastix_int_t );
        memcpy( *newrow, oldrow, nnz * sizeof(pastix_int_t) );
    }
    if( newperm != NULL ) {
        MALLOC_INTERN( *newperm, n, pastix_int_t );
        for (i = 0; i < n; i++) {
            (*newperm)[i] = i;
        }
    }
    if( newinvp != NULL ) {
        MALLOC_INTERN( *newinvp, n, pastix_int_t );
        for (i = 0; i < n; i++) {
            (*newinvp)[i] = i;
        }
    }
}

/**
 * @brief Init and fill the inverse permutation array.
 */
static inline void
graph_isolate_permutations(       pastix_int_t  n,
                                  pastix_int_t  isolate_n,
                            const pastix_int_t *isolate_list,
                                  pastix_int_t *permtab,
                                  pastix_int_t *invptab,
                                  pastix_int_t  baseval )
{
    pastix_int_t *invp_isolate, *invp_nonisol;
    pastix_int_t  i;

    /* First, fill inverse permutations array */
    invp_nonisol = invptab;
    invp_isolate = invptab + (n - isolate_n);
    for (i = 0; i <n; i++)
    {
        if ( i == (*isolate_list - baseval) ) {
            *invp_isolate = i;
            invp_isolate++;
            isolate_list++;
        }
        else {
            *invp_nonisol = i;
            invp_nonisol++;
        }
    }
    assert( (invp_nonisol - invptab) == (n - isolate_n) );
    assert( (invp_isolate - invptab) ==  n );

    /* Second, fill permutation array */
    invp_nonisol = invptab;
    for( i = 0; i < n; i++, invp_nonisol++ )
    {
        permtab[ *invp_nonisol ] = i;
    }

#if defined(PASTIX_DEBUG_GRAPH)
    for(i = 0; i < n; i++)
    {
        assert(permtab[i] < n );
        assert(permtab[i] > -1);
    }
#endif
}

/**
 * @brief Init and fill the new column array.
 */
static inline void
graph_isolate_init_newcol(       pastix_int_t  n,
                           const pastix_int_t *colptr,
                           const pastix_int_t *rowptr,
                                 pastix_int_t  new_n,
                                 pastix_int_t *newcol,
                           const pastix_int_t *permtab,
                                 pastix_int_t  baseval )
{
    pastix_int_t  i, j, ip;

    newcol[0] = baseval;
    for ( i = 0; i < n; i++, colptr++ )
    {
        ip = permtab[i];
        if( ip >= new_n ) {
            rowptr += (colptr[1] - colptr[0]);
            continue;
        }

        for( j = colptr[0]; j < colptr[1]; j++, rowptr++ )
        {
            /* Count edges in each column of the new graph */
            if( permtab[ *rowptr - baseval ] < new_n ) {
                newcol[ip+1]++;
            }
        }
    }

    for (i = 0; i < new_n; i++)
    {
        newcol[i+1] += newcol[i];
    }
}

/**
 * @brief Init and fill the new row array.
 */
static inline void
graph_isolate_init_newrow(       pastix_int_t  n,
                           const pastix_int_t *colptr,
                           const pastix_int_t *rowptr,
                                 pastix_int_t  new_n,
                           const pastix_int_t *newcol,
                                 pastix_int_t *newrow,
                           const pastix_int_t *permtab,
                                 pastix_int_t  baseval )
{
    pastix_int_t  i, j, ip, index, row;

    for( i = 0; i < n; i++, colptr++ )
    {
        ip = permtab[i];
        if( ip >= new_n ) {
            rowptr += (colptr[1] - colptr[0]);
            continue;
        }

        index = newcol[ip] - baseval;
        for( j = colptr[0]; j < colptr[1]; j++, rowptr++ )
        {
            row = permtab[ *rowptr-baseval ];
            /* Count edges in each column of the new graph */
            if( row < new_n ) {
                newrow[index] = row + baseval;
                index++;
            }
        }
        assert( index == (newcol[ip+1]-baseval) );
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Isolate a subset of vertices from a given graph.
 *
 * Return a new graph cleaned from those vertices.
 *
 *******************************************************************************
 *
 * @param[in] n
 *          The number of columns of the original GRAPH matrix.
 *
 * @param[in] colptr
 *          Array of size n+1.
 *          Index of first element of each column in rowptr array.
 *
 * @param[in] rowptr
 *          Array of size nnz = colptr[n] - colptr[0].
 *          Rows of each non zero entries.
 *
 * @param[in] isolate_n
 *          The number of columns to isolate from the original graph.
 *
 * @param[inout] isolate_list
 *          Array of size isolate_n.
 *          List of columns to isolate. On exit, the list is sorted by ascending
 *          indexes. Must be based as the graph.
 *
 * @param[out] new_colptr
 *          Array of size n-isolate_n+1.
 *          Index of first element of each column in rows array for the new graph.
 *          If new_colptr == NULL, nothing is returned, otherwise the pointer to
 *          the allocated structure based as the input colptr.
 *
 * @param[out] new_rowptr
 *          Array of size new_nnz = (*new_colptr)[n] - (*new_colptr)[0].
 *          Rows of each non zero entries for the new graph.
 *          If new_rows == NULL, nothing is returned, otherwise the pointer to
 *          the allocated structure based as the input rows.
 *
 * @param[out] new_perm
 *          Array of size n-isolate_n.
 *          Contains permutation generated to isolate the columns at the end of
 *          the graph that is 0-based.
 *          If new_perm == NULL, nothing is returned, otherwise the pointer to
 *          the allocated structure.
 *
 * @param[out] new_invp
 *          Array of size n-isolate_n.
 *          Contains the inverse permutation generated to isolate the columns
 *          at the end of the graph that is 0-based.
 *          If new_invp == NULL, nothing is returned, otherwise the pointer to
 *          the allocated structure.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success,
 * @retval PASTIX_ERR_ALLOC if allocation went wrong,
 * @retval PASTIX_ERR_BADPARAMETER if incorrect parameters are given.
 *
 *******************************************************************************/
int graphIsolate(       pastix_int_t   n,
                  const pastix_int_t  *colptr,
                  const pastix_int_t  *rowptr,
                        pastix_int_t   isolate_n,
                        pastix_int_t  *isolate_list,
                        pastix_int_t **new_colptr,
                        pastix_int_t **new_rowptr,
                        pastix_int_t **new_perm,
                        pastix_int_t **new_invp )
{
    pastix_int_t *tmpcolptr = NULL;
    pastix_int_t *tmprowptr = NULL;
    pastix_int_t *tmpperm   = NULL;
    pastix_int_t *tmpinvp   = NULL;
    pastix_int_t  baseval   = colptr[0];
    pastix_int_t  nnz       = colptr[n] - baseval;
    pastix_int_t  new_n     = n - isolate_n;
    pastix_int_t  new_nnz;

    if ( (isolate_n > n)  || (isolate_n < 0) ) {
        errorPrintW("Number of columns to isolate greater than the columns in the GRAPH matrix\n");
        return PASTIX_ERR_BADPARAMETER;
    }

    /* For the moment, make sure the graph is 0 based */
    assert( baseval == 0 );

    /* Quick Return */
    if (isolate_n == 0) {
        if (new_colptr != NULL) *new_colptr = (pastix_int_t*)colptr;
        if (new_rowptr != NULL) *new_rowptr = (pastix_int_t*)rowptr;
        return PASTIX_SUCCESS;
    }

    /* We isolate the whole graph */
    if (isolate_n == n) {
        graph_isolate_everything( n, nnz, colptr, rowptr,
                                  new_colptr, new_rowptr, new_perm, new_invp );
        return PASTIX_SUCCESS;
    }

    /* Sort the lost of vertices */
    intSort1asc1(isolate_list, isolate_n);

    /* Init invp and perm array */
    MALLOC_INTERN(tmpinvp, n, pastix_int_t);
    MALLOC_INTERN(tmpperm, n, pastix_int_t);
    graph_isolate_permutations( n, isolate_n, isolate_list,
                                tmpperm, tmpinvp, baseval );

    /* Create the new_colptr array */
    MALLOC_INTERN(tmpcolptr, new_n + 1, pastix_int_t);
    memset(tmpcolptr, 0, (new_n + 1)*sizeof(pastix_int_t));
    graph_isolate_init_newcol( n, colptr, rowptr,
                               new_n, tmpcolptr, tmpperm, baseval );

    new_nnz = tmpcolptr[new_n] - tmpcolptr[0];

    /* Create the new rowptr array */
    if ( new_nnz != 0 ) {
        MALLOC_INTERN(tmprowptr, new_nnz, pastix_int_t);
        graph_isolate_init_newrow( n, colptr, rowptr,
                                   new_n, tmpcolptr, tmprowptr,
                                   tmpperm, baseval );
    }

    graph_isolate_assign_ptr( new_colptr, tmpcolptr );
    graph_isolate_assign_ptr( new_rowptr, tmprowptr );
    graph_isolate_assign_ptr( new_perm, tmpperm );
    graph_isolate_assign_ptr( new_invp, tmpinvp );

    return PASTIX_SUCCESS;
}

/**
 * @brief Fill the isolated colptr and rowptr.
 */
static inline void
graph_iRange_fill_outptr( const pastix_graph_t  *graph,
                          const pastix_order_t  *order,
                                ExtendVectorINT *vec,
                                pastix_int_t    *out_colptr,
                                pastix_int_t    *out_rowptr,
                                pastix_int_t     fnode,
                                pastix_int_t     lnode,
                                pastix_int_t     distance )
{
    pastix_int_t *out_connected;
    pastix_int_t *colptr ,*rowptr, *rowtmp;
    pastix_int_t *perm   = order->permtab;
    pastix_int_t *invp   = order->peritab;
    pastix_int_t  out_n  = lnode - fnode;
    pastix_int_t  ip, jp, k, jj, i, j;
    pastix_int_t  d, sze, baseval;

    MALLOC_INTERN( out_connected, out_n, pastix_int_t );

    /* The first loop counts the number of edges */
    baseval = graph->baseval;
    colptr  = graph->colptr;
    rowptr  = graph->rowptr - baseval;
    rowtmp  = out_rowptr;
    for( ip=fnode; ip<lnode; ip++ )
    {
        /* Clear the previous computations */
        extendint_Clear( vec );
        memset(out_connected, 0, out_n * sizeof(pastix_int_t));
        out_connected[ip-fnode] = 1;

        /* i^th vertex in the initial numbering */
        extendint_Add( vec, invp[ip] );
        sze =  1;
        d   = -1;
        k   =  0;

        while( d < distance ) {
            for(; k<sze; k++) {
                i = extendint_Read( vec, k );

                for (jj = colptr[i]; jj < colptr[i+1]; jj++)
                {
                    j  = rowptr[jj] - baseval;
                    jp = perm[j];

                    /* Count edges in each column of the new graph */
                    if ( ( jp >= fnode ) && ( jp < lnode ) ) {
                        if (out_connected[jp-fnode] == 0){
                            out_connected[jp-fnode] = 1;

                            /* Fill the out_colptr */
                            out_colptr[ip-fnode+1]++;

                            /* Fill the out_rowptr */
                            *rowtmp = jp-fnode;
                            rowtmp++;
                        }
                    }
                    else {
                        extendint_Add( vec, j );
                    }
                }
            }
            d++;
            sze = extendint_Size( vec );
        }
    }
    free(out_connected);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Isolate the subgraph associated to a range of unknowns in the permuted
 * graph.
 *
 * This routine isolates a continuous subset of vertices from a given graph, and
 * returns a new graph made of those vertices and internal connexions. Extra
 * edges are created between vertices if they are connected through a halo at a
 * distance d given in parameter.
 *
 *******************************************************************************
 *
 * @param[in] graph
 *          The original graph associated from which vertices and edges must be
 *          extracted.
 *
 * @param[in] order
 *          The ordering structure associated to the graph.
 *
 * @param[inout] out_graph
 *          The extracted graph. If the graph is allocated, it is freed before usage.
 *          On exit, contains the subgraph of the vertices invp[fnode] to invp[lnode-1].
 *
 * @param[in] fnode
 *          The index of the first node to extract in the inverse permutation.
 *
 * @param[in] lnode
 *          The index (+1) of the last node to extract in the inverse permutation.
 *
 * @param[in] distance
 *          Distance considered in number of edges to create an edge in isolated
 *          graph.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success.
 * @retval PASTIX_ERR_ALLOC if allocation went wrong.
 * @retval PASTIX_ERR_BADPARAMETER if incorrect parameters are given.
 *
 *******************************************************************************/
int
graphIsolateRange( const pastix_graph_t *graph,
                   const pastix_order_t *order,
                         pastix_graph_t *out_graph,
                         pastix_int_t    fnode,
                         pastix_int_t    lnode,
                         pastix_int_t    distance )
{
    ExtendVectorINT vec;
    pastix_int_t   *out_rowptr;
    pastix_int_t   *out_colptr;
    pastix_int_t    baseval = graph->baseval;
    pastix_int_t    n       = graph->n;
    pastix_int_t    out_n   = lnode - fnode;
    pastix_int_t    i;

    if ( out_graph == NULL ) {
        errorPrintW( "graphIsolateSupernode: Incorrect pointer for the output graph\n");
        return PASTIX_ERR_BADPARAMETER;
    }

    n             = graph->n;
    out_graph->n  = out_n;

    /* Copy dofs datas, as they are global */
    out_graph->dof  = graph->dof;
    if( out_graph->dof < 0 ) {
        MALLOC_INTERN( out_graph->dofs, graph->gN, pastix_int_t );
        memcpy( out_graph->dofs, graph->dofs, graph->gN * sizeof(pastix_int_t) );
    }

    if ( out_n == 0 ) {
        errorPrintW( "graphIsolateSupernode: Empty supernode\n");
        return PASTIX_ERR_BADPARAMETER;
    }

    /* For the moment, make sure the graph is 0 based */
    assert( baseval == 0 );

    /* Quick Return */
    if ( out_n == n ) {
        /* Only one supernode */
        assert( order->cblknbr == 1 );
        graphCopy( out_graph, graph );
        return PASTIX_SUCCESS;
    }

    /* Create the new_colptr array */
    MALLOC_INTERN( out_graph->colptr, out_n + 1, pastix_int_t );
    memset( out_graph->colptr, 0, (out_n + 1) * sizeof(pastix_int_t) );
    out_colptr = out_graph->colptr;

    /* Create the new_rowptr array, will be reallocated later */
    MALLOC_INTERN( out_graph->rowptr, graph->nnz, pastix_int_t );
    out_rowptr = out_graph->rowptr;

    /*
     * (i,j) in permutated ordering
     * (ip,jp) in initial ordering
     */
    out_graph->baseval = baseval;
    out_colptr[0]      = baseval;

    extendint_Init( &vec, 100 );

    /* Fill the out_colptr and the out_rowptr */
    graph_iRange_fill_outptr( graph, order, &vec,
                              out_colptr, out_rowptr,
                              fnode, lnode, distance );

    /* Update the colptr */
    for (i = 0; i < out_n; i++){
        out_colptr[i+1] += out_colptr[i];
    }

    out_graph->nnz = out_colptr[out_n] - out_colptr[0];

    /* Allocation will fail if matrix is diagonal and no off-diagonal elements are found */
    if ( out_graph->nnz == 0 ){
        fprintf( stderr, "Diagonal matrix cannot be correcly managed here!\n" );
        return EXIT_FAILURE;
    }

    /* Create the new rowptr array */
    out_graph->rowptr =
        (pastix_int_t *) memRealloc( out_graph->rowptr, out_graph->nnz * sizeof(pastix_int_t) );

    extendint_Exit( &vec );
    graphBase( out_graph, 0 );

    /* Update mainly gN and gnnz */
    graphUpdateComputedFields( out_graph );

    return PASTIX_SUCCESS;
}
