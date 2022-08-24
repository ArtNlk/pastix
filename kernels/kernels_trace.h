/**
 *
 * @file kernels_trace.h
 *
 * Wrappers to trace kernels.
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
#ifndef _kernels_trace_h_
#define _kernels_trace_h_

#include "common.h"
#include "flops.h"
#include "kernels_enums.h"

/**
 * @brief Global array to store the number of flops executed per kernel
 */
extern volatile double kernels_flops[PastixKernelLvl1Nbr];

/**
 * @brief Lock to accumulate flops
 */
extern pastix_atomic_lock_t lock_flops;

/**
 * @brief Overall number of flops
 */
extern double overall_flops[3];

#if defined(PASTIX_GENERATE_MODEL)

/**
 * @brief Structure to store information linked to a kernel in order to generate
 * the cost model
 */
typedef struct pastix_model_entry_s {
    pastix_ktype_t ktype; /**< The type of the kernel             */
    int m;                /**< The first diemension of the kernel */
    int n;                /**< The second dimension of the kernel if present, 0 otherwise */
    int k;                /**< The third dimension of the kernel, 0 otherwise             */
    double time;          /**< The time spent in the kernel (s)                           */
} pastix_model_entry_t;

extern pastix_model_entry_t *model_entries;     /**< Array to all entries                 */
extern volatile int32_t      model_entries_nbr; /**< Index of the last entry in the array */
extern int32_t               model_size;        /**< Size of the model_entries array      */

#endif

void   kernelsTraceStart( const pastix_data_t *pastix_data );
double kernelsTraceStop(  const pastix_data_t *pastix_data );

/**
 *******************************************************************************
 *
 * @brief Start the trace of a single kernel
 *
 *******************************************************************************
 *
 * @param[in] ktype
 *          Type of the kernel starting that need to be traced.
 *          With EZTrace mode, this call is empty if the environment variable
 *          PASTIX_EZTRACE_LEVEL is different from 1.
 *
 *******************************************************************************
 *
 * @return the starting time if PASTIX_GENERATE_MODEL is enabled, 0. otherwise.
 *
 *******************************************************************************/
static inline double
kernel_trace_start( pastix_ktype_t ktype )
{
    double time = 0.;

#if defined(PASTIX_WITH_EZTRACE)

    if (pastix_eztrace_level == 1) {
        EZTRACE_EVENT_PACKED_0( KERNELS_LVL1_CODE(ktype) );
    }

#endif

#if defined(PASTIX_GENERATE_MODEL)

    time = clockGet();

#endif

    (void)ktype;
    return time;
}

/**
 *******************************************************************************
 *
 * @brief Stop the trace of a single kernel
 *
 *******************************************************************************
 *
 * @param[in] ktype
 *          Type of the kernel starting that need to be traced.
 *          With EZTrace mode, this call is empty if the environment variable
 *          PASTIX_EZTRACE_LEVEL is different from 1.
 *
 * @param[in] m
 *          The m parameter of the kernel (used by xxTRF, TRSM, and GEMM)
 *
 * @param[in] n
 *          The n parameter of the kernel (used by TRSM, and GEMM)
 *
 * @param[in] k
 *          The k parameter of the kernel (used by GEMM)
 *
 * @param[in] flops
 *          The number of flops of the kernel
 *
 * @param[in] starttime
 *          The stating time of the kernel. Used only if PASTIX_GENERATE_MODEL
 *          is enabled.
 *
 *******************************************************************************/
static inline void
kernel_trace_stop( int8_t inlast, pastix_ktype_t ktype, int m, int n, int k, double flops, double starttime )
{

#if defined(PASTIX_WITH_EZTRACE)

    if (pastix_eztrace_level == 1) {
        EZTRACE_EVENT_PACKED_1( KERNELS_CODE( PastixKernelStop ), flops );
    }

#endif

#if defined(PASTIX_GENERATE_MODEL)

    {
        double  time = clockGet() - starttime;
        int32_t index = pastix_atomic_inc_32b( &model_entries_nbr );

        if ( index < model_size ) {
            model_entries[index].ktype = ktype;
            model_entries[index].m     = m;
            model_entries[index].n     = n;
            model_entries[index].k     = k;
            model_entries[index].time  = time;
        }
        else {
            fprintf(stderr, "WARNING: too many kernels to log %d\n", index);
        }
    }

#endif

    pastix_atomic_lock( &lock_flops );
    overall_flops[inlast] += flops;
    pastix_atomic_unlock( &lock_flops );

    (void)ktype;
    (void)m;
    (void)n;
    (void)k;
    (void)flops;
    (void)starttime;
    return;
}

/**
 * @}
 */
#endif /* _kernels_trace_h_ */
