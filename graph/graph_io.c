/**
 *
 * @file graph_io.c
 *
 * PaStiX graph IO routines
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-06-14
 *
 **/
#include "common.h"
#include "graph/graph.h"
#include <spm.h>

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Load a graph from a file
 *
 * The file is named 'graphname' in the local directory.
 *
 *******************************************************************************
 *
 * @param[in] pastix_data
 *          The pointer to the solver instance to get options as rank,
 *          communicators, ...
 *
 * @param[inout] graph
 *          The graph structure to store the loaded graph.
 *          The graph is read from the file named by the environment variable
 *          PASTIX_FILE_GRAPH, and if PASTIX_FILE_GRAPH is not defined, the
 *          default filename "graphname" in the local directory is used.
 *
 *******************************************************************************/
void
graphLoad( const pastix_data_t *pastix_data,
           pastix_graph_t      *graph )
{
    char *filename = NULL;
    int   env = 1;

    /* Parameter checks */
    if ( graph == NULL ) {
        return;
    }

    /*
     * Get the environment variable as second option
     */
    filename = pastix_getenv( "PASTIX_FILE_GRAPH" );
    env = 1;

    /*
     * Get the default name as third option
     */
    if ( filename == NULL ) {
        filename = "graphname";
        env = 0;
    }

    spmLoad( graph, filename );

    if (env) {
        pastix_cleanenv( filename );
    }

    (void)pastix_data;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_graph
 *
 * @brief Save a graph to file.
 *
 * The file is named 'graphgen' in the local directory.
 *
 *******************************************************************************
 *
 * @param[in] pastix_data
 *          The pointer to the solver instance to get options as rank,
 *          communicators, ...
 *
 * @param[in] graph
 *          The graph structure to store the loaded graph.
 *          The graph is written to the file named by the environment variable
 *          PASTIX_FILE_GRAPH, and if PASTIX_FILE_GRAPH is not defined, the
 *          default filename "graphname" in the local directory is used.
 *
 *******************************************************************************/
void
graphSave( pastix_data_t        *pastix_data,
           const pastix_graph_t *graph )
{
    char *filename = NULL;
    char *fullname = NULL;
    int   env      = 1;

    /* Parameter checks */
    if ( graph == NULL ) {
        return;
    }

    /*
     * Get the environment variable as first option
     */
    filename = pastix_getenv( "PASTIX_FILE_GRAPH" );

    /*
     * Get the default name as second option
     */
    if ( filename == NULL ) {
        filename = "graphgen";
        env = 0;
    }

    pastix_gendirectories( pastix_data );
    fullname = pastix_fname( pastix_data->dir_local, filename );
    if ( fullname ) {
        spmSave( graph, fullname );
        free( fullname );
    }

    if ( env ) {
        pastix_cleanenv( filename );
    }
}
