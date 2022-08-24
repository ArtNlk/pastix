/**
 *
 * @file order_draw.c
 *
 * PaStiX order routines dedicated to split supernodes thanks to graph connectivity
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
 */
#define _GNU_SOURCE 1
#include "common.h"
#include "order_internal.h"
#include "graph/graph.h"
#include <scotch.h>

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Dump the last separator into an ivview file.
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix data structure that holds the graph and the ordering.
 *          On exit the output directories may be initialized, if not previously.
 *
 * @param[in] extname
 *          Filename extension to specify the .map file if multiple.
 *
 * @param[in] sndeidx
 *          The index of the supernode to dump into file.
 *
 * @param[in] dump
 *          Define which information to dump:
 *          - (0x1 << 0) dumps the graph file
 *          - (0x1 << 1) dumps the coordinate file
 *          - (0x1 << 2) dumps the color mapping file
 *
 *******************************************************************************/
void
orderDraw( pastix_data_t *pastix_data,
           const char    *extname,
           pastix_int_t   sndeidx,
           int            dump )
{
    char           *fname;
    FILE           *file;
    pastix_graph_t *graph = pastix_data->graph;
    pastix_order_t *order = pastix_data->ordemesh;
    pastix_int_t    ibeg, iend, size;
    pastix_int_t    i, j;
    int             rc;

    assert( graph != NULL );
    assert( order != NULL );
    assert( order->sndetab != NULL );

    ibeg = order->sndetab[sndeidx];
    iend = order->sndetab[sndeidx+1];
    size = iend - ibeg;

    if ( dump ) {
        pastix_gendirectories( pastix_data );
    }

    /*
     * Dump the graph file
     */
    if ( dump & orderDrawGraph ) {
        SCOTCH_Graph   sn_sgraph;
        pastix_graph_t sn_pgraph;
        pastix_int_t  *sn_colptr;
        pastix_int_t  *sn_rows;

        /**
         * Extract the subgraph with unknowns of the supernode sndeidx
         *
         * 1 is sufficient for the max_distance in most cases, but set to 2 for
         * corner cases that need extra connexions, and to match order_supernode.
         */
        graphIsolateRange( graph, order, &sn_pgraph,
                           ibeg, iend, 2 );

        sn_colptr = sn_pgraph.colptr;
        sn_rows   = sn_pgraph.rowptr;

        if ( !SCOTCH_graphInit(&sn_sgraph) )
        {
            SCOTCH_graphBuild( &sn_sgraph,
                               order->baseval,
                               size,
                               sn_colptr,
                               NULL,
                               NULL,
                               NULL,
                               sn_colptr[ size ] - order->baseval,
                               sn_rows,
                               NULL );
        }
        else
        {
            fprintf( stderr, "Failed to build graph\n" );
            return;
        }

        rc = asprintf( &fname, "part.%ld.grf", (long)sndeidx);
        assert( rc != -1 );

        file = pastix_fopenw( pastix_data->dir_global, fname, "w" );
        SCOTCH_graphSave( &sn_sgraph, file );
        fclose( file );
        free(fname);

        fprintf(stderr,"Check: %d\n", SCOTCH_graphCheck( &sn_sgraph ));
        free(sn_colptr);
        free(sn_rows);
    }

    /*
     * Dump the XYZ file
     */
    if ( dump & orderDrawCoordinates )
    {
        FILE *filein;
        long dim, n;

        filein = fopen( "before.xyz", "r" );
        if ( filein == NULL ) {
            fprintf( stderr, "Please give before.xyz file\n" );
            return;
        }

        /* Read dimensions */
        rc = fscanf( filein, "%ld %ld", &dim, &n );
        if ( n != order->vertnbr ){
            fprintf(stderr, "Cannot proceed part.xyz and part.map files: invalid number of vertices in before.xyz\n");
            fclose(filein);
            return;
        }

        rc = asprintf( &fname, "part.%ld.xyz", (long)sndeidx);
        assert( rc != -1 );
        file = pastix_fopenw( pastix_data->dir_global, fname, "w" );
        free( fname );

        fprintf( file, "%ld %ld\n", (long)dim, (long)size );
        for(i=0; i<order->vertnbr; i++) {
            long v, iv;
            double x, y, z;

            rc = fscanf(filein, "%ld %lf %lf %lf", &v, &x, &y, &z );
            assert( rc == 4 );

            /* If node within the selected supernode, let's keep it */
            iv = order->permtab[i];
            if ( (iv >= ibeg) && (iv < iend) ) {
                fprintf( file, "%ld %lf %lf %lf\n",
                         (long)(iv - ibeg), x, y, z );
            }
        }

        fclose(file);
        fclose(filein);
    }

    /*
     * Dump the mapping file
     */
    if ( dump & orderDrawMapping )
    {
        pastix_int_t color = 0;

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
        i = order->cblknbr;
        while ( (i > 0) && (order->rangtab[i] > iend) ) {
            i--;
        }
        i--;

        for (; i>0; i--) {
            pastix_int_t fnode = order->rangtab[i];
            pastix_int_t lnode = order->rangtab[i+1];

            if ( fnode < ibeg ) {
                assert( lnode <= ibeg );
                break;
            }

            for (j=fnode; j<lnode; j++) {
                fprintf( file, "%ld %ld\n",
                         (long)(j - ibeg), (long)color );
            }
            color++;
        }
        fclose(file);
    }
    (void)rc;
}
