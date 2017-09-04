/**
 * @file coeftab_z.h
 *
 * Precision dependent coeficient array header.
 *
 * @copyright 2012-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author David Goudin
 * @author Pascal Henon
 * @author Francois Pellegrini
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Xavier Lacoste
 * @date 2011-11-11
 *
 * @precisions normal z -> s d c
 *
 * @addtogroup coeftab
 * @{
 *
 **/
#ifndef _coeftab_z_h_
#define _coeftab_z_h_

/**
 *    @name PastixComplex64 compression/uncompression routines
 *    @{
 */
pastix_int_t coeftab_zcompress  ( SolverMatrix *solvmtx );
void         coeftab_zuncompress( SolverMatrix *solvmtx );
pastix_int_t coeftab_zmemory    ( const SolverMatrix *solvmtx );

/**
 *    @}
 *    @name PastixComplex64 initialization routines
 *    @{
 */
/**
 *    @}
 *    @name PastixComplex64 Schur routines
 *    @{
 */
void coeftab_zgetschur( const SolverMatrix *solvmtx,
                        pastix_complex64_t *S, pastix_int_t lds );

/**
 *    @}
 *    @name PastixComplex64 debug routines
 *    @{
 */
void coeftab_zdump    ( pastix_data_t      *pastix_data,
                        const SolverMatrix *solvmtx,
                        const char         *filename );
int  coeftab_zdiff    ( const SolverMatrix *solvA,
                        SolverMatrix       *solvB );

/**
 *    @}
 */
#endif /* _coeftab_z_h_ */

/**
 * @}
 */
