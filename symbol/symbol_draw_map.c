/**
 *
 * @file symbol_draw_map.c
 *
 * PaStiX symbol routines to print the .map files of the cblks
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @date 2021-01-03
 *
 */
#define _GNU_SOURCE 1
#include "common.h"
#include "order/order_internal.h"
#include "symbol/symbol.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_symbol
 *
 * @brief Dump a separator mapping into a map file.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix data structure that holds the symbol and the ordering.
 *          On exit the output directories may be initialized, if not previously.
 *
 * @param[in] extname
 *          Filename extension to specify the .map file if multiple.
 *
 * @param[in] sndeidx
 *          The index of the supernode to dump into file.
 *
 *******************************************************************************/
void
pastixSymbolDrawMap( pastix_data_t *pastix_data,
                     const char    *extname,
                     pastix_int_t   sndeidx )
{
    char                  *fname;
    FILE                  *file;
    const pastix_order_t  *order    = pastix_data->ordemesh;
    const symbol_matrix_t *symbmtx  = pastix_data->symbmtx;
    const symbol_cblk_t   *symbcblk = symbmtx->cblktab;
    pastix_int_t           ibeg, iend, size;
    pastix_int_t           i, j;
    pastix_int_t           color = 0;
    int                    rc;

    assert( order != NULL );
    assert( order->sndetab != NULL );
    assert( symbmtx != NULL );

    ibeg = order->sndetab[sndeidx];
    iend = order->sndetab[sndeidx+1];
    size = iend - ibeg;

    pastix_gendirectories( pastix_data );

    if ( extname ) {
        rc = asprintf( &fname, "part.%ld.%s.map",
                       (long)sndeidx, extname );
    }
    else {
        rc = asprintf( &fname, "part.%ld.map",
                       (long)sndeidx );
    }
    assert( rc != -1 );
    file = pastix_fopenw( pastix_data->dir_global, fname, "w" );
    free( fname );

    fprintf( file, "%ld\n", (long)size );

    /*
     * Look for the last cblk implied in the original supernode
     * The search is down backward to have more coherent coloring from one
     * version to another, and because we usually draw the last supernode
     * and no more.
     */
    i = symbmtx->cblknbr;
    while ( (i > 0) && (symbcblk[i].fcolnum > iend) ) {
        i--;
    }
    i--;

    for (; i>0; i--) {
        pastix_int_t fnode = symbcblk[i].fcolnum;
        pastix_int_t lnode = symbcblk[i].lcolnum;

        if ( fnode < ibeg ) {
            assert( lnode < ibeg );
            break;
        }

        for (j=fnode; j<=lnode; j++) {
            fprintf( file, "%ld %ld\n",
                     (long)(j - ibeg), (long)color );
        }
        color++;
    }
    fclose( file );

    (void)rc;
}
