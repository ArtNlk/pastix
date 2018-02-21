/**
 * @file reentrant.c
 *
 * @brief A reentrant example that runs two threads then run two instances of the solver in each thread.
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Hastaran Matias
 * @date 2017-01-17
 *
 * @ingroup pastix_examples
 * @code
 *
 */
#include <pthread.h>
#include <pastix.h>
#include <spm.h>

/**
 *  Struct: solv_param
 *
 *  Structure containing information to give
 *  to each thread.
 */
typedef struct solve_param {
    pastix_int_t        iparm[IPARM_SIZE];
    double              dparm[DPARM_SIZE];
    char               *filename;
    pastix_driver_t     driver;
    int                 check;
    int                 id;
} solve_param_t;

/**
 * Function: solve_smp
 *
 * Thread routine to launch the solver
 *
 * Parameters:
 *   arg - a pointer to a <solve_param> structure.
 */
static void *solve_smp(void *arg)
{
    pastix_data_t *pastix_data = NULL; /*< Pointer to the storage structure required by pastix */
    pastix_spm_t  *spm;
    pastix_spm_t  *spm2;
    void          *x0 = NULL;
    void          *x;
    void          *b;
    int           *bindtab;
    size_t         size;
    int            i, check;
    int            nrhs = 1;
    solve_param_t  param = *(solve_param_t *)arg;

    if ( param.iparm[IPARM_THREAD_NBR] == -1) {
        param.iparm[IPARM_THREAD_NBR] = 2;
    }
    check = param.check;

    /* Bindtab array */
    bindtab = malloc( param.iparm[IPARM_THREAD_NBR] * sizeof(int) );
    for(i=0; i<param.iparm[IPARM_THREAD_NBR]; i++ ) {
        bindtab[i] = param.iparm[IPARM_THREAD_NBR] * param.id + i;
    }

    /**
     * Startup PaStiX
     */
    pastixInitWithAffinity( &pastix_data, MPI_COMM_WORLD,
                            param.iparm, param.dparm,
                            bindtab );
    free(bindtab);

    /**
     * Read the sparse matrix with the driver
     */
    spm = malloc( sizeof( pastix_spm_t ) );
    spmReadDriver( param.driver, param.filename, spm, MPI_COMM_WORLD );

    spmPrintInfo( spm, stdout );

    spm2 = spmCheckAndCorrect( spm );
    if ( spm2 != spm ) {
        spmExit( spm );
        free(spm);
        spm = spm2;
    }

    /**
     * Generate a Fake values array if needed for the numerical part
     */
    if ( spm->flttype == PastixPattern ) {
        spmGenFakeValues( spm );
    }

    /**
     * Perform ordering, symbolic factorization, and analyze steps
     */
    pastix_task_analyze( pastix_data, spm );

    /**
     * Normalize A matrix (optional, but recommended for low-rank functionality)
     */
    double normA = spmNorm( PastixFrobeniusNorm, spm );
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
        spmGenRHS( PastixRhsRndX, nrhs, spm, x0, spm->n, b, spm->n );
        memcpy( x, b, size );
    }
    else {
        spmGenRHS( PastixRhsRndB, nrhs, spm, NULL, spm->n, x, spm->n );

        /* Apply also normalization to b vector */
        spmScalVector( 1./normA, spm, b );

        /* Save b for refinement: TODO: make 2 examples w/ or w/o refinement */
        memcpy( b, x, size );
    }

    /**
     * Solve the linear system
     */
    pastix_task_solve( pastix_data, nrhs, x, spm->n );
    pastix_task_refine( pastix_data, x, nrhs, b );
    if ( check )
    {
        spmCheckAxb( nrhs, spm, x0, spm->n, b, spm->n, x, spm->n );
        if (x0) free(x0);
    }
    spmExit( spm );
    free(x);
    free(b);
    free( spm );
    pastixFinalize( &pastix_data );

    return EXIT_SUCCESS;
}

int main (int argc, char **argv)
{
    pastix_int_t        iparm[IPARM_SIZE];      /*< Integer in/out parameters for pastix                */
    double              dparm[DPARM_SIZE];      /*< Floating in/out parameters for pastix               */
    pastix_driver_t     driver;                 /*< Matrix driver(s) requested by user                  */
    char               *filename;               /*< Filename(s) given by user                           */
    int                 nbcallingthreads = 2;
    solve_param_t      *solve_param;
    pthread_t          *threads;
    int                 i, check = 1;

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
     *    Set parameters for each thread
     */
    solve_param = (solve_param_t*) malloc(nbcallingthreads * sizeof(solve_param_t));
    threads     = (pthread_t*)     malloc(nbcallingthreads * sizeof(pthread_t));

    for (i = 0; i < nbcallingthreads; i++)
    {
        memcpy(solve_param[i].iparm, iparm, sizeof(solve_param[i].iparm));
        memcpy(solve_param[i].dparm, dparm, sizeof(solve_param[i].dparm));
        solve_param[i].check    = check;
        solve_param[i].id       = i;
        solve_param[i].driver   = driver;
        solve_param[i].filename = filename;

        /**
         *   Launch instance of solver
         */
        pthread_create(&threads[i], NULL, solve_smp, (void *)&solve_param[i]);
    }

    /**
     *     Wait for the end of thread
     */
    for (i = 0; i < nbcallingthreads; i++)
        pthread_join(threads[i],(void**)NULL);

    free(filename);
    free(threads);
    free(solve_param);
    return EXIT_SUCCESS;
}

/**
 * @endcode
 */
