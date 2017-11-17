/**
 * @file pastix_lowrank.h
 *
 * PaStiX kernel header.
 *
 * @copyright 2011-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2017-10-05
 * @precisions normal z -> c d s
 *
 */
#ifndef _pastix_lowrank_h_
#define _pastix_lowrank_h_

/**
 * @brief Macro to specify if the U part of a low-rank matrix is orthogonal or not (Used in LRMM functions).
 */
#define PASTIX_LRM3_ORTHOU (1 << 0)
/**
 * @brief Macro to specify if the U part of a low-rank matrix has been allocated and need to be freed or not (Used in LRMM functions).
 */
#define PASTIX_LRM3_ALLOCU (1 << 1)
/**
 * @brief Macro to specify if the V part of a low-rank matrix has been allocated and need to be freed or not (Used in LRMM functions).
 */
#define PASTIX_LRM3_ALLOCV (1 << 2)
/**
 * @brief Macro to specify if the the operator on B, still needs to be applied to the V part of the low-rank matrix or not (Used in LRMM functions).
 */
#define PASTIX_LRM3_TRANSB (1 << 3)

/**
 * @brief Define the minmal ratio for which we accept to compress a matrix into a low-rank form or not (@sa core_get_rklimit()).
 */
#define PASTIX_LR_MINRATIO 4

/**
 * @brief Compute the maximal rank accepted for a given matrix size
 * @param[in] M The number of rows of the matrix
 * @param[in] N The number of columns of the matrix
 * @return The maximal rank accepeted for this matrix size.
 */
static inline pastix_int_t
core_get_rklimit( pastix_int_t M, pastix_int_t N ) {
    return pastix_imin( M, N ) / PASTIX_LR_MINRATIO;
}

struct pastix_lr_s;
typedef struct pastix_lr_s pastix_lr_t;

/**
 * @brief The block low-rank structure to hold a matrix in low-rank form
 */
typedef struct pastix_lrblock_s {
    int   rk;    /**< Rank of the low-rank matrix: -1 is dense, otherwise rank-rk matrix           */
    int   rkmax; /**< Leading dimension of the matrix u                                            */
    void *u;     /**< Contains the dense matrix if rk=-1, or the u factor from u vT representation */
    void *v;     /**< Not referenced if rk=-1, otherwise, the v factor                             */
} pastix_lrblock_t;

/**
 * @brief Type of the functions to compress a dense block into a low-rank form.
 */
typedef pastix_fixdbl_t (*fct_ge2lr_t)( pastix_fixdbl_t, pastix_int_t, pastix_int_t, pastix_int_t,
                                        const void *, pastix_int_t, pastix_lrblock_t * );

/**
 * @brief Type of the functions to add two low-rank blocks together.
 */
typedef pastix_fixdbl_t (*fct_rradd_t)( const pastix_lr_t *, pastix_trans_t, const void *,
                                        pastix_int_t, pastix_int_t, const pastix_lrblock_t *,
                                        pastix_int_t, pastix_int_t,       pastix_lrblock_t *,
                                        pastix_int_t, pastix_int_t );

/**
 * @brief Structure to define the type of function to use for the low-rank
 *        kernels and their parmaeters.
 */
typedef struct pastix_lr_s {
    pastix_int_t compress_when;       /**< When to compress in the full solver              */
    pastix_int_t compress_method;     /**< Compression method                               */
    pastix_int_t compress_min_width;  /**< Minimum width to compress a supernode            */
    pastix_int_t compress_min_height; /**< Minimum height to compress an off-diagonal block */
    double       tolerance;           /**< Absolute compression tolerance                   */
    fct_rradd_t  core_rradd;          /**< Recompression function                           */
    fct_ge2lr_t  core_ge2lr;          /**< Compression function                             */
} pastix_lr_t;

/**
 * @}
 */
#endif /* _pastix_lowrank_h_ */
