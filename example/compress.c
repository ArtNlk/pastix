/**
 * @file compress.c
 *
 * @brief A compression example that factorizes the matrix with the Just-In-Time
 * strategy and Rank-Revealing kernels.
 *
 * @copyright 2015-2021 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Tony Delarue
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
    char           *filename = NULL;
    spmatrix_t     *spm, spm2;
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
     * Set low-rank parameters
     */
    iparm[IPARM_COMPRESS_WHEN]       = PastixCompressWhenEnd;
    iparm[IPARM_COMPRESS_METHOD]     = PastixCompressMethodPQRCP;
    iparm[IPARM_COMPRESS_MIN_WIDTH]  = 128;
    iparm[IPARM_COMPRESS_MIN_HEIGHT] = 25;
    dparm[DPARM_COMPRESS_TOLERANCE]  = 1e-8;

    /**
     * Startup PaStiX
     */
    pastixInit( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    /**
     * Read the sparse matrix with the driver
     */
    spm = malloc( sizeof( spmatrix_t ) );
    rc = spmReadDriver( driver, filename, spm );
    free( filename );
    if ( rc != SPM_SUCCESS ) {
        pastixFinalize( &pastix_data );
        return rc;
    }

    spmPrintInfo( spm, stdout );

    rc = spmCheckAndCorrect( spm, &spm2 );
    if ( rc != 0 ) {
        spmExit( spm );
        *spm = spm2;
        rc = 0;
    }

    /**
     * Generate a Fake values array if needed for the numerical part
     */
    if ( spm->flttype == SpmPattern ) {
        spmGenFakeValues( spm );
    }

    /**
     * Perform ordering, symbolic factorization, and analyze steps
     */
    pastix_task_analyze( pastix_data, spm );

    /**
     * Normalize A matrix (optional, but recommended for low-rank functionality)
     */
    double normA = spmNorm( SpmFrobeniusNorm, spm );
    spmScalMatrix( 1./normA, spm );

    /**
     * Perform the numerical factorization
     */
    pastix_task_numfact( pastix_data, spm );

    /**
     * Generates the b and x vector such that A * x = b
     * Compute the norms of the initial vectors if checking purpose.
     */
    size = pastix_size_of( spm->flttype ) * spm->n * nrhs;
    x = malloc( size );
    b = malloc( size );

    if ( check )
    {
        if ( check > 1 ) {
            x0 = malloc( size );
        }
        spmGenRHS( SpmRhsRndX, nrhs, spm, x0, spm->n, b, spm->n );
        memcpy( x, b, size );
    }
    else {
        spmGenRHS( SpmRhsRndB, nrhs, spm, NULL, spm->n, x, spm->n );

        /* Apply also normalization to b vectors */
        spmScalVector( spm->flttype, 1./normA, spm->n * nrhs, b, 1 );

        /* Save b for refinement */
        memcpy( b, x, size );
    }

    /**
     * Solve the linear system (and perform the optional refinement)
     */
    pastix_task_solve( pastix_data, nrhs, x, spm->n );
    pastix_task_refine( pastix_data, spm->n, nrhs, b, spm->n, x, spm->n );

    if ( check )
    {
        rc = spmCheckAxb( dparm[DPARM_EPSILON_REFINEMENT], nrhs, spm, x0, spm->n, b, spm->n, x, spm->n );

        if ( x0 ) {
            free( x0 );
        }
    }

    spmExit( spm );
    free( spm );
    free( x );
    free( b );
    pastixFinalize( &pastix_data );

    return rc;
}

/**
 * @endcode
 */
