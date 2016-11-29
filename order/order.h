/**
 *
 * @file order.h
 *
 *  PaStiX order routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 6.0.0
 * @author Francois Pellegrini
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#ifndef _ORDER_H_
#define _ORDER_H_

#include "graph.h"

/**
 * @ingroup pastix_ordering
 * @struct Order - Ordering structure.
 */
struct Order_ {
    pastix_int_t  baseval;   /*< base value used for numbering       */
    pastix_int_t  vertnbr;   /*< Number of vertices                  */
    pastix_int_t  cblknbr;   /*< Number of column blocks             */
    pastix_int_t *permtab;   /*< Permutation array [based]           */
    pastix_int_t *peritab;   /*< Inverse permutation array [based]   */
    pastix_int_t *rangtab;   /*< Column block range array [based,+1] */
    pastix_int_t *treetab;   /*< Partitioning tree [based]           */
};

/*
 * The function prototypes.
 */
int  orderInit (      Order * const ordeptr, pastix_int_t baseval, pastix_int_t cblknbr, pastix_int_t vertnbr,
                      pastix_int_t *perm, pastix_int_t *peri, pastix_int_t *rang, pastix_int_t *tree );
int  orderAlloc(      Order * const ordeptr, pastix_int_t cblknbr, pastix_int_t vertnbr);
void orderExit (      Order * const ordeptr);
void orderBase (      Order * const ordeptr, pastix_int_t baseval);
int  orderCheck(const Order * const ordeptr);

int  orderComputeScotch(   pastix_data_t *pastix_data, pastix_graph_t *graph );
int  orderComputePTScotch( pastix_data_t *pastix_data, pastix_graph_t *graph );
int  orderComputeMetis(    pastix_data_t *pastix_data, pastix_graph_t *graph );
int  orderComputeParMetis( pastix_data_t *pastix_data, pastix_graph_t *graph );
int  orderComputeOptimal(  pastix_data_t *pastix_data, pastix_int_t n );

int  orderLoad(       Order * const ordeptr, char *filename );
int  orderSave( const Order * const ordeptr, char *filename );

void orderFindSupernodes( const pastix_graph_t *graph,
                          Order * const ordeptr );

int  orderApplyLevelOrder( Order *ordeptr, pastix_int_t distribution_level );

int  orderAddIsolate( Order              *ordeptr,
                      pastix_int_t        new_n,
                      const pastix_int_t *perm );


#endif /* _ORDER_H_ */

