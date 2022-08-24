/**
 * @file simple_dist.c
 *
 * @brief A simple example that reads the matrix and then runs pastix in one call.
 *
 * @copyright 2015-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Tony Delarue
 * @author Mathieu Faverge
 * @date 2021-04-07
 *
 * @ingroup pastix_examples
 * @code
 *
 */
#include <pastix.h>
#include <spm.h>

int main (int argc, char **argv)
{
    pastix_data_t  *pastix_data = NULL; /*< Pointer to the storage structure required by pastix */
    pastix_int_t    iparm[IPARM_SIZE];  /*< Integer in/out parameters for pastix                */
    double          dparm[DPARM_SIZE];  /*< Floating in/out parameters for pastix               */
    spm_driver_t    driver;
    char           *filename;
    spmatrix_t     *spm, *spmd, spm2;
    void           *x, *b, *x0 = NULL;
    size_t          size;
    int             check = 1;
    int             nrhs  = 1;
    int             rc    = 0;

    /**
     * Initialize parameters to default values
     */
    pastixInitParam( iparm, dparm );

    /**
     * Get options from command line
     */
    pastixGetOptions( argc, argv,
                      iparm, dparm,
                      &check, &driver, &filename );

    /**
     * Startup PaStiX
     */
    pastixInit( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    /**
     * Read the sparse matrix with the driver
     */
    spm = malloc( sizeof( spmatrix_t ) );
    spmReadDriver( driver, filename, spm );
    free( filename );

    spmPrintInfo( spm, stdout );

#if defined(PASTIX_WITH_MPI)
    /* Scatter the spm and make sure everything works in distributed */
    spmd = spmScatter( spm, 0, NULL, 1, -1, MPI_COMM_WORLD );
    spmExit(spm);
    free(spm);
#else
    spmd = spm;
#endif

    rc = spmCheckAndCorrect( spmd, &spm2 );
    if ( rc != 0 ) {
        spmExit( spmd );
        *spmd = spm2;
        rc = 0;
    }

    /**
     * Generate a Fake values array if needed for the numerical part
     */
    if ( spmd->flttype == SpmPattern ) {
        spmGenFakeValues( spmd );
    }

    /**
     * Perform ordering, symbolic factorization, and analyze steps
     */
    pastix_task_analyze( pastix_data, spmd );

    /**
     * Normalize A matrix (optional, but recommended for low-rank functionality)
     */
    double normA = spmNorm( SpmFrobeniusNorm, spmd );
    spmScalMatrix( 1./normA, spmd );

    /**
     * Perform the numerical factorization
     */
    pastix_task_numfact( pastix_data, spmd );

    /**
     * Generates the b and x vector such that A * x = b
     * Compute the norms of the initial vectors if checking purpose.
     */
    size = pastix_size_of( spmd->flttype ) * spmd->n * nrhs;
    x = malloc( size );
    b = malloc( size );

    if ( check )
    {
        if ( check > 1 ) {
            x0 = malloc( size );
        }
        spmGenRHS( SpmRhsRndX, nrhs, spmd, x0, spmd->n, b, spmd->n );
        memcpy( x, b, size );
    }
    else {
        spmGenRHS( SpmRhsRndB, nrhs, spmd, NULL, spmd->n, x, spmd->n );

        /* Apply also normalization to b vectors */
        spmScalVector( spmd->flttype, 1./normA, spmd->n * nrhs, b, 1 );

        /* Save b for refinement */
        memcpy( b, x, size );
    }

    /**
     * Solve the linear system (and perform the optional refinement)
     */
    pastix_task_solve( pastix_data, nrhs, x, spmd->n );
    pastix_task_refine( pastix_data, spmd->n, nrhs, b, spmd->n, x, spmd->n );

    if ( check )
    {
        rc = spmCheckAxb( dparm[DPARM_EPSILON_REFINEMENT], nrhs, spmd, x0, spmd->n, b, spmd->n, x, spmd->n );

        if ( x0 ) {
            free( x0 );
        }
    }

    spmExit( spmd );
    free( spmd );
    free( x );
    free( b );
    pastixFinalize( &pastix_data );

    return rc;
}

/**
 * @endcode
 */
