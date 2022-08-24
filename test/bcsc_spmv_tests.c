/**
 *
 * @file bcsc_spmv_tests.c
 *
 * @copyright 2011-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * Tests and validate the bcsc_spmv routines.
 *
 * @version 6.2.0
 * @author Mathieu Faverge
 * @author Theophile Terraz
 * @author Esragul Korkmaz
 * @author Pierre Ramet
 * @author Tony Delarue
 * @author Vincent Bridonneau
 * @date 2020-06-07
 *
 **/
#include <pastix.h>
#include "common.h"
#include "bcsc/bcsc.h"

#include "z_tests.h"
#include "c_tests.h"
#include "d_tests.h"
#include "s_tests.h"

#define PRINT_RES(_ret_)                        \
    if(_ret_) {                                 \
        printf("FAILED(%d)\n", _ret_);          \
        err++;                                  \
    }                                           \
    else {                                      \
        printf("SUCCESS\n");                    \
    }

char* fltnames[]   = { "Pattern", "", "Float", "Double", "Complex32", "Complex64" };
char* transnames[] = { "NoTrans", "Trans", "ConjTrans" };
char* mtxnames[]   = { "General", "Symmetric", "Hermitian" };
char* schednames[] = { "Sequential", "Static" };

int main (int argc, char **argv)
{
    pastix_data_t  *pastix_data = NULL; /* Pointer to a storage structure needed by pastix  */
    pastix_int_t    iparm[IPARM_SIZE];  /* integer parameters for pastix                    */
    double          dparm[DPARM_SIZE];  /* floating parameters for pastix                   */
    spm_driver_t    driver;             /* Matrix driver(s) requested by user               */
    spmatrix_t     *spm, spm2;
    char *filename;                     /* Filename(s) given by user                        */
    int s, t;
    int ret = PASTIX_SUCCESS;
    int err = 0;

    /**
     * Initialize parameters to default values
     */
    pastixInitParam( iparm, dparm );

    /**
     * Get options from command line
     */
    pastixGetOptions( argc, argv,
                      iparm, dparm,
                      NULL, &driver, &filename );

    /**
     * Initialize the PaStiX library
     */
    pastixInit( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    /**
     * Read the sparse matrix with the driver
     */
    spm = malloc( sizeof( spmatrix_t ) );
    spmReadDriver( driver, filename, spm );
    free(filename);

    ret = spmCheckAndCorrect( spm, &spm2 );
    if ( ret != 0 ) {
        spmExit( spm );
        *spm = spm2;
        ret = 0;
    }

    if ( spm->flttype == SpmPattern ) {
        spmGenFakeValues( spm );
    }

    /**
     * Startup pastix to perform the analyze step
     */
    pastix_task_analyze( pastix_data, spm );
    pastix_subtask_spm2bcsc( pastix_data, spm );

    printf(" -- BCSC MatVec Test --\n");
    for( s=PastixSchedSequential; s<=PastixSchedStatic; s++ )
    {
        pastix_data->iparm[IPARM_SCHEDULER] = s;
        for( t=PastixNoTrans; t<=PastixConjTrans; t++ )
        {
            if ( (t == PastixConjTrans) &&
                 ((spm->flttype != SpmComplex64) && (spm->flttype != SpmComplex32)) )
            {
                continue;
            }
            printf("   Case %s - %s - %s - %s:\n",
                   schednames[s],
                   fltnames[spm->flttype],
                   mtxnames[spm->mtxtype - SpmGeneral],
                   transnames[t - PastixNoTrans] );

            switch( spm->flttype ){
            case SpmComplex64:
                ret = z_bcsc_spmv_check( t, spm, pastix_data );
                break;

            case SpmComplex32:
                ret = c_bcsc_spmv_check( t, spm, pastix_data );
                break;

            case SpmFloat:
                ret = s_bcsc_spmv_check( t, spm, pastix_data );
                break;

            case SpmDouble:
            default:
                ret = d_bcsc_spmv_check( t, spm, pastix_data );
            }
            err += ret;
            if ( pastix_data->procnum == 0 ) {
                printf( "   Case %s - %s - %s - %s: ",
                        schednames[s],
                        fltnames[spm->flttype],
                        mtxnames[spm->mtxtype - SpmGeneral],
                        transnames[t - PastixNoTrans] );
                if( err != 0 ) {
                    printf( "FAILED\n" );
                }
                else {
                    printf( "SUCCESS\n" );
                }
            }
        }
    }

    spmExit( spm );
    free( spm );

    if ( pastix_data->procnum == 0 ) {
        if( err == 0 ) {
            printf(" -- All tests PASSED --\n");
        }
        else {
            printf(" -- %d tests FAILED --\n", err);
        }
    }

    pastixFinalize( &pastix_data );

    return ( err == 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
}
