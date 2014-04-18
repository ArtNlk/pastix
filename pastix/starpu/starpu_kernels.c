#ifdef PASTIX_WITH_MAGMABLAS
#include <magmablas.h>
#endif /* PASTIX_WITH_MAGMABLAS */

#include "starpu_defines.h"
#include "common.h"
#include "sopalin_thread.h"
#include "sopalin_acces.h"
#include "sopalin3d.h"
#include "sopalin_compute.h"
#include "pastix_dcores.h"
#include "starpu_kernels.h"
#include "starpu_submit_tasks.h"
#include "compute_diag.h"
#include "compute_trsm.h"


#ifdef PASTIX_WITH_CUDA
#  include "sparse_gemm.h"
#  include "pastix_cuda_helper.h"
#endif

#include <inttypes.h>

#ifdef PASTIX_WITH_CUDA
#  ifdef STARPU_USE_CUDA
#    if ((!defined PREC_DOUBLE)  || (!(defined __CUDA_ARCH__) || __CUDA_ARCH__ >= 130))
#      if !(defined PREC_DOUBLE && defined TYPE_COMPLEX && CUDA_SM_VERSION < 20)
#        ifndef FORCE_NO_CUDA
#          define STARPU_USE_CUDA_GEMM_FUNC
#        endif
#      endif
#     endif
#  endif
#endif


#if (!defined STARPU_USE_CUDA || defined FORCE_NO_CUDA)
#  ifdef  PASTIX_WITH_MAGMABLAS
#    undef  PASTIX_WITH_MAGMABLAS
#  endif /* PASTIX_WITH_MAGMABLAS */
#endif /* !STARPU_USE_CUDA || FORCE_NO_CUDA*/


#ifdef PASTIX_WITH_MAGMABLAS
#include "geadd_cuda.h"
#include "getra_cuda.h"
#include "zgetrf_stapiv_gpu.h"
#endif /* PASTIX_WITH_MAGMABLAS */

#define DimTrans                API_CALL(DimTrans)
#define kernel_trsm             API_CALL(kernel_trsm)
#define CORE_gemdm              API_CALL(CORE_gemdm)

#if !(defined STARPU_USE_CUDA_GEMM_FUNC)
#define CUDA_SPARSE_GEMM(TRANSA, TRANSB,                \
                         dimi, dimj, dima,              \
                         alpha,                         \
                         A,  stride_A,                  \
                         B,  stride_B,                  \
                         beta,                          \
                         C, stride_C,                   \
                         blocknbr,  blocktab,           \
                         fblocknbr, fblocktab)          \
  do {                                                  \
  } while(0)
#define CUDA_SPARSE_GEMDM(TRANSA, TRANSB,                \
                          dimi, dimj, dima,              \
                          alpha,                         \
                          A,  stride_A,                  \
                          D,  stride_D,                  \
                          B,  stride_B,                  \
                          beta,                          \
                          C, stride_C,                   \
                          blocknbr,  blocktab,           \
                          fblocknbr, fblocktab)          \
  do {                                                   \
    assert(0);                                           \
    /* avoid warnings */                                 \
    assert(fblocktab);                                   \
    assert(blocktab);                                    \
    assert(fblocknbr);                                   \
    assert(blocknbr);                                    \
    assert(beta);                                        \
    assert(alpha);                                       \
    assert(dima);                                        \
    assert(dimj);                                        \
    assert(dimi);                                        \
    assert(stridefc);                                    \
  } while(0)
#else
#if (CUDA_SM_VERSION >= 20)
#define CUDA_SPARSE_GEMM(TRANSA, TRANSB,                                \
                         dimi, dimj, dima,                              \
                         alpha,                                         \
                         A,  stride_A,                                  \
                         B,  stride_B,                                  \
                         beta,                                          \
                         C, stride_C,                                   \
                         blocknbr, blocktab,                            \
                         fblocknbr, fblocktab)                          \
  do {                                                                  \
    CU_FLOAT cu_alpha = CU_FLOAT_INIT(creal(alpha), cimag(alpha));      \
    CU_FLOAT cu_beta  = CU_FLOAT_INIT(creal(beta),  cimag(beta));       \
    GENERATE_SM_VERSION_NAME(gemm)(*TRANSA, *TRANSB,                    \
                                   (int)dimi, (int)dimj, (int)dima,     \
                                   cu_alpha,                            \
                                   (CU_FLOAT*)A, (int)stride_A,         \
                                   (CU_FLOAT*)B, (int)stride_B,         \
                                   cu_beta,                             \
                                   (CU_FLOAT*)C, (int)stride_C,         \
                                   blocknbr,  blocktab,                 \
                                   fblocknbr, fblocktab,                \
                                   starpu_cuda_get_local_stream());     \
    cudaStreamSynchronize(starpu_cuda_get_local_stream());              \
  } while(0)
#define CUDA_SPARSE_GEMDM(TRANSA, TRANSB,                               \
                          dimi, dimj, dima,                             \
                          alpha,                                        \
                          A,  stride_A,                                 \
                          D, stride_D,                                  \
                          B,  stride_B,                                 \
                          beta,                                         \
                          C, stride_C,                                  \
                          blocknbr, blocktab,                           \
                          fblocknbr, fblocktab)                         \
    do {                                                                \
      CU_FLOAT cu_alpha = CU_FLOAT_INIT(creal(alpha), cimag(alpha));    \
      CU_FLOAT cu_beta  = CU_FLOAT_INIT(creal(beta),  cimag(beta));     \
      GENERATE_SM_VERSION_NAME(gemdm)(*TRANSA, *TRANSB,                 \
                                      (int)dimi, (int)dimj, (int)dima,  \
                                      cu_alpha,                         \
                                      (CU_FLOAT*)A, (int)stride_A,      \
                                      (CU_FLOAT*)D, (int)stride_D,      \
                                      (CU_FLOAT*)B, (int)stride_B,      \
                                      cu_beta,                          \
                                      (CU_FLOAT*)C, (int)stride_C,      \
                                      blocknbr,  blocktab,              \
                                      fblocknbr, fblocktab,             \
                                      starpu_cuda_get_local_stream());  \
    cudaStreamSynchronize(starpu_cuda_get_local_stream());              \
    } while(0)
#else
#define CUDA_SPARSE_GEMM(TRANSA, TRANSB,                                \
                         dimi, dimj, dima,                              \
                         alpha,                                         \
                         A,  stride_A,                                  \
                         B,  stride_B,                                  \
                         beta,                                          \
                         C, stride_C,                                   \
                         blocknbr, blocktab,                            \
                         fblocknbr, fblocktab)                          \
  do {                                                                  \
    magmablas_sparse_gemm_kernel_N_T_64_16_4_16_4((int)dimi,            \
                                                  (int)dimj,            \
                                                  (int)dima,            \
                                                  (float)alpha,         \
                                                  A,                    \
                                                  (int)stride_A,        \
                                                  B,                    \
                                                  (int)stride_B,        \
                                                  (float)beta,          \
                                                  C,                    \
                                                  (int)stride_C,        \
                                                  blocknbr,             \
                                                  blocktab,             \
                                                  fblocknbr,            \
                                                  fblocktab);           \
    cudaStreamSynchronize(starpu_cuda_get_local_stream());              \
  } while(0)
#define CUDA_SPARSE_GEMDM(TRANSA, TRANSB,                               \
                          dimi, dimj, dima,                             \
                          alpha,                                        \
                          A,  stride_A,                                 \
                          D,  stride_D,                                 \
                          B,  stride_B,                                 \
                          beta,                                         \
                          C, stride_C,                                  \
                          blocknbr, blocktab,                           \
                          fblocknbr, fblocktab)                         \
  do {                                                                  \
    magmablas_sparse_gemdm_kernel_N_T_64_16_4_16_4((int)dimi,           \
                                                   (int)dimj,           \
                                                   (int)dima,           \
                                                   (float)alpha,        \
                                                   A,                   \
                                                   (int)stride_A,       \
                                                   D,                   \
                                                   (int)stride_D,       \
                                                   B,                   \
                                                   (int)stride_B,       \
                                                   (float)beta,         \
                                                   C,                   \
                                                   (int)stride_C,       \
                                                   blocknbr,            \
                                                   blocktab,            \
                                                   fblocknbr,           \
                                                   fblocktab);          \
    cudaStreamSynchronize(starpu_cuda_get_local_stream());              \
  } while(0)
#endif
#endif


#define DECLARE_ARGS_GEMM                                               \
      Sopalin_Data_t     * sopalin_data;                                \
      SolverMatrix       * datacode;                                    \
      pastix_int_t           cblknum;                                     \
      pastix_int_t           bloknum;                                     \
      pastix_int_t           tasknum;                                     \
      pastix_int_t           fcblknum;                                    \
      pastix_int_t           stride;                                      \
      pastix_int_t           indblok;                                     \
      pastix_int_t           dimi;                                        \
      pastix_int_t           dimj;                                        \
      pastix_int_t           dima;                                        \
      int                  blocknbr;                                    \
      int                  fblocknbr;                                   \
      int                * blocktab;                                    \
      SolverCblk *cblk, *fcblk;                                         \
      SolverBlok *blok;                                                 \
      int                * fblocktab

#define UNPACK_ARGS_GEMM(_args, sopalin_data, cblknum, bloknum,         \
                         all_blocktab, tasknum, datacode,               \
                         fcblknum, indblok,                             \
                         blocktab, fblocktab, stride, dimi, dimj, dima, \
                         blocknbr, fblocknbr)                           \
      do {                                                              \
          starpu_codelet_unpack_args(_args, &sopalin_data,              \
                                     &cblk, &blok, &fcblk);             \
        datacode  = sopalin_data->datacode;                             \
        if (cblk_islocal(datacode, fcblk)) {                            \
            fcblknum  = fcblk - datacode->cblktab;                      \
            fblocktab = &(all_blocktab[2*SYMB_BLOKNUM(fcblknum)]);      \
            fblocknbr = CBLK_BLOKNBR( fcblknum);                        \
        } else {                                                        \
            assert(cblk_isfanin(datacode, fcblk));                      \
            fcblknum = fcblk_getnum(datacode, fcblk, SOLV_PROCNUM);     \
            fblocknbr = fcblk[1].fblokptr - fcblk[0].fblokptr;          \
        }                                                               \
        if (cblk_ishalo(datacode, cblk)) {                              \
            /* HALO Gemm*/                                              \
            pastix_int_t hcblk;                                         \
            hcblk = hcblk_getnum(datacode, cblk);                       \
            cblknum = -hcblk-1;                                         \
            bloknum = blok-datacode->hbloktab;                          \
            blocktab  = &(all_blocktab[2*(SYMB_BLOKNBR + bloknum)]);    \
        } else {                                                        \
            if (cblk_islocal(datacode, cblk)) {                         \
                cblknum   = cblk-datacode->cblktab;                     \
                bloknum = blok-datacode->bloktab;                       \
                blocktab  = &(all_blocktab[2*bloknum]);                 \
            }                                                           \
        }                                                               \
        stride = cblk->stride;                                          \
        indblok = blok->coefind;                                        \
        dimi = stride - indblok;                                        \
        dimj = blok->lrownum - blok->frownum + 1;                       \
        dima = cblk_colnbr(cblk);                                       \
        blocknbr = cblk[1].fblokptr - blok;                             \
        tasknum = cblknum;                                              \
      } while (0)

#define DECLARE_ARGS_TRSM                                               \
        Sopalin_Data_t    * sopalin_data;                               \
        SolverMatrix      * datacode;                                   \
        pastix_int_t          tasknum;                                  \
        SolverCblk * cblk;                                              \
        pastix_int_t          cblknum;                                  \
        pastix_int_t          fblknum;                                  \
        pastix_int_t          lblknum;                                  \
        pastix_int_t          dima;                                     \
        pastix_int_t          dimb



#define UNPACK_ARGS_TRSM(_args, sopalin_data, cblknum, tasknum,         \
                         datacode, fblknum, lblknum, dima, dimb)        \
        do {                                                            \
            starpu_codelet_unpack_args(_args, &sopalin_data, &cblk);    \
            datacode = sopalin_data->datacode;                          \
            cblknum  = cblk - datacode->cblktab;                        \
            tasknum  = cblknum;                                         \
            fblknum  = cblk->fblokptr - datacode->bloktab;              \
            lblknum  = cblk[1].fblokptr - datacode->bloktab;            \
            dima     = cblk->lcolnum - cblk->fcolnum + 1;               \
            dimb     = stride - dima;                                   \
        } while (0)

/* #define SOPALIN_SPARSE_GEMM(TRANSA, TRANSB,                     \ */
/*                             dimi, dimj, dima,                   \ */
/*                             alpha,  A,  stride_A,               \ */
/*                             B,  stride_B,                       \ */
/*                             beta,  C, stride_C,                 \ */
/*                             nblocs, blocs_idx, facing_bloc_idx, \ */
/*                             wtmp, wtmpsize)                     \ */
/*   do {                                                          \ */
/*     sparse_gemm(TRANSA, TRANSB,                                 \ */
/*                 dimi, dimj, dima,                               \ */
/*                 alpha,  A,  stride_A,                           \ */
/*                 B,  stride_B,                                   \ */
/*                 beta,  C, stride_C,                             \ */
/*                 nblocs, blocs_idx, facing_bloc_idx,             \ */
/*                 wtmp, wtmpsize);                                \ */
/*   } while(0) */

#define SOPALIN_SPARSE_GEMM(TRANSA, TRANSB,                 \
                            dimi, dimj, dima,               \
                            alpha,  A,  stride_A,           \
                            B,  stride_B,                   \
                            beta,  C, stride_C,             \
                            blocknbr, blocktab,             \
                            fblocknbr, fblocktab,           \
                            wtmp, wtmpsize)                 \
  do {                                                      \
    assert(0);                                              \
    assert(wtmp); assert(wtmpsize);                         \
    /* Avoid warnings */                                    \
    assert(fblocktab);                                      \
    assert(blocktab );                                      \
    assert(fblocknbr);                                      \
    assert(blocknbr );                                      \
    assert(beta     );                                      \
    assert(alpha    );                                      \
    assert(dima     );                                      \
    assert(dimj     );                                      \
    assert(dimi     );                                      \
    assert(stridefc );                                      \
  } while(0)

#ifdef TYPE_COMPLEX
#  ifdef PREC_DOUBLE
#    define CUDA_TRSM(RL, UL, transa, transb, dimb, dima,        \
                      one_cuf,                                   \
                      A, stridea,                                \
                      B, strideb) do {                           \
      cublasZtrsm(RL, UL, transa, transb, dimb, dima, one_cuf,   \
                  (CU_FLOAT*)((void*)A), stridea,                \
                  (CU_FLOAT*)((void*)B), strideb);               \
    } while(0)
#  else /* not PREC_DOUBLE */
#    define CUDA_TRSM(RL, UL, transa, transb, dimb, dima,        \
                      one_cuf,                                   \
                      A, stridea,                                \
                      B, strideb) do {                           \
      cublasCtrsm(RL, UL, transa, transb, dimb, dima, one_cuf,   \
                  (CU_FLOAT*)((void*)A), stridea,                \
                  (CU_FLOAT*)((void*)B), strideb);               \
    } while(0)
#  endif /* not PREC_DOUBLE */
#else /* not TYPE_COMPLEX */
#  ifdef PREC_DOUBLE
#    define CUDA_TRSM(RL, UL, transa, transb, dimb, dima,        \
                      one_cuf,                                   \
                      A, stridea,                                \
                      B, strideb) do {                           \
      cublasDtrsm(RL, UL, transa, transb, dimb, dima, one_cuf,   \
                  (CU_FLOAT*)((void*)A), stridea,                \
                  (CU_FLOAT*)((void*)B), strideb);               \
    } while(0)
#  else /* not PREC_DOUBLE */
#    define CUDA_TRSM(RL, UL, transa, transb, dimb, dima,        \
                      one_cuf,                                   \
                      A, stridea,                                \
                      B, strideb) do {                           \
      cublasStrsm(RL, UL, transa, transb, dimb, dima, one_cuf,   \
                  (CU_FLOAT*)((void*)A), stridea,                \
                  (CU_FLOAT*)((void*)B), strideb);               \
    } while(0)
#  endif /* not PREC_DOUBLE */
#endif /* not TYPE_COMPLEX */

/*
 * Function: xxtrf_starpu_common
 *
 * Diagonal block factorization.
 *
 * Parameters:
 *   buffers    - Data handlers :
 *     0            - L column block
 *     1            - U column block
 *   _args      - Codelet arguments:
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 */
static inline
void xxtrf_starpu_common(void * buffers[], void * _args, int arch) {
  pastix_float_t      * lDiag        = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
  pastix_int_t          stride       = STARPU_MATRIX_GET_LD(buffers[0]);
  pastix_float_t      * lExtraDiag   = NULL;
  int                 me           = starpu_worker_get_id();
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  pastix_float_t      * uDiag        = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
  pastix_float_t      * uExtraDiag   = NULL;
#endif
#ifndef CHOL_SOPALIN
  pastix_float_t      * tmp4         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[1]);
#endif
#ifdef PASTIX_WITH_MAGMABLAS
  magma_int_t         info;
#endif /* PASTIX_WITH_MAGMABLAS */

  DECLARE_ARGS_TRSM;
  UNPACK_ARGS_TRSM(_args, sopalin_data, cblknum, tasknum, datacode,
                   fblknum, lblknum, dima, dimb);

  switch(arch) {
  case ARCH_CPU:
#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
    /* Add U diagonal updates into L */
    SOPALIN_GEAM("T", "N", dima, dima, 1.0,
                 uDiag, stride,
                 lDiag, stride);
    /* Factorize diagonal block (two terms version with workspace) */
    PASTIX_getrf_block(lDiag, dima, dima, stride,
                       &(sopalin_data->thread_data[me]->nbpivot),
                       sopalin_data->critere);
    /* Transpose L_diag in U_diag Matrix */
    DimTrans(lDiag,stride, dima,uDiag);
#  else /* SOPALIN_LU */
    PASTIX_potrf_block(lDiag, dima, stride,
                       &(sopalin_data->thread_data[me]->nbpivot),
                       sopalin_data->critere);

#  endif /* SOPALIN_LU */
#else /* CHOL_SOPALIN */
#  ifdef HERMITIAN
    PASTIX_hetrf_block(lDiag, dima, stride,
                       &(sopalin_data->thread_data[me]->nbpivot),
                       sopalin_data->critere,
                       sopalin_data->thread_data[me]->maxbloktab1);
#  else
    PASTIX_sytrf_block(lDiag, dima, stride,
                       &(sopalin_data->thread_data[me]->nbpivot),
                       sopalin_data->critere,
                       sopalin_data->thread_data[me]->maxbloktab1);
#  endif
#endif /* CHOL_SOPALIN */
      break;
#ifdef PASTIX_WITH_MAGMABLAS
  case ARCH_CUDA:
#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
    geadd_cuda("T", "N", dima, dima,
               1.0, uDiag, stride,
               1.0, lDiag, stride);
    magma_zgetrf_stapiv_gpu(dima, dima, (CU_FLOAT*)lDiag, stride,
                            sopalin_data->critere,
                            &(sopalin_data->thread_data[me]->nbpivot), &info);
    getra_cuda(lDiag, stride, uDiag, stride, dima);
#  else /* SOPALIN_LU */
    magma_zpotrf_stapiv_gpu("L", dima, (CU_FLOAT*)lDiag, stride,
                            sopalin_data->critere,
                            &(sopalin_data->thread_data[me]->nbpivot), &info);
#  endif /* SOPALIN_LU */
#else /* CHOL_SOPALIN */
#  ifdef HERMITIAN
    magma_zhetrf_stapiv_gpu('L', dima,
                            lDiag, stride,
                            sopalin_data->critere,
                            &(sopalin_data->thread_data[me]->nbpivot),
                            sopalin_data->thread_data[me]->maxbloktab1,
                            info);
#  else
    magma_zsytrf_stapiv_gpu('L', dima,
                            lDiag, stride,
                            sopalin_data->critere,
                            &(sopalin_data->thread_data[me]->nbpivot),
                            sopalin_data->thread_data[me]->maxbloktab1,
                            info);
#  endif
#endif /* CHOL_SOPALIN */
    break;
#endif /* PASTIX_WITH_MAGMABLAS */
  default:
    errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
    assert(0);
    break;
  }
}

void xxtrf_starpu_cpu(void * buffers[], void * _args) {
  xxtrf_starpu_common(buffers, _args, ARCH_CPU);
}
void xxtrf_starpu_cuda(void * buffers[], void * _args) {
  xxtrf_starpu_common(buffers, _args, ARCH_CUDA);
}
/*
 * Function: trsm_starpu_common
 *
 * Extra diagonal block system solve.
 *
 * Parameters:
 *   buffers    - Data handlers :
 *     0            - L column block
 *     1            - U column block
 *   _args      - Codelet arguments:
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 */
static inline
void trsm_starpu_common(void * buffers[], void * _args, int arch)
{
  pastix_float_t      * lDiag        = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
  pastix_int_t          stride       = STARPU_MATRIX_GET_LD(buffers[0]);
  pastix_float_t      * lExtraDiag   = NULL;
  int                 me           = starpu_worker_get_id();
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  pastix_float_t      * uDiag        = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
  pastix_float_t      * uExtraDiag   = NULL;
#endif
#ifndef CHOL_SOPALIN
  pastix_float_t      * tmp4         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[1]);
#else
  pastix_float_t      * tmp4         = NULL;
#endif
#ifdef PASTIX_WITH_CUDA /* see pastix_cuda_helper in old branch */
  CU_FLOAT            one_cuf      = CU_FLOAT_INIT(1.0, 0.0);
#endif
  DECLARE_ARGS_TRSM;
  UNPACK_ARGS_TRSM(_args, sopalin_data, cblknum, tasknum, datacode,
                   fblknum, lblknum, dima, dimb);

  switch(arch) {
  case ARCH_CPU:
      if ( fblknum+1 < lblknum )
      {
          lExtraDiag = lDiag + SOLV_COEFIND(fblknum+1);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
          uExtraDiag = uDiag + SOLV_COEFIND(fblknum+1);
#endif
          kernel_trsm(dimb, dima,
                      lDiag,
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
                      uDiag,
#endif
                      stride,
                      lExtraDiag,
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
                      uExtraDiag,
#endif
#ifndef CHOL_SOPALIN
                      tmp4,
#endif
                      stride);
      }
      break;
#ifdef PASTIX_WITH_CUDA
  case ARCH_CUDA:
      /* if there is an extra-diagonal bloc in column block */
      if ( fblknum+1 < lblknum ) {

          lExtraDiag = lDiag + SOLV_COEFIND(fblknum+1);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
          uExtraDiag = uDiag + SOLV_COEFIND(fblknum+1);
#endif
          CUDA_TRSM('R',
#  ifdef CHOL_SOPALIN
#    ifdef SOPALIN_LU
                    'U', 'N',
#    else
                    'L', 'T',
#    endif
#  else
#    ifdef HERMITIAN
                    'L', 'C',
#    else
                    'L', 'T',
#    endif
#  endif
                    'N', dimb, dima,
                    one_cuf, lDiag, stride, lExtraDiag, stride);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
          CUDA_TRSM('R', 'U', 'N', 'U', dimb, dima,
                    one_cuf, uDiag, stride, uExtraDiag, stride);
#endif
      }
      break;
#endif /* PASTIX_WITH_CUDA */
  default:
      errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
      assert(0);
      break;
  }
  if (pastix_starpu_with_nested_task() == API_YES) {
      starpu_submit_bunch_of_gemm(tasknum, sopalin_data);
  }
    {
        char name[256];
        sprintf(name, "cblk_%d_after_trf_trsm", cblk->gcblknum);
        cblk_save(cblk, name, lDiag);
    }
}

void trsm_starpu_cpu(void * buffers[], void * _args) {
  trsm_starpu_common(buffers, _args, ARCH_CPU);
}
void trsm_starpu_cuda(void * buffers[], void * _args) {
  trsm_starpu_common(buffers, _args, ARCH_CUDA);
}
/*
 * Function: trfsp1d_starpu_common
 *
 * Diagonal block factorization and column block update for LU decomposition.
 *
 * Parameters:
 *   buffers    - Data handlers :
 *     0            - L column block
 *     1            - U column block
 *   _args      - Codelet arguments:
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 */
static inline
void trfsp1d_starpu_common(void * buffers[], void * _args, int arch)
{
  xxtrf_starpu_common(buffers, _args, arch);
  trsm_starpu_common(buffers, _args, arch);
}



/*
 * Function: trfsp1d_starpu_cpu
 *
 * Diagonal block factorization and column block update for
 * LU/LLT/LDLT/LDLH decomposition.
 *
 * CPU function.
 *
 * Parameters:
 *   buffers    - Data handlers :
 *     0            - L column block
 *     1            - U column block
 *   _args      - Codelet arguments:
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 */
void trfsp1d_starpu_cpu(void * buffers[], void * _args)
{
  trfsp1d_starpu_common(buffers, _args, ARCH_CPU);
}

#ifdef PASTIX_WITH_MAGMABLAS
/*
 * Function: trfsp1d_starpu_cuda
 *
 * Diagonal block factorization and column block update for LU decomposition.
 *
 * CUDA function.
 *
 * Parameters:
 *   buffers    - Data handlers :
 *     0            - L column block
 *     1            - U column block
 *   _args      - Codelet arguments:
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 */
void trfsp1d_starpu_cuda(void * buffers[], void * _args)
{
  trfsp1d_starpu_common(buffers, _args, ARCH_CUDA);
}
#endif /* PASTIX_WITH_MAGMABLAS */

/*
  Function: trfsp1d_gemm_starpu_common

  General update of left block column facing current block.

  Common function for CPU and GPU.

  Update block by block.

  Parameters:
    buffers - Data handlers :
      0 - L column block.
      1 - L facing column block.
      2 - U column block.
      3 - U facing column block.
      4 - Working memory area.
    _args   - codelet arguments :
      sopalin_data - global PaStiX internal data.
      cblknum      - Current column block index.
      bloknum      - Current block index.
      fcblknum     - Facing column block index.
   arch     - indicate if the codelet is runned on CPU or CUDA node.
*/
static inline void
trfsp1d_gemm_starpu_common(void * buffers[], void * _args, int arch)
{
  pastix_float_t       * L            = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
  pastix_float_t       * Cl           = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  pastix_float_t       * U            = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[2]);
  pastix_float_t       * Cu           = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[3]);
  pastix_float_t       * work         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[4]);
  pastix_int_t           ldw          = (pastix_int_t)STARPU_VECTOR_GET_NX(buffers[4]);
  int                * all_blocktab = (int*)STARPU_VECTOR_GET_PTR(buffers[5]);;
#else
  pastix_float_t       * work         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[2]);
  pastix_int_t           ldw          = (pastix_int_t)STARPU_VECTOR_GET_NX(buffers[2])/2;
  pastix_float_t       * work2        = work + ldw;
  int                * all_blocktab = (int*)STARPU_VECTOR_GET_PTR(buffers[3]);;
#endif
#ifndef CHOL_SOPALIN
#endif
  pastix_int_t           stridefc     = STARPU_MATRIX_GET_LD(buffers[1]);
  pastix_float_t       * Aik;
#ifdef CHOL_SOPALIN
  pastix_float_t       * Akj;
#endif
  pastix_float_t       * Aij;
  pastix_int_t           lblknum, frownum;
  pastix_int_t           b, j, dimb;
  pastix_float_t       * wtmp = work;
#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
  char * trans = "T";
#  else  /* SOPALIN_LU */
  char * trans = "C";
#  endif
#else
#  ifdef HERMITIAN
  char * trans = "C";
#  else
  char * trans = "T";
#  endif
#endif

  DECLARE_ARGS_GEMM;
  UNPACK_ARGS_GEMM(_args, sopalin_data, cblknum, bloknum,
                   all_blocktab, tasknum, datacode,
                   fcblknum, indblok, blocktab, fblocktab,
		   stride, dimi, dimj, dima, blocknbr, fblocknbr);

  lblknum  = bloknum + blocknbr;
  /* Matrix A = Aik */
  Aik = L + indblok;
#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
  Akj = U + indblok;
#  else
  Akj = Aik;
#  endif
#endif

  switch(arch) {
  case ARCH_CPU:
#ifdef CHOL_SOPALIN
      SOPALIN_GEMM( "N", trans,
                    dimi, dimj, dima,
                    1.,  Aik,  stride,
                    Akj,  stride,
                    0.,  wtmp, dimi);
#else
  /* Compute the contribution */
      CORE_gemdm( PastixNoTrans,
#  ifdef HERMITIAN
                  PastixConjTrans,
#  else
                  PastixTrans,
#  endif
                  dimi, dimj, dima,
                  1.,  Aik,   stride,
                  Aik,   stride,
                  0.,  wtmp, dimi,
                  L,     stride+1,
                  work2, ldw );
#endif
      /*
       * Add contribution to facing cblk
       */
      if (cblk_islocal(datacode, fcblk)) {
          b = SYMB_BLOKNUM( fcblknum );
      } else {
          b = fcblk->fblokptr - datacode->fbloktab[SOLV_PROCNUM];
      }
      Cl = Cl + ( blok->frownum - fcblk->fcolnum ) * fcblk->stride;
      /* if (cblknum < 0) { */
      /*     Cl = Cl + (HBLOCK_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)) * stridefc; */
      /* } else { */
      /*     Cl = Cl + (SYMB_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)) * stridefc; */
      /* } */
      /* for all following blocks in block column */
      for (j=bloknum; j<lblknum; j++) {
          if (cblk_ishalo(datacode, cblk)) {
              frownum = HBLOCK_FROWNUM(j);
              dimb = HBLOCK_ROWNBR(j);
              /* Find facing bloknum */
              while (!HBLOCK_ISFACING(j,b)) {
                  b++;
                  assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
              }
          } else {
              if (cblk_islocal(datacode, cblk)) {
                  frownum = SYMB_FROWNUM(j);
                  dimb = BLOK_ROWNBR(j);
                  /* Find facing bloknum */
                  if (cblk_islocal(datacode, fcblk)) {
                      while (!is_block_inside_fblock(&(blok[j-bloknum]),
                                                     &(datacode->bloktab[b]))) {
                          b++;
                          assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
                      }
                  } else {
                      while (!is_block_inside_fblock(&(blok[j-bloknum]),
                                                     &(datacode->fbloktab[SOLV_PROCNUM][b]))) {
                          b++;
                          assert( b < fcblk[1].fblokptr - datacode->fbloktab );
                      }

                  }
              } else {
                  frownum = blok[j-bloknum].frownum;
                  dimb    = blok[j-bloknum].lrownum - frownum + 1;
                  /* Find facing bloknum */
                  while (!is_block_inside_fblock(&(blok[j-bloknum]),
                                                 &(datacode->bloktab[b]))) {
                      b++;
                      assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
                  }
              }
          }
          if (cblk_islocal(datacode, fcblk)) {
              Aij = Cl + SOLV_COEFIND(b) + frownum - SYMB_FROWNUM(b);
          } else {
              Aij = Cl + datacode->fbloktab[SOLV_PROCNUM][b].coefind + frownum -
                  datacode->fbloktab[SOLV_PROCNUM][b].frownum;
          }
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
          SOPALIN_GEAM("N", "N", dimb, dimj, -1.0,
                       wtmp, dimi,
                       Aij,  stridefc );
#else
          MAT_zaxpy( dimb, dimj, -1.0,
                     wtmp, dimi,
                     Aij,  stridefc );
#endif
          /* Displacement to next block */
          wtmp += dimb;
      }

#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
      /*
       * Compute update on U
       */

      Aik = U + indblok;
      Akj = L + indblok;
      wtmp = work;
      SOPALIN_GEMM( "N", "T",
                    dimi, dimj, dima,
                    1.,  Aik,  stride,
                    Akj,  stride,
                    0.,  wtmp, dimi  );
      wtmp += SYMB_LROWNUM(bloknum) - SYMB_FROWNUM(bloknum) + 1;

      /*
       * Add contribution to facing cblk
       */
      if (cblk_islocal(datacode, fcblk)) {
          b = SYMB_BLOKNUM( fcblknum );
      } else {
          b = fcblk->fblokptr - datacode->fbloktab[SOLV_PROCNUM];
      }
      Cl = Cl + ( blok->frownum - fcblk->fcolnum ) * fcblk->stride;
      /* if (cblknum < 0) { */
      /*   Cl = Cl + (HBLOCK_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)); */
      /* } else { */
      /*   Cl = Cl + (SYMB_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)); */
      /* } */
      /* for all following blocks in block column */
      for (j=bloknum+1; j<lblknum; j++) {
      if (cblk_ishalo(datacode, cblk)) {
          frownum = HBLOCK_FROWNUM(j);
          dimb = HBLOCK_ROWNBR(j);

          /* Find facing bloknum */
          /* WARNING: may not work for NAPA */
          if (!HBLOCK_ISFACING(j,b))
              break;
      } else {
          if (cblk_islocal(datacode, cblk)) {
              frownum = SYMB_FROWNUM(j);
              dimb = BLOK_ROWNBR(j);

              /* Find facing bloknum */
              /* WARNING: may not work for NAPA */
              if (!BLOCK_ISFACING(j,b))
                  break;
          } else {
              frownum = blok[j-bloknum].frownum;
              dimb    = blok[j-bloknum].lrownum - frownum + 1;
              /* Find facing bloknum */
              while (!is_block_inside_fblock(&(blok[j-bloknum]),
                                             &(datacode->bloktab[b]))) {
                  b++;
                  assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
              }
          }

    }


    Aij = Cl + (frownum - SYMB_FROWNUM(b))*stridefc;

    switch(arch) {
    case ARCH_CPU:
      SOPALIN_GEAM( "T", "N", dimj, dimb, -1.0,
                    wtmp, dimi,
                    Aij,  stridefc );
      break;
    default:
      errorPrint("Unknown Architecture");
      assert(0);
      break;
    }

    /* Displacement to next block */
    wtmp += dimb;
  }

  Cu = Cu + ( blok->frownum - fcblk->fcolnum ) * fcblk->stride;
  /* if (cblknum < 0) { */
  /*   Cu = Cu + (HBLOCK_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)) * stridefc; */
  /* } else { */
  /*   Cu = Cu + (SYMB_FROWNUM(bloknum) - SYMB_FCOLNUM(fcblknum)) * stridefc; */
  /* } */
  /* Keep updating on U */
  for (; j<lblknum; j++) {
      if (cblk_ishalo(datacode, cblk)) {
          frownum = HBLOCK_FROWNUM(j);
          dimb = HBLOCK_ROWNBR(j);

          /* Find facing bloknum */
          while (!HBLOCK_ISFACING(j,b)) {
              b++;
              assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
          }
      } else {
          if (cblk_islocal(datacode, cblk)) {
              frownum = SYMB_FROWNUM(j);
              dimb = SYMB_LROWNUM(j) - frownum + 1;
              /* Find facing bloknum */
              while (!BLOCK_ISFACING(j,b)) {
                  b++;
                  assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
              }
          } else {
              frownum = blok[j-bloknum].frownum;
              dimb    = blok[j-bloknum].lrownum - frownum + 1;
              /* Find facing bloknum */
              while (!is_block_inside_fblock(&(blok[j-bloknum]),
                                             &(datacode->bloktab[b]))) {
                  b++;
                  assert( b < SYMB_BLOKNUM( fcblknum+1 ) );
              }
          }
    }

    Aij = Cu + SOLV_COEFIND(b) + frownum - SYMB_FROWNUM(b);
    SOPALIN_GEAM("N", "N", dimb, dimj, -1.0,
                 wtmp, dimi,
                 Aij,  stridefc );



    /* Displacement to next block */
    wtmp += dimb;
  }
#endif
      break;
  default:
      errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
      assert(0);
      break;
  }
  SUBMIT_TRF_IF_NEEDED;
    {
        char name[256];
        sprintf(name, "cblk_%d_after_gemm_%d_%d_%d_on_%d", fcblk->gcblknum,
                cblk->gcblknum, blok - cblk->fblokptr, fcblk->gcblknum,
                sopalin_data->datacode->clustnum);
        cblk_save(fcblk, name, Cl);
    }
}

/*
  Function: trfsp1d_gemm_starpu_cpu

  General update of left block column facing current block.

  Update block by block.

  Parameters:
    buffers - Data handlers :
      0 - L column block.
      1 - L facing column block.
      2 - U column block.
      3 - U facing column block.
      4 - Working memory area.
    _args   - codelet arguments :
      sopalin_data - global PaStiX internal data.
      cblknum      - Current column block index.
      bloknum      - Current block index.
      fcblknum     - Facing column block index.
*/
void trfsp1d_gemm_starpu_cpu(void * buffers[], void * _args)
{
  trfsp1d_gemm_starpu_common(buffers, _args, ARCH_CPU);
}

#ifdef STARPU_USE_CUDA
/*
  Function: trfsp1d_gemm_starpu_cuda

  General update of left block column facing current block.

  Update block by block.

  Parameters:
    buffers - Data handlers :
      0 - L column block.
      1 - L facing column block.
      2 - U column block.
      3 - U facing column block.
      4 - Working memory area.
    _args   - codelet arguments :
      sopalin_data - global PaStiX internal data.
      cblknum      - Current column block index.
      bloknum      - Current block index.
      fcblknum     - Facing column block index.
*/
void trfsp1d_gemm_starpu_cuda(void * buffers[], void * _args)
{
  trfsp1d_gemm_starpu_common(buffers, _args, ARCH_CUDA);
}
#endif


/*
 *  Function: trfsp1d_sparse_gemm_starpu_common
 * 
 *  Sparse update of left block column facing current block.
 * 
 *  Common function for CPU and GPU.
 * 
 *  Update all the facing column block at once.
 * 
 *  TODO: Implement the CPU version
 * 
 *  Parameters:
 *    buffers - Data handlers :
 *      0 - L column block.
 *      1 - L facing column block.
 *      2 - U column block.
 *      3 - U facing column block.
 *      4 - Working memory area.
 *      5 - blocktab
 *    _args   - codelet arguments :
 *      sopalin_data - global PaStiX internal data.
 *      cblknum      - Current column block index.
 *      bloknum      - Current block index.
 *      nblocs       - Number of blocks in current column block.
 *    arch     - indicate if the codelet is runned on CPU or CUDA node.
 */
static inline
void trfsp1d_sparse_gemm_starpu_common(void * buffers[], void * _args,
                                         int arch)
{
  pastix_float_t       * L            = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
  pastix_float_t       * Cl           = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  pastix_float_t       * U            = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[2]);
  pastix_float_t       * Cu           = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[3]);
  pastix_float_t       * work         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[4]);
  size_t               worksize     = STARPU_VECTOR_GET_NX(buffers[4]);
  int                * all_blocktab = (int*)STARPU_VECTOR_GET_PTR(buffers[5]);
#else
  pastix_float_t       * work         = (pastix_float_t*)STARPU_VECTOR_GET_PTR(buffers[2]);
  size_t               worksize     = STARPU_VECTOR_GET_NX(buffers[2]);
  int                * all_blocktab = (int*)STARPU_VECTOR_GET_PTR(buffers[3]);
#endif
  pastix_int_t           stridefc     = STARPU_MATRIX_GET_LD(buffers[1]);
  pastix_float_t         alpha        = -1.;
  pastix_float_t         beta         = 1.;
  pastix_float_t       * Aik;
  pastix_float_t       * Akj;
  pastix_float_t       * Aij;
#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
  char * TRANS = "T";
  char * trans = "t";
#  else  SOPALIN_LU
  char * TRANS = "C";
  char * trans = "c";
#  endif
#else
#  ifdef HERMITIAN
  char * TRANS = "C";
  char * trans = "c";
#  else
  char * TRANS = "T";
  char * trans = "t";
#  endif
#endif

  DECLARE_ARGS_GEMM;
  UNPACK_ARGS_GEMM(_args, sopalin_data, cblknum, bloknum, all_blocktab,
                   tasknum, datacode, fcblknum, indblok, blocktab, fblocktab,
                   stride, dimi, dimj, dima, blocknbr, fblocknbr);

  Aik = L + indblok;
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  Akj = U + indblok;
#else
  Akj = Aik;
#endif
  if ( cblknum < 0 ) {
    Aij = Cl + ( HBLOCK_FROWNUM(bloknum) -
		 SYMB_FCOLNUM(fcblknum) )*SOLV_STRIDE(fcblknum);
  } else {
    Aij = Cl + ( SYMB_FROWNUM(bloknum) -
		 SYMB_FCOLNUM(fcblknum) )*SOLV_STRIDE(fcblknum);
  }
  switch(arch) {
  case ARCH_CPU:
    SOPALIN_SPARSE_GEMM( "N", TRANS,
                         dimi, dimj, dima,
                         alpha,  Aik,  stride,
                         Akj,  stride,
                         beta,  Aij, stridefc,
                         blocknbr, blocktab, fblocknbr, fblocktab,
                         work, worksize);
    break;
#ifdef PASTIX_WITH_CUDA
  case ARCH_CUDA:
#  ifdef CHOL_SOPALIN
    CUDA_SPARSE_GEMM( "n", trans,
                      dimi, dimj, dima,
                      alpha,  Aik,  stride,
                      Akj,  stride,
                      beta,  Aij, stridefc,
                      blocknbr, blocktab, fblocknbr, fblocktab);
#  else
    CUDA_SPARSE_GEMDM( "n", trans,
    		       dimi, dimj, dima,
    		       alpha,
    		       Aik,  stride,
    		       L,    stride+1,
    		       Akj,  stride,
    		       beta,  Aij, stridefc,
    		       blocknbr, blocktab, fblocknbr, fblocktab);
#  endif
    break;
#endif
  default:
    errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
    assert(0);
    break;
  }

#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  /*
   * Compute update on U
   */
  if ( blocknbr > 1 ) {
    if (cblknum < 0) {
      dimi = dimi - (HBLOCK_COEFIND(bloknum+1) - HBLOCK_COEFIND(bloknum));
      Aik = U + HBLOCK_COEFIND(bloknum+1);
      Aij = Cu + (HBLOCK_FROWNUM(bloknum) -
		  SYMB_FCOLNUM(fcblknum))*SOLV_STRIDE(fcblknum);
    } else {
      dimi = dimi - (SOLV_COEFIND(bloknum+1) - SOLV_COEFIND(bloknum));
      Aik = U + SOLV_COEFIND(bloknum+1);
      Aij = Cu + (SYMB_FROWNUM(bloknum) -
		  SYMB_FCOLNUM(fcblknum))*SOLV_STRIDE(fcblknum);
    }
    Akj = L + indblok;
    switch(arch) {
    case ARCH_CPU:
      SOPALIN_SPARSE_GEMM( "N", "T",
			   dimi, dimj, dima,
			   alpha,  Aik,  stride,
			   Akj,  stride,
			   beta,  Aij, stridefc,
			   blocknbr-1, &(blocktab[2]),
			   fblocknbr, fblocktab,
			   work, worksize);
      break;
#if defined(PASTIX_WITH_CUDA)
    case ARCH_CUDA:
      CUDA_SPARSE_GEMM( "n", "t",
			dimi, dimj, dima,
			alpha,  Aik,  stride,
			Akj,  stride,
			beta,  Aij, stridefc,
			blocknbr-1, &(blocktab[2]),
			fblocknbr, fblocktab);
      break;
#endif /* defined(PASTIX_WITH_CUDA) */
    default:
      errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
      assert(0);
      break;
    }
  }
#endif /* CHOL_SOPALIN && SOPALIN_LU */
  SUBMIT_TRF_IF_NEEDED;
}


/*
 *  Function: trfsp1d_sparse_gemm_starpu_cpu
 * 
 *  Sparse update of left block column facing current block.
 * 
 *  Update all the facing column block at once.
 * 
 *  Parameters:
 *    buffers - Data handlers :
 *      0 - L column block.
 *      1 - L facing column block.
 *      2 - U column block.
 *      3 - U facing column block.
 *      4 - Working memory area.
 *      5 - blocktab.
 *    _args   - codelet arguments
 *      sopalin_data - global PaStiX internal data.
 *      bloknum      - Current block index.
 *      nblocs       - Number of blocks in current column block.
*/
void trfsp1d_sparse_gemm_starpu_cpu(void * buffers[], void * _args)
{
  trfsp1d_gemm_starpu_common(buffers, _args, ARCH_CPU);
}

/*
 * Function: trfsp1d_sparse_gemm_starpu_cuda
 *
 * Sparse update of left block column facing current block.
 *
 * Update all the facing column block at once.
 *
 * Parameters:
 *   buffers - Data handlers :
 *     0 - L column block.
 *     1 - L facing column block.
 *     2 - U column block.
 *     3 - U facing column block.
 *     4 - Working memory area.
 *     5 - blocktab.
 *   _args   - codelet arguments :
 *     sopalin_data - global PaStiX internal data.
 *     cblknum      - Current column block index.
 *     bloknum      - Current block index.
 *     nblocs       - Number of blocks in current column block.
 */
void trfsp1d_sparse_gemm_starpu_cuda(void * buffers[], void * _args)
{
  trfsp1d_sparse_gemm_starpu_common(buffers, _args, ARCH_CUDA);
}


static inline
void init_coeftab_common(void * buffers[], void * _args, int arch) {
  pastix_float_t       * L          = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
  pastix_int_t           stride     = STARPU_MATRIX_GET_LD(buffers[0]);
  pastix_int_t           nx         = STARPU_MATRIX_GET_NX(buffers[0]);

  switch(arch) {
  case ARCH_CPU:
    memset(L, 0, stride*nx*sizeof(pastix_float_t));
    break;
#if defined(PASTIX_WITH_CUDA)
  case ARCH_CUDA:
    cudaMemset(L, 0, stride*nx*sizeof(pastix_float_t));
    break;
#endif /* defined(PASTIX_WITH_CUDA) */
  default:
    errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
    assert(0);
    break;
  }

}

void init_coeftab_cpu(void * buffers[], void * _args) {
  init_coeftab_common(buffers, _args, ARCH_CPU);
}

void init_coeftab_cuda(void * buffers[], void * _args) {
  init_coeftab_common(buffers, _args, ARCH_CUDA);
}


static inline
void fill_coeftab_common(void * buffers[], void * _args, int arch) {
  pastix_float_t       * L          = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
  pastix_float_t       * U          = (pastix_float_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
#endif
  pastix_int_t           stride     = STARPU_MATRIX_GET_LD(buffers[0]);
  pastix_int_t           nx         = STARPU_MATRIX_GET_NX(buffers[0]);
  Sopalin_Data_t    * sopalin_data;
  SolverMatrix      * datacode;
  pastix_int_t          cblknum;
  pastix_int_t          tasknum;
  pastix_int_t          fblknum;
  pastix_int_t          lblknum;
  pastix_int_t          dima;
  pastix_int_t          dimb;
  const CscMatrix   * cscmtx;
  pastix_float_t      * trandcsc;

  starpu_codelet_unpack_args(_args, &sopalin_data, &cblknum,
                             &tasknum);
  datacode = sopalin_data->datacode;
  fblknum  = SYMB_BLOKNUM(cblknum);
  lblknum  = SYMB_BLOKNUM(cblknum+1);
  dima     = CBLK_COLNBR(cblknum);
  dimb     = stride - dima;
  cscmtx   = sopalin_data->sopar->cscmtx;
  trandcsc = sopalin_data->sopar->transcsc;
  switch(arch) {
  case ARCH_CPU:
    if (cblknum < CSC_FNBR(cscmtx)) {
      pastix_int_t itercoltab;
      for (itercoltab=0;
           itercoltab < CSC_COLNBR(cscmtx,cblknum);
           itercoltab++) {
        pastix_int_t iterval;
        for (iterval = CSC_COL(cscmtx,cblknum,itercoltab);
             iterval < CSC_COL(cscmtx,cblknum,itercoltab+1);
             iterval++) {
          if (CSC_ROW(cscmtx,iterval) >=
              SYMB_FCOLNUM(cblknum)) {
            pastix_int_t iterbloc = SYMB_BLOKNUM(cblknum);

            ASSERTDBG(iterbloc < SYMB_BLOKNBR, MOD_SOPALIN);
            while (( iterbloc < SYMB_BLOKNUM(cblknum+1)) &&
                   (( SYMB_LROWNUM(iterbloc) < CSC_ROW(cscmtx,iterval)) ||
                    ( SYMB_FROWNUM(iterbloc) > CSC_ROW(cscmtx,iterval)))) {
              iterbloc++;
            }

            if ( iterbloc < SYMB_BLOKNUM(cblknum+1) ) {
              pastix_int_t coefindx;
              coefindx = SOLV_COEFIND(iterbloc);

              coefindx += CSC_ROW(cscmtx,iterval) - SYMB_FROWNUM(iterbloc);

              coefindx += SOLV_STRIDE(cblknum)*itercoltab;
              L[coefindx] += CSC_VAL(cscmtx,iterval);
#if (defined CHOL_SOPALIN && defined SOPALIN_LU)
              if (trandcsc != NULL && iterbloc != SYMB_BLOKNUM(cblknum)) {
                if (cscmtx->type == 'H')
                  U[coefindx] += CONJ_FLOAT(trandcsc[iterval]);
                else
                  U[coefindx] += trandcsc[iterval];
              }
#endif
            } else {
              printf("ILU: csc2solv drop coeff from CSC c=%ld(%ld) l=%ld(%ld)"
                     " cblk=%ld fcol=%ld lcol=%ld\n",
                     (long)datacode->cblktab[cblknum].fcolnum+
                     (long)itercoltab,(long)itercoltab,
                     (long)CSC_ROW(cscmtx,iterval),(long)iterval,
                     (long)cblknum,
                     (long)datacode->cblktab[cblknum].fcolnum,
                     (long)datacode->cblktab[cblknum].lcolnum);
            }
          }
        }
      }
    }
    break;
  case ARCH_CUDA:
  default:
    errorPrint("%s:%d Unknown Architecture", __FILE__, __LINE__);
    assert(0);
    break;
  }
}

void fill_coeftab_cpu(void * buffers[], void * _args) {
  fill_coeftab_common(buffers, _args, ARCH_CPU);
}

void fill_coeftab_cuda(void * buffers[], void * _args) {
  fill_coeftab_common(buffers, _args, ARCH_CUDA);
}
