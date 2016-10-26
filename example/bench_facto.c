/**
 *  @file: simple.c
 *
 *  A simple example :
 *  read the matrix, check it is correct and correct it if needed,
 *  then run pastix in one call.
 *
 */
#include <pastix.h>
#include <spm.h>
#include "../matrix_drivers/drivers.h"

int main (int argc, char **argv)
{
    pastix_data_t  *pastix_data = NULL; /*< Pointer to the storage structure required by pastix */
    pastix_int_t    iparm[IPARM_SIZE];  /*< Integer in/out parameters for pastix                */
    double          dparm[DPARM_SIZE];  /*< Floating in/out parameters for pastix               */
    pastix_driver_t driver;
    char           *filename;
    pastix_spm_t   *spm, *spm2;
    void           *x0, *x, *b;
    size_t          size;
    int             check = 2;
    int             nrhs = 1;
    int             i, nbruns = 3;

    /**
     * Initialize parameters to default values
     */
    pastixInitParam( iparm, dparm );

    /**
     * Get options from command line
     */
    pastix_ex_getoptions( argc, argv,
                          iparm, dparm,
                          &driver, &filename );

    /**
     * Startup PaStiX
     */
    pastixInit( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    /**
     * Read the sparse matrix with the driver
     */
    spm = malloc( sizeof( pastix_spm_t ) );
    spmReadDriver( driver, filename, spm, MPI_COMM_WORLD );
    free(filename);

    spm2 = spmCheckAndCorrect( spm );
    if ( spm2 != spm ) {
        spmExit( spm );
        free(spm);
        spm = spm2;
    }

    /**
     * Perform ordering, symbolic factorization, and analyze steps
     */
    pastix_task_order( pastix_data, spm, NULL, NULL );
    pastix_task_symbfact( pastix_data, NULL, NULL );
    pastix_task_reordering( pastix_data );
    pastix_task_blend( pastix_data );


    for(i=0; i<nbruns; i++) {
        /**
         * Perform the numerical factorization
         */
        pastix_task_sopalin( pastix_data, spm );
    }

    /**
     * Generates the b and x vector such that A * x = b
     * Compute the norms of the initial vectors if checking purpose.
     */
    size = pastix_size_of( spm->flttype ) * spm->n;
    x = malloc( size );

    if ( check )
    {
        b = malloc( size );

        if ( check > 1 ) {
            x0 = malloc( size );
        } else {
            x0 = NULL;
        }
        spmGenRHS( PastixRhsRndX, nrhs, spm, x0, spm->n, b, spm->n );
        memcpy( x, b, size );
    }
    else {
        spmGenRHS( PastixRhsRndB, nrhs, spm, NULL, spm->n, x, spm->n );

        /* Save b for refinement: TODO: make 2 examples w/ or w/o refinement */
        b = malloc( size );
        memcpy( b, x, size );
    }

    /**
     * Solve the linear system
     */
    pastix_task_solve( pastix_data, spm, nrhs, x, spm->n );

    pastix_task_raff(pastix_data, x, nrhs, b);

    if ( check )
    {
        spmCheckAxb( nrhs, spm, x0, spm->n, b, spm->n, x, spm->n );

        if (x0) free(x0);

        free(x); free(b);
    }
    spmExit( spm );
    free( spm );
    pastixFinalize( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    return EXIT_SUCCESS;
}