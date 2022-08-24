/**
 *
 * @file order_internal.h
 *
 * PaStiX order internal routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Francois Pellegrini
 * @author Mathieu Faverge
 * @date 2021-06-28
 *
 **/
#ifndef _order_internal_h_
#define _order_internal_h_

#include "pastix/order.h"

BEGIN_C_DECLS

#define orderDrawGraph       ( 0x1 << 0 )
#define orderDrawCoordinates ( 0x1 << 1 )
#define orderDrawMapping     ( 0x1 << 2 )

void
orderDraw( pastix_data_t *pastix_data,
           const char    *filename,
           pastix_int_t   sndeidx,
           int            dump );

pastix_int_t
orderSupernodes( const pastix_graph_t *graph,
                 pastix_order_t       *order,
                 EliminTree           *etree,
                 pastix_int_t         *iparm, 
                 int                   do_schur );

EliminTree *pastixOrderBuildEtree( const pastix_order_t *order );

END_C_DECLS

#endif /* _order_internal_h_ */

/**
 * @}
 */
