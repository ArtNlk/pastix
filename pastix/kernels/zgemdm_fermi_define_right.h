/**
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
/*
    -- MAGMA (version 1.1) --
       Univ. of Tennessee, Knoxville
       Univ. of California, Berkeley
       Univ. of Colorado, Denver
       November 2011
*/

#ifndef _ZGEMDM_FERMI_DEFINE_RIGHT_H_
#define _ZGEMDM_FERMI_DEFINE_RIGHT_H_

#define PRECISION_z

//#include "gemdm_stencil_defs.cu"
#include "gemdm_stencil_right.h"
///////////////////////////////////////////////////////////////////////////////////////////////////
// Common parameters

// size of work for a thread block
/* #define BLK_M_nn 24 */
/* #define BLK_N_nn 16 */
/* #define BLK_K_nn  8 */

/* #define BLK_M_nt 16 */
/* #define BLK_N_nt 24 */
/* #define BLK_K_nt  8 */

/* #define BLK_M_tt 16 */
/* #define BLK_N_tt 24 */
/* #define BLK_K_tt  8 */

/* #define BLK_M_tn 24 */
/* #define BLK_N_tn 16 */
/* #define BLK_K_tn  8 */

/* // size of thread block for calculating C (innermost loop) */
/* #define DIM_X  8 */
/* #define DIM_Y  8 */

/* // size of thread block for reading B (dev->regs->shmem) */
/* #define DIM_XB 8 */
/* #define DIM_YB 8 */


/* /////////////////////////////////////////////////////////////////////////////////////////////////// */
/* // */
/* //  NoTrans - NoTrans */
/* // */

/* // size of work for a thread block */
/* #define BLK_M BLK_M_nn */
/* #define BLK_N BLK_N_nn  */
/* #define BLK_K BLK_K_nn */

/* // size of thread block for reading A (dev->regs->shmem) */
/* #define DIM_XA 8 */
/* #define DIM_YA 8 */
  
/* #define version trans_nn */
/* #include "gemdm_stencil.cu" */
 
/* #undef BLK_M */
/* #undef BLK_N */
/* #undef BLK_K */

/* #undef DIM_XA */
/* #undef DIM_YA */

/* /////////////////////////////////////////////////////////////////////////////////////////////////// */
/* // */
/* //  NoTrans - Trans */
/* // */
 
/* // size of work for a thread block */
/* #define BLK_M BLK_M_nt */
/* #define BLK_N BLK_N_nt */
/* #define BLK_K BLK_K_nt */
  
/* // size of thread block for reading A (dev->regs->shmem) */
/* #define DIM_XA 8 */
/* #define DIM_YA 8 */

/* #define version trans_nt */
/* #include "gemdm_stencil.cu" */

/* #define version trans_nc */
/* #include "gemdm_stencil.cu" */

/* #undef BLK_M */
/* #undef BLK_N */
/* #undef BLK_K */

/* #undef DIM_XA */
/* #undef DIM_YA */

/* /////////////////////////////////////////////////////////////////////////////////////////////////// */
/* // */
/* //  Trans - Trans */
/* // */
 
/* // size of work for a thread block */
/* #define BLK_M BLK_M_tt */
/* #define BLK_N BLK_N_tt */
/* #define BLK_K BLK_K_tt */

/* // size of thread block for reading A (dev->regs->shmem) */
/* #define DIM_XA 4 */
/* #define DIM_YA 16 */

/* #define version trans_tt */
/* #include "gemdm_stencil.cu" */

/* #define version trans_tc */
/* #include "gemdm_stencil.cu" */

/* #define version trans_ct */
/* #include "gemdm_stencil.cu" */

/* #define version trans_cc */
/* #include "gemdm_stencil.cu" */

/* #undef BLK_M */
/* #undef BLK_N */
/* #undef BLK_K */

/* #undef DIM_XA */
/* #undef DIM_YA */

/* /////////////////////////////////////////////////////////////////////////////////////////////////// */
/* // */
/* //  Trans - NoTrans */
/* // */
 
/* // size of work for a thread block */
/* #define BLK_M BLK_M_tn */
/* #define BLK_N BLK_N_tn */
/* #define BLK_K BLK_K_tn */

/* // size of thread block for reading A (dev->regs->shmem) */
/* #define DIM_XA 8 */
/* #define DIM_YA 8 */

/* #define version trans_tn */
/* #include "gemdm_stencil.cu" */

/* #define version trans_cn */
/* #include "gemdm_stencil.cu" */

///////////////////////////////////////////////////////////////////////////////////////////////////

#endif /* _ZGEMDM_FERMI_DEFINE_RIGHT_H_ */