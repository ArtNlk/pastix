/**
 *
 * @file coeftab.h
 *
 * PaStiX coefficient array routines header.
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 * @addtogroup coeftab
 * @{
 *   This group collects all the functions that operate on the full matrix and
 *   which are not factorization/solve routines.
 *
 **/
#ifndef _COEFTAB_H_
#define _COEFTAB_H_

#include "sopalin/coeftab_z.h"
#include "sopalin/coeftab_c.h"
#include "sopalin/coeftab_d.h"
#include "sopalin/coeftab_s.h"

void coeftabInit( pastix_data_t     *pastix_data,
                  pastix_coefside_t  side );
void coeftabExit( SolverMatrix      *solvmtx );

/**
 * @brief Type of the memory gain functions
 */
typedef pastix_int_t (*coeftab_fct_memory_t)( const SolverMatrix * );

/**
 * @brief List of functions to compute the memory gain in low-rank per precision.
 */
coeftab_fct_memory_t coeftabMemory[4];

/**
 * @}
 */
#endif /* _COEFTAB_H_ */
