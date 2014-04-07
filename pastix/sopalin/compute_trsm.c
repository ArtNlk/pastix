/*
  File: compute_trsm.c

  Computation functions.

  Pierre Ramet    : fev 2003
  Mathieu Faverge
  Xavier Lacoste

*/

#include "compute_trsm.h"


/*********************************************************************
 *
 * Trsm executed in 1D version, or in 1D+esp:
 *   all the blocks are grouped together to do only one TRSM
 *
 *   sopalin_data - pointer on data associated to the factorization
 *   me           - Thread id
 *   c            - Block column id
 *
 */
void API_CALL(factor_trsm1d)(Sopalin_Data_t *sopalin_data, pastix_int_t me, pastix_int_t c)
{
  SolverMatrix  *datacode    = sopalin_data->datacode;
  pastix_float_t *dL, *L;
#ifdef SOPALIN_LU
  pastix_float_t *dU, *U;
#endif
#ifndef CHOL_SOPALIN
  Thread_Data_t *thread_data = sopalin_data->thread_data[me];
  pastix_float_t *L2;
#endif
  pastix_int_t dima, dimb, stride, offsetD, offsetED;
  (void)me;

  offsetD  = SOLV_COEFIND(SYMB_BLOKNUM(c));
  offsetED = SOLV_COEFIND(SYMB_BLOKNUM(c)+1);

  /* diagonal column block address */
  dL = &(SOLV_COEFTAB(c)[offsetD]);

  /* first extra-diagonal bloc in column block address */
  L  = &(SOLV_COEFTAB(c)[offsetED]);

  stride = SOLV_STRIDE(c);

  /* horizontal dimension */
  dima = SYMB_LCOLNUM(c) - SYMB_FCOLNUM(c) + 1;
  /* vertical dimension */
  dimb = stride - dima;

#ifdef CHOL_SOPALIN
#  ifdef SOPALIN_LU
  dU = &(SOLV_UCOEFTAB(c)[offsetD ]);
  U  = &(SOLV_UCOEFTAB(c)[offsetED]);
  API_CALL(kernel_trsm)(dimb, dima, dL, dU, stride, L,  U, stride);
#  else
  API_CALL(kernel_trsm)(dimb, dima, dL,     stride, L,     stride);
#  endif /* SOPALIN_LU */
#else
  L2 = thread_data->maxbloktab1;
  ASSERTDBG(SOLV_COEFMAX >= dimb*dima, MOD_SOPALIN);
  API_CALL(kernel_trsm)(dimb, dima, dL,     stride, L, L2, stride);
#endif /* CHOL_SOPALIN */
}
