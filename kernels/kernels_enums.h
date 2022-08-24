/**
 *
 * @file kernels_enums.h
 *
 * Wrappers to trace enums kernels.
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.0
 * @author Gregoire Pichon
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @date 2021-03-16
 *
 * @addtogroup eztrace_dev
 * @{
 *
 **/
#ifndef _kernels_enums_h_
#define _kernels_enums_h_

/**
 * @brief Main stop enum event for all the events in traces
 */
#define PastixKernelStop 0

/**
 * @brief List of the Level 0 events that may be traced in PaStiX
 *
 * This is only the high level steps.
 */
typedef enum pastix_ktype0_e {
    PastixKernelLvl0Facto,
    PastixKernelLvl0Solve,
    PastixKernelLvl0Diag,
    PastixKernelLvl0Nbr
} pastix_ktype0_t;

/**
 * @brief List of the Level 1 events that may be traced in PaStiX
 *
 * This is the main information that traces all the major kernels during the
 * factorization step.
 */
typedef enum pastix_ktype_e {
    PastixKernelGETRF,        /**< LU diagonal block kernel             */
    PastixKernelHETRF,        /**< LDLh diagonal block kernel           */
    PastixKernelPOTRF,        /**< Cholesky diagonal block kernel       */
    PastixKernelPXTRF,        /**< Complex LL^t diagonal block kernel   */
    PastixKernelSYTRF,        /**< LDLt diagonal block kernel           */
    PastixKernelSCALOCblk,    /**< Scaling out-of-place of a panel      */
    PastixKernelSCALOBlok,    /**< Scaling out-of-place of a block      */
    PastixKernelTRSMCblk1d,   /**< TRSM applied to a panel in 1d layout */
    PastixKernelTRSMCblk2d,   /**< TRSM applied to a panel in 2d layout */
    PastixKernelTRSMCblkLR,   /**< TRSM applied to a panel in low-rank  */
    PastixKernelTRSMBlok2d,   /**< TRSM applied to a block in 2d layout */
    PastixKernelTRSMBlokLR,   /**< TRSM applied to a block in low-rank  */
    PastixKernelGEMMCblk1d1d, /**< GEMM applied from a panel in 1d layout to a panel in 1d layout */
    PastixKernelGEMMCblk1d2d, /**< GEMM applied from a panel in 1d layout to a panel in 2d layout */
    PastixKernelGEMMCblk2d2d, /**< GEMM applied from a panel in 2d layout to a panel in 2d layout */
    PastixKernelGEMMCblkFRLR, /**< GEMM applied from a panel in full-rank to a panel in low-rank  */
    PastixKernelGEMMCblkLRLR, /**< GEMM applied from a panel in low-rank to a panel in low-rank   */
    PastixKernelGEMMBlok2d2d, /**< GEMM applied from a block in 2d layout to a block in 2d layout */
    PastixKernelGEMMBlokLRLR, /**< GEMM applied from a block in low-rank to a block in low-rank   */
    PastixKernelGEADDCblkFRFR, /**< GEADD applied from a panel in full-rank to a panel in full-rank  */
    PastixKernelGEADDCblkFRLR, /**< GEADD applied from a panel in full-rank to a panel in low-rank  */
    PastixKernelGEADDCblkLRLR, /**< GEADD applied from a panel in low-rank to a panel in low-rank   */
    PastixKernelLvl1Nbr
} pastix_ktype_t;

/**
 * @brief List of the Level 2 events that may be traced in PaStiX
 *
 * This is the low-level information that traces all the individual calls to
 * blas/lapack routines in the code. It is used to compute the number of flops
 * in low-rank compression, and to distinguish the amount of flops spent in each
 * part of the low-rank updates.
 *
 */
typedef enum pastix_ktype2_e {

    /* General kernels: similar in low-rank and dense */
    PastixKernelLvl2GETRF,             /**< LU diagonal block kernel           */
    PastixKernelLvl2HETRF,             /**< LDLh diagonal block kernel         */
    PastixKernelLvl2POTRF,             /**< Cholesky diagonal block kernel     */
    PastixKernelLvl2PXTRF,             /**< Complex LL^t diagonal block kernel */
    PastixKernelLvl2SYTRF,             /**< LDLt diagonal block kernel         */

    /* Solve operations */
    PastixKernelLvl2_FR_TRSM,
    PastixKernelLvl2_LR_TRSM,

    /* Update operations */
    PastixKernelLvl2_FR_GEMM,

    /* Formation (and application) of A * B */
    PastixKernelLvl2_LR_FRFR2FR,
    PastixKernelLvl2_LR_FRLR2FR,
    PastixKernelLvl2_LR_LRFR2FR,
    PastixKernelLvl2_LR_LRLR2FR,
    PastixKernelLvl2_LR_FRFR2LR,
    PastixKernelLvl2_LR_FRLR2LR,
    PastixKernelLvl2_LR_LRFR2LR,
    PastixKernelLvl2_LR_LRLR2LR,
    PastixKernelLvl2_LR_FRFR2null,
    PastixKernelLvl2_LR_FRLR2null,
    PastixKernelLvl2_LR_LRFR2null,
    PastixKernelLvl2_LR_LRLR2null,

    /* Compression kernels */
    PastixKernelLvl2_LR_init_compress,
    PastixKernelLvl2_LR_add2C_uncompress,
    PastixKernelLvl2_LR_add2C_recompress,
    PastixKernelLvl2_LR_add2C_updateCfr,
    PastixKernelLvl2_LR_add2C_orthou,
    PastixKernelLvl2_LR_add2C_rradd_orthogonalize, /**<< CGS, partialQR or fullQR */
    PastixKernelLvl2_LR_add2C_rradd_recompression,
    PastixKernelLvl2_LR_add2C_rradd_computeNewU,

    PastixKernelLvl2Nbr
} pastix_ktype2_t;

/**
 * @brief Total number of kernel events
 */
#define PastixKernelsNbr (PastixKernelLvl0Nbr + PastixKernelLvl1Nbr + PastixKernelLvl2Nbr)

#if defined(PASTIX_WITH_EZTRACE)

#include "eztrace_module/kernels_ev_codes.h"
/**
 * @brief Define the level traced by the EZTrace module
 */
extern int pastix_eztrace_level;

#else

static inline void kernel_trace_start_lvl0     ( pastix_ktype0_t ktype )  { (void)ktype; }
static inline void kernel_trace_stop_lvl0      ( double flops )           { (void)flops; }
static inline void kernel_trace_start_lvl2     ( pastix_ktype2_t ktype )  { (void)ktype; }
static inline void kernel_trace_stop_lvl2      ( double flops )           { (void)flops; }
static inline void kernel_trace_stop_lvl2_rank ( double flops, int rank ) { (void)flops; (void)rank; }

#endif

/**
 * @}
 */
#endif /* _kernels_enums_h_ */
