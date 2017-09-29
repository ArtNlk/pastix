/**
 *
 * @file pastix_zparsec.h
 *
 * Pastix PaRSEC codelets header
 *
 * @copyright 2016-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2013-06-24
 *
 * @precisions normal z -> z c d s
 *
 **/

void
parsec_zpotrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data );

void
parsec_zpxtrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data );

void
parsec_zgetrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data );

void
parsec_zhetrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data );

void
parsec_zsytrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data );


