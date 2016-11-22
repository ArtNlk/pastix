/**
 *
 * @file sopalin_data.h
 *
 *  PaStiX sopalin routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Pascal Henon
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#ifndef _SOPALIN_DATA_H_
#define _SOPALIN_DATA_H_

struct sopalin_data_s {
    SolverMatrix *solvmtx;
    double        diagthreshold; /* Threshold for static pivoting on diagonal value */
};
typedef struct sopalin_data_s sopalin_data_t;

void coeftab_zdump( const SolverMatrix *solvmtx, const char *filename );
void coeftab_cdump( const SolverMatrix *solvmtx, const char *filename );
void coeftab_ddump( const SolverMatrix *solvmtx, const char *filename );
void coeftab_sdump( const SolverMatrix *solvmtx, const char *filename );

void sequential_ztrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag, sopalin_data_t *sopalin_data, int nrhs, pastix_complex64_t *b, int ldb );
void sequential_ctrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag, sopalin_data_t *sopalin_data, int nrhs, pastix_complex32_t *b, int ldb );
void sequential_dtrsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag, sopalin_data_t *sopalin_data, int nrhs, double *b, int ldb );
void sequential_strsm( pastix_data_t *pastix_data, int side, int uplo, int trans, int diag, sopalin_data_t *sopalin_data, int nrhs, float *b, int ldb );

void sequential_zdiag( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data, int nrhs, pastix_complex64_t *b, int ldb );
void sequential_cdiag( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data, int nrhs, pastix_complex32_t *b, int ldb );
void sequential_ddiag( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data, int nrhs, double *b, int ldb );
void sequential_sdiag( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data, int nrhs, float *b, int ldb );

void sopalin_zgetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_cgetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_dgetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_sgetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );

void sopalin_zhetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_chetrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );

void sopalin_zpotrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_cpotrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_dpotrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_spotrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );

void sopalin_zsytrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_csytrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_dsytrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );
void sopalin_ssytrf( pastix_data_t *pastix_data, sopalin_data_t *sopalin_data );

#endif /* _SOPALIN_DATA_H_ */


