/**
 *
 * @file graph_compute_kway.c
 *
 * PaStiX routines to compute kway on a given supernode
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-01-25
 *
 **/
#include "common.h"
#if !defined(PASTIX_ORDERING_SCOTCH)
#error "This file should be compiled inly if SCOTCH is enabled and found"
#endif /* defined(PASTIX_ORDERING_PTSCOTCH) */
#include <spm.h>
#include "graph/graph.h"
#include "pastix/order.h"
#include <scotch.h>

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Compute the K-way partition of a given set of unknowns
 *
 * Take a graph with its own associated ordering, and generates a k-way
 * partition of the graph.
 *
 *******************************************************************************
 *
 * @param[in] graph
 *          The pointer to the graph structure on wich to apply the K-way
 *          partitioning on the subcomponent comp_id.
 *
 * @param[inout] order
 *          The order structure associated to the given graph.
 *          On entry, the structure is initialized through previous operation on
 *          the graph, and on exit vertices are reordered by components in the
 *          K-way partitioning.
 *
 * @param[inout] peritab
 *          Array of size n.
 *          Pointer to the peritab array of the global graph in which the
 *          subproblem belongs to. This array is updated according to the
 *          eprmutation performed for the K-way partitioning.
 *
 * @param[inout] comp_nbr
 *          On entry the number of components in the graph.
 *          On exit, the new number of components after K-way partitioning.
 *
 * @param[inout] comp_size
 *          The size of each components in the graph.
 *          On entry, only the first comp_nbr sizes are initialized, others are 0.
 *          On exit, the array is extended to store the size of the new
 *          components generated by the k-way partitionning.
 *
 * @param[inout] comp_vtx
 *          Array of size n that hold the index of the component for each vertex
 *          of the graph.
 *
 * @param[in] comp_id
 *          The index of the component that must be split by K-way paritioning.
 *
 * @param[in] nbpart
 *          The number of part wanted in the k-way partitioning.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on success,
 * @retval PASTIX_ERR_UNKNOWN if something went wrong with Scotch.
 *
 *******************************************************************************/
int graphComputeKway( const pastix_graph_t *graph,
                      pastix_order_t       *order,
                      pastix_int_t         *peritab,
                      pastix_int_t         *comp_nbr,
                      pastix_int_t         *comp_sze,
                      pastix_int_t         *comp_vtx,
                      pastix_int_t          comp_id,
                      pastix_int_t          nbpart )
{
    SCOTCH_Graph   comp_sgraph;
    SCOTCH_Strat   sstrat;
    pastix_graph_t comp_graph;
    pastix_int_t  i, n, comp_n, fnode, lnode;
    pastix_int_t *perm = order->permtab;
    pastix_int_t *invp = order->peritab;
    pastix_int_t *parttab;

    n = graph->n;

    fnode = 0;
    for (i=0; i<comp_id; i++) {
        fnode += comp_sze[i];
    }
    lnode = fnode + comp_sze[comp_id];
    assert( comp_sze[comp_id] != 0 );
    assert( lnode <= n );

    comp_n = lnode - fnode;

    /* Isolate unknowns of the component */
    if ( comp_n == n ) {
        memcpy( &comp_graph, graph, sizeof(pastix_graph_t) );
    }
    else {
        /* First, let's make sure everything is sorted by component to extract it */
        {
            void *sortptr[3];
            sortptr[0] = comp_vtx;
            sortptr[1] = invp;
            sortptr[2] = peritab;

            qsort3IntAsc( sortptr, n );

            /* Update the perm array */
            for(i=0; i<n; i++) {
                perm[invp[i]] = i;
            }

            /* pastixOrderCheck( order ); */
        }
        memset( &comp_graph, 0, sizeof(pastix_graph_t) );
        graphIsolateRange( graph, order, &comp_graph,
                           fnode, lnode, 0 );
    }
    assert( comp_graph.n == comp_n );

    /* Build Scotch graph */
    if ( ! SCOTCH_graphInit( &comp_sgraph ) ) {
        SCOTCH_graphBuild( &comp_sgraph,
                           order->baseval,
                           comp_n,
                           comp_graph.colptr,
                           NULL,
                           NULL,
                           NULL,
                           comp_graph.colptr[ comp_n ] - comp_graph.colptr[ 0 ],
                           comp_graph.rowptr,
                           NULL);
    }
    else {
        fprintf(stderr,"Failed to build graph\n");
    }

#if !defined(NDEBUG)
    if ( SCOTCH_graphCheck( &comp_sgraph ) ) {
        pastix_print_error( "error in graph graphCheck()...\n" );
        return PASTIX_ERR_BADPARAMETER;
    }
#endif

    if ( SCOTCH_stratInit( &sstrat ) != 0 ) {
        pastix_print_error( "Failed to initialize partitioning strategy\n" );
        return PASTIX_ERR_UNKNOWN;
    }

    parttab = malloc( comp_n * sizeof(pastix_int_t) );
    memset( parttab, 0, comp_n * sizeof(pastix_int_t) );

    SCOTCH_graphPart( &comp_sgraph, nbpart,
                      &sstrat, parttab );

    SCOTCH_graphExit( &comp_sgraph );
    SCOTCH_stratExit( &sstrat );

    /* Update the comp_vtx, and comp_size */
    {
        pastix_int_t *vtx;
        pastix_int_t newid;

        for(i=0; i<nbpart; i++) {
            comp_sze[ *comp_nbr + i ] = 0;
        }

        for(i=0; i<comp_n; i++) {
            vtx = comp_vtx + fnode + i;
            assert( *vtx == comp_id );
            newid = *comp_nbr + parttab[i];
            comp_sze[newid]++;
            *vtx = newid;
        }
        comp_sze[comp_id] = 0;
        *comp_nbr += nbpart;
    }

    if ( comp_n != n ) {
        graphExit( &comp_graph );
    }

    /* /\* Sort the unknown of the component *\/ */
    /* { */
    /*     void *sortptr[3]; */
    /*     sortptr[0] = comp_vtx + fnode; */
    /*     sortptr[1] = invp     + fnode; */
    /*     sortptr[2] = peritab  + fnode; */

    /*     qsort3IntAsc( sortptr, comp_n ); */

    /*     /\* If necessary, move the partition to the end to keep the right order *\/ */
    /*     if ( (graph->n - lnode) != 0 ) { */
    /*         move_to_end( comp_n, graph->n - lnode, */
    /*                      comp_vtx + fnode, parttab ); */
    /*         move_to_end( comp_n, graph->n - lnode, */
    /*                      invp     + fnode, parttab ); */
    /*         move_to_end( comp_n, graph->n - lnode, */
    /*                      peritab  + fnode, parttab ); */

    /*         /\* Update the perm array *\/ */
    /*         for(i=0; i<graph->n; i++) { */
    /*             perm[invp[i]] = i; */
    /*         } */
    /*     } */
    /*     else { */
    /*         /\* Update the perm array *\/ */
    /*         for(i=fnode; i<lnode; i++) { */
    /*             perm[invp[i]] = i; */
    /*         } */
    /*     } */
    /* } */

    free( parttab );
    return PASTIX_SUCCESS;
}
