/**
 *
 * @file graph.h
 *
 * PaStiX graph structure routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-01-25
 *
 *
 * @addtogroup pastix_graph
 * @{
 *   @brief Functions to generate and manipulate the graph structure.
 *
 *   This module provides the set of function to prepare the graph structure
 *   associated to a given sparse matrix.
 *   It is possible to symmetrize a graph, to extract a subgraph and to apply a
 *   new permutation.
 *
 **/
#ifndef _graph_h_
#define _graph_h_

/**
 * @name Graph basic subroutines
 * @{
 */
int  graphPrepare(       pastix_data_t   *pastix_data,
                   const spmatrix_t      *spm,
                         pastix_graph_t **graph );
void graphBase   (       pastix_graph_t  *graph,
                         pastix_int_t     baseval );
void graphExit   (       pastix_graph_t  *graph );

/**
 * @}
 * @name Graph IO subroutines
 * @{
 */
void graphLoad( const pastix_data_t  *pastix_data,
                      pastix_graph_t *graph );
void graphSave(       pastix_data_t  *pastix_data,
                const pastix_graph_t *graph );

/**
 * @}
 * @name Graph manipulation subroutines
 * @{
 */
int  graphCopy      (       pastix_graph_t *graphdst,
                      const pastix_graph_t *graphsrc );
int  graphSpm2Graph (       pastix_graph_t *graph,
                      const spmatrix_t     *spm );
void graphSort      (       pastix_graph_t *graph );
void graphNoDiag    (       pastix_graph_t *graph );
int  graphSymmetrize(       pastix_graph_t *graph );

int  graphUpdateComputedFields( pastix_graph_t *graph );

int  graphIsolate   (       pastix_int_t    n,
                      const pastix_int_t   *colptr,
                      const pastix_int_t   *rows,
                            pastix_int_t    isolate_n,
                            pastix_int_t   *isolate_list,
                            pastix_int_t   **new_colptr,
                            pastix_int_t   **new_rows,
                            pastix_int_t   **new_perm,
                            pastix_int_t   **new_invp );

int  graphApplyPerm ( const pastix_graph_t *graphA,
                      const pastix_int_t   *perm,
                            pastix_graph_t *graphPA );

int graphIsolateRange( const pastix_graph_t *graphIn,
                       const pastix_order_t *order,
                             pastix_graph_t *graphOut,
                             pastix_int_t    fnode,
                             pastix_int_t    lnode,
                             pastix_int_t    distance );
void graphComputeProjection( const pastix_graph_t *graph,
                             const int            *vertlvl,
                                   pastix_order_t *order,
                             const pastix_graph_t *subgraph,
                                   pastix_order_t *suborder,
                                   pastix_int_t    fnode,
                                   pastix_int_t    lnode,
                                   pastix_int_t    sn_level,
                                   pastix_int_t    distance,
                                   pastix_int_t    maxdepth,
                                   pastix_int_t    maxwidth,
                                   pastix_int_t   *depthsze );

pastix_int_t graphIsolateConnectedComponents( const pastix_graph_t *graph,
                                              pastix_int_t         *comp_vtx,
                                              pastix_int_t         *comp_sze );

int graphComputeKway( const pastix_graph_t *graph,
                      pastix_order_t       *order,
                      pastix_int_t         *peritab,
                      pastix_int_t         *comp_nbr,
                      pastix_int_t         *comp_sze,
                      pastix_int_t         *comp_vtx,
                      pastix_int_t          comp_id,
                      pastix_int_t          nbpart );
/**
 * @}
 */

#endif /* _graph_h_ */

/**
 * @}
 */
