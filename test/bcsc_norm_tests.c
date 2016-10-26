/**
 *
 * @file bcsc_norm_test.c
 *
 * Tests and validate the bcsc_norm routines.
 *
 * @version 5.1.0
 * @author Mathieu Faverge
 * @author Theophile Terraz
 * @date 2015-01-01
 *
 **/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pastix.h>
#include "../matrix_drivers/drivers.h"
#include "common.h"
#include <spm.h>
#include <bcsc.h>
#include "sopalin_data.h"

int z_bcsc_norm_check( const pastix_spm_t *spm, const pastix_bcsc_t *bcsc );
int c_bcsc_norm_check( const pastix_spm_t *spm, const pastix_bcsc_t *bcsc );
int d_bcsc_norm_check( const pastix_spm_t *spm, const pastix_bcsc_t *bcsc );
int s_bcsc_norm_check( const pastix_spm_t *spm, const pastix_bcsc_t *bcsc );

#define PRINT_RES(_ret_)                        \
    if(_ret_) {                                 \
        printf("FAILED(%d)\n", _ret_);          \
        err++;                                  \
    }                                           \
    else {                                      \
        printf("SUCCESS\n");                    \
    }

char* fltnames[] = { "Pattern", "", "Float", "Double", "Complex32", "Complex64" };
char* mtxnames[] = { "General", "Symmetric", "Hermitian" };

int main (int argc, char **argv)
{
    pastix_data_t  *pastix_data = NULL; /* Pointer to a storage structure needed by pastix  */
    pastix_int_t    iparm[IPARM_SIZE];  /* integer parameters for pastix                    */
    double          dparm[DPARM_SIZE];  /* floating parameters for pastix                   */
    pastix_driver_t driver;             /* Matrix driver(s) requested by user               */
    pastix_spm_t   *spm, *spm2;
    pastix_bcsc_t   bcsc;
    char *filename;                     /* Filename(s) given by user                        */
    int ret = PASTIX_SUCCESS;
    int err = 0;

    /**
     * Initialize parameters to default values
     */
    pastixInitParam( iparm, dparm );
    pastixInit( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    /**
     * Get options from command line
     */
    pastix_ex_getoptions( argc, argv,
                          NULL, NULL,
                          &driver, &filename );

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
     * Run preprocessing steps required to generate the blocked csc
     */
    pastix_task_order( pastix_data, spm, NULL, NULL );
    pastix_task_symbfact( pastix_data, NULL, NULL );
    pastix_task_blend( pastix_data );

    /**
     * Generate the blocked csc
     */
    bcscInit( spm,
              pastix_data->ordemesh,
              pastix_data->solvmatr,
              spm->mtxtype == PastixGeneral, &bcsc );

    printf(" -- BCSC Norms Test --\n");
    printf(" Datatype: %s\n", fltnames[spm->flttype] );
    spmBase( spm, 0 );

    printf("   Matrix type : %s\n", mtxnames[spm->mtxtype - PastixGeneral] );

    switch( spm->flttype ){
    case PastixComplex64:
        ret = z_bcsc_norm_check( spm, &bcsc );
        break;

    case PastixComplex32:
        ret = c_bcsc_norm_check( spm, &bcsc );
        break;

    case PastixFloat:
        ret = s_bcsc_norm_check( spm, &bcsc );
        break;

    case PastixDouble:
    default:
        ret = d_bcsc_norm_check( spm, &bcsc );
    }
    PRINT_RES(ret);

    spmExit( spm );
    free( spm );
    bcscExit( &bcsc );
    pastixFinalize( &pastix_data, MPI_COMM_WORLD, iparm, dparm );

    if( err == 0 ) {
        printf(" -- All tests PASSED --\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf(" -- %d tests FAILED --\n", err);
        return EXIT_FAILURE;
    }
}