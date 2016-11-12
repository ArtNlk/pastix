/**
 *
 * @file spm_norm_dof_test.c
 *
 * Tests and validate the spm_norm routines when the spm hold constant and/or variadic dofs.
 *
 * @version 5.1.0
 * @author Mathieu Faverge
 * @author Theophile Terraz
 * @date 2015-01-01
 *
 **/
#define _GNU_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pastix.h>
#include "../matrix_drivers/drivers.h"
#include <spm.h>

int z_spm_norm_check( const pastix_spm_t *spm );
int c_spm_norm_check( const pastix_spm_t *spm );
int d_spm_norm_check( const pastix_spm_t *spm );
int s_spm_norm_check( const pastix_spm_t *spm );

void z_spm_print_check( char *filename, const pastix_spm_t *spm );
void c_spm_print_check( char *filename, const pastix_spm_t *spm );
void d_spm_print_check( char *filename, const pastix_spm_t *spm );
void s_spm_print_check( char *filename, const pastix_spm_t *spm );

#define PRINT_RES(_ret_)                        \
    if(_ret_) {                                 \
        printf("FAILED(%d)\n", _ret_);          \
        err++;                                  \
    }                                           \
    else {                                      \
        printf("SUCCESS\n");                    \
    }

char* fltnames[] = { "Pattern", "", "Float", "Double", "Complex32", "Complex64" };
char* fmtnames[] = { "CSC", "CSR", "IJV" };
char* mtxnames[] = { "General", "Symmetric", "Hermitian" };

int main (int argc, char **argv)
{
    pastix_spm_t    original, *spm;
    pastix_driver_t driver;
    char *filename;
    int spmtype, mtxtype, fmttype, baseval;
    int ret = PASTIX_SUCCESS;
    int err = 0;
    int i, dofmax = 6;

    /**
     * Get options from command line
     */
    pastix_ex_getoptions( argc, argv,
                          NULL, NULL,
                          &driver, &filename );

    spmReadDriver( driver, filename, &original, MPI_COMM_WORLD );
    free(filename);

    spmtype = original.mtxtype;
    printf(" -- SPM Dof Expand Test --\n");

    for( i=0; i<2; i++ )
    {
        spm = spmDofExtend( i, dofmax, &original );

        for( fmttype=0; fmttype<3; fmttype++ )
        {
            spmConvert( fmttype, spm );

            for( baseval=0; baseval<1; baseval++ )
            {
                spmBase( spm, baseval );

                for( mtxtype=PastixGeneral; mtxtype<=PastixHermitian; mtxtype++ )
                {
                    if ( (mtxtype == PastixHermitian) &&
                         ( ((spm->flttype != PastixComplex64) && (spm->flttype != PastixComplex32)) ||
                           (spmtype != PastixHermitian) ) )
                    {
                        continue;
                    }
                    if ( (mtxtype != PastixGeneral) &&
                         (spmtype == PastixGeneral) )
                    {
                        continue;
                    }
                    spm->mtxtype = mtxtype;

                    asprintf( &filename, "%d_%s_%d_%s_%s",
                              i, fmtnames[fmttype], baseval,
                              mtxnames[mtxtype - PastixGeneral],
                              fltnames[spm->flttype] );

                    switch( spm->flttype ){
                    case PastixComplex64:
                        z_spm_print_check( filename, spm );
                        break;

                    case PastixComplex32:
                        c_spm_print_check( filename, spm );
                        break;

                    case PastixFloat:
                        s_spm_print_check( filename, spm );
                        break;

                    case PastixDouble:
                    default:
                        d_spm_print_check( filename, spm );
                    }
                    free(filename);
                }
            }
        }
        spmExit( spm );
        free(spm);
    }
    spmExit( &original );

    return EXIT_SUCCESS;
}
