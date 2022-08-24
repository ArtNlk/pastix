/**
 *
 * @file graph_connected_components.c
 *
 * PaStiX routines to isolate disconnected subgraphs
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-02-12
 *
 **/
#include "common.h"
#include <spm.h>
#include "graph/graph.h"
#include "pastix/order.h"

typedef struct _Queue {
    pastix_int_t *tab;
    pastix_int_t size;
    pastix_int_t start;
    pastix_int_t end;
} Queue;

static void queue_init (Queue *self, pastix_int_t size) {
    self->size = size+1;
    self->tab = malloc (self->size * sizeof (pastix_int_t));
    self->start = -1;
    self->end   = -1;
}

static void queue_push_back (Queue *self, pastix_int_t v) {
    self->end++;
    assert( self->end < self->size );
    self->tab[ self->end ] = v;
}

static pastix_int_t queue_pop_front (Queue* self) {
    pastix_int_t v;

    self->start++;
    assert( self->start <= self->end );
    v = self->tab[self->start];
    return v;
}

static pastix_int_t queue_is_empty (const Queue *self) {
    return self->start == self->end;
}

static void queue_free (Queue *self) {
    free (self->tab);
}

/**
 * @brief Invert partition p1 and p2 in buffer A of size (p1+p2), using workspace W of size p1
 */
void
move_to_end( pastix_int_t  p1,
             pastix_int_t  p2,
             pastix_int_t *A,
             pastix_int_t *W )
{
    pastix_int_t size, from, to, move;

    memcpy( W, A, p1 * sizeof(pastix_int_t) );

    size = p2;
    from = p1;
    to   = 0;
    while( size > 0 ) {
        move = pastix_imin( p1, size );
        memcpy( A + to, A + from, move * sizeof(pastix_int_t) );
        size -= move;
        from += move;
        to   += move;
    }
    assert( p2 - to == 0 );
    memcpy( A + to, W, p1 * sizeof(pastix_int_t) );
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Isolate the connected components from the original graph
 *
 *******************************************************************************
 *
 * @param[in] graph
 *          The original graph associated from which vertices and edges must be
 *          extracted.
 *
 * @param[inout] comp_vtx
 *          Array of size n that hold the index of the component for each vertex
 *          of the graph.
 *
 * @param[inout] comp_size
 *          The size of each components in the graph.
 *
 *******************************************************************************
 *
 * @retval The amount of connected components in the graph
 *
 *******************************************************************************/
pastix_int_t
graphIsolateConnectedComponents( const pastix_graph_t *graph,
                                 pastix_int_t *comp_vtx,
                                 pastix_int_t *comp_sze )
{
    const pastix_int_t *colptr;
    const pastix_int_t *rowptr;
    pastix_int_t baseval = graph->baseval;
    pastix_int_t n       = graph->n;
    pastix_int_t cur     = 0;
    pastix_int_t i, u, v, total;
    Queue        q;

    /* Set the comp_vtx to -1 */
    memset(comp_vtx, 0xff, n*sizeof(pastix_int_t));

    /* Make sure the graph is 0 based. */
    assert(baseval == 0);

    queue_init (&q, n);

    total  = n;
    colptr = graph->colptr;
    rowptr = graph->rowptr - baseval;
    while ( total > 0 ) {
        i = 0;

        while ((i < n) && (comp_vtx[i] != -1) ) {
            i++;
        }
        assert( i < n );

        queue_push_back( &q, i );
        comp_vtx[i]   = cur;
        comp_sze[cur] = 1;
        total--;

        while ( !queue_is_empty( &q ) )
        {
            v = queue_pop_front( &q );

            for (i = colptr[v]; i < colptr[v+1]; i++) {
                u = rowptr[i] - baseval;

                if ( comp_vtx[u] != -1 ) {
                    assert( comp_vtx[u] == comp_vtx[v] );
                    continue;
                }

                queue_push_back( &q, u );
                comp_vtx[u] = cur;
                comp_sze[cur]++;
                total--;
            }
        }
        /* End of this component */
        cur++;
    }

#ifndef NDEBUG
    for (i=0; i<n; i++){
        assert( comp_vtx[i] != -1);
    }
#endif

    queue_free (&q);
    return cur;
}
