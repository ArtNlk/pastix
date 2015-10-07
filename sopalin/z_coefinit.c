/**
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
/*
 * File: coefinit.c
 *
 * Allocation and initialisation of the coeficient of the z_solver matrix.
 *
 */

#include "common.h"
#ifndef FORCE_NOSMP
#  include <pthread.h>
#endif
#include "sopalin_define.h"
#include "dof.h"
#include "z_ftgt.h"
#include "symbol.h"
#include "z_csc.h"
#include "z_updown.h"
#include "queue.h"
#include "bulles.h"
#include "z_solver.h"
#include "sopalin_thread.h"
#include "stack.h"
#include "z_sopalin3d.h"
#include "order.h"
#include "z_debug_dump.h"
#include "z_csc_intern_solve.h"
#include "z_csc_intern_build.h"
#include "sopalin_time.h"
#include "z_ooc.h"
#include "sopalin_acces.h" /* ATTENTION : inclure apres define SMP_SOPALIN */
#include "z_coefinit.h"

/* Section: Functions */

/*
 * Function: z_CoefMatrix_Allocate
 *
 * Allocate matrix coefficients in coeftab and ucoeftab.
 *
 * Should be first called with me = -1 to allocated coeftab.
 * Then, should be called with me set to thread ID
 * to allocate column blocks coefficients arrays.
 *
 * Parameters
 *
 *    datacode  - solverMatrix
 *    factotype - factorization type (LU, LLT ou LDLT)
 *    me        - thread number. (-1 for first call,
 *                from main thread. >=0 to allocate column blocks
 *     assigned to each thread.)
 */
void z_CoefMatrix_Allocate(z_SopalinParam    *sopar,
                         z_SolverMatrix    *datacode,
                         pthread_mutex_t *mutex,
                         pastix_int_t              factotype,
                         pastix_int_t              me)
{
  pastix_int_t i;
#ifndef OOC
  pastix_int_t itercblk, coefnbr;
#endif
#ifdef PASTIX_WITH_STARPU
  /* For CUDA devices we have no allocation (yet?) */
  if ( sopar->iparm[IPARM_STARPU] == API_YES && me >= SOLV_THRDNBR)
    return;
#endif /* WITH_STARPU */
#ifndef OOC
  {
    /* On ne passe pas ici en OOC */
    pastix_int_t bubnum  = me;
    pastix_int_t task;

#  ifdef PASTIX_DYNSCHED
    while (bubnum != -1)
      {
        pastix_int_t fcandnum = datacode->btree->nodetab[bubnum].fcandnum;
        pastix_int_t lcandnum = datacode->btree->nodetab[bubnum].lcandnum;
        for (i=(me-fcandnum);i < datacode->ttsknbr[bubnum]; i+=(lcandnum-fcandnum+1))
#  else
    for (i=0; i < datacode->ttsknbr[bubnum]; i++)
#  endif /* PASTIX_DYNSCHED */

      {
        task = datacode->ttsktab[bubnum][i];
        itercblk = TASK_CBLKNUM(task);
        coefnbr  = SOLV_STRIDE(itercblk) * (SYMB_LCOLNUM(itercblk) -
                                            SYMB_FCOLNUM(itercblk) + 1);
        if ((TASK_TASKID(task) == COMP_1D)
            || (TASK_TASKID(task) == DIAG))
          {
            datacode->cblktab[itercblk].procdiag = me;
            if (SOLV_COEFTAB(itercblk) == NULL)
              { /* If not NULL it should be the schur */
                MALLOC_INTERN(SOLV_COEFTAB(itercblk), coefnbr, pastix_complex64_t);
              }
          }
        else if ( SOLV_COEFIND(TASK_BLOKNUM(task)) == 0 )
          {
            MUTEX_LOCK(mutex);
            datacode->cblktab[itercblk].procdiag = me;
            if (SOLV_COEFTAB(itercblk) == NULL)
              {
                MALLOC_INTERN(SOLV_COEFTAB(itercblk), coefnbr, pastix_complex64_t);
              }
            MUTEX_UNLOCK(mutex);
          }
      }

#  ifdef PASTIX_DYNSCHED
        bubnum = BFATHER(datacode->btree, bubnum);
      }
#  endif /* PASTIX_DYNSCHED */
  }
#endif /* OOC */

  /*
   * Allocate LU coefficient arrays
   * We also use it to store the diagonal in LDLt factorization using esp
   */
  if ( (factotype == API_FACT_LU) /* LU */
       || ( (factotype == API_FACT_LDLT) && sopar->iparm[IPARM_ESP] ) )
    {
#ifndef OOC
      {
        /* On ne passe pas ici en OOC */
        pastix_int_t bubnum  = me;
        pastix_int_t task;

#  ifdef PASTIX_DYNSCHED
        while (bubnum != -1)
          {
            pastix_int_t fcandnum = datacode->btree->nodetab[bubnum].fcandnum;
            pastix_int_t lcandnum = datacode->btree->nodetab[bubnum].lcandnum;
            for (i=(me-fcandnum); i < datacode->ttsknbr[bubnum]; i+=(lcandnum-fcandnum+1))
#  else
        for (i=0; i < datacode->ttsknbr[bubnum]; i++)
#  endif /* PASTIX_DYNSCHED */

          {
            task = datacode->ttsktab[bubnum][i];
            itercblk = TASK_CBLKNUM(task);
            if ( (me != datacode->cblktab[itercblk].procdiag)
                 || (SOLV_UCOEFTAB(itercblk) != NULL) )
              {
                continue;
              }

            if ( (factotype == API_FACT_LDLT) && sopar->iparm[IPARM_ESP] )
              {
                coefnbr  = SYMB_LCOLNUM(itercblk) - SYMB_FCOLNUM(itercblk) + 1;
              }
            else
              {
                coefnbr  = SOLV_STRIDE(itercblk) * (SYMB_LCOLNUM(itercblk) -
                                                    SYMB_FCOLNUM(itercblk) + 1);
              }

            MALLOC_INTERN(SOLV_UCOEFTAB(itercblk), coefnbr, pastix_complex64_t);
          }

#  ifdef PASTIX_DYNSCHED
            bubnum = BFATHER(datacode->btree, bubnum);
          }
#  endif /* PASTIX_DYNSCHED */
      }
#endif /* OOC */
    }
}

/*
 * Function: z_CoefMatrix_Init
 *
 * Init coeftab and ucoeftab coefficients.
 *
 * Parameters:
 *    datacode     - solverMatrix
 *    barrier      - Barrier used for thread synchronisation.
 *    me           - Thread ID
 *    iparm        - Integer parameters array.
 *    transcsc     - vecteur transcsc
 *    sopalin_data - <z_Sopalin_Data_t> structure.
 */
void z_CoefMatrix_Init(z_SolverMatrix         *datacode,
                     sopthread_barrier_t  *barrier,
                     pastix_int_t                   me,
                     pastix_int_t                  *iparm,
                     pastix_complex64_t               **transcsc,
                     z_Sopalin_Data_t       *sopalin_data)
{

  pastix_int_t j, itercblk;
  pastix_int_t i, coefnbr;

#ifdef PASTIX_WITH_STARPU
  /* For CUDA devices we have no allocation (yet?) */
  if ( iparm[IPARM_STARPU] == API_YES && me >= SOLV_THRDNBR)
    return;
#endif /* WITH_STARPU */

  /* Remplissage de la matrice */
  if (iparm[IPARM_FILL_MATRIX] == API_NO)
  {
    /* Remplissage par bloc */
    pastix_int_t bubnum  = me;
#ifdef PASTIX_DYNSCHED
    while (bubnum != -1)
    {
      pastix_int_t fcandnum = datacode->btree->nodetab[bubnum].fcandnum;
      pastix_int_t lcandnum = datacode->btree->nodetab[bubnum].lcandnum;
      for (i=(me-fcandnum);i < datacode->ttsknbr[bubnum]; i+=(lcandnum-fcandnum+1))
#else
        for (i=0; i < datacode->ttsknbr[bubnum]; i++)
#endif /* PASTIX_DYNSCHED */

        {
          pastix_int_t task;
          pastix_int_t k = i;
#ifdef OOC
          /* En OOC, on inverse la boucle pour conserver les premiers blocs en m�moire */
          k = datacode->ttsknbr[bubnum]-i-1;
#endif
          task = datacode->ttsktab[bubnum][k];
          itercblk = TASK_CBLKNUM(task);

          if ( me != datacode->cblktab[itercblk].procdiag )
            continue;

          coefnbr  = SOLV_STRIDE(itercblk) * (SYMB_LCOLNUM(itercblk) - SYMB_FCOLNUM(itercblk) + 1);

          z_ooc_wait_for_cblk(sopalin_data, itercblk, me);

          /* initialisation du bloc colonne */
          for (j=0 ; j < coefnbr ; j++)
          {
            SOLV_COEFTAB(itercblk)[j] = ZERO;
            if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
              SOLV_UCOEFTAB(itercblk)[j] = ZERO;
          }

          /* remplissage */
          z_Csc2solv_cblk(sopalin_data->sopar->cscmtx, datacode, *transcsc, itercblk);

          z_ooc_save_coef(sopalin_data, task, itercblk, me);
        }

#ifdef PASTIX_DYNSCHED
      bubnum = BFATHER(datacode->btree, bubnum);
    }
#endif /* PASTIX_DYNSCHED */

#ifdef DEBUG_COEFINIT
    if (me == 0)
    {
      FILE *transfile;
      char transfilename[10];
      sprintf(transfilename, "trans%ld.%ld",(long) me,(long) SOLV_PROCNUM);
      transfile = fopen(transfilename, "w");
      z_dump7(*transcsc, transfile);
    }
#endif
    /* Lib�ration de m�moire */
#ifdef STARPU_INIT_SMP
    if (sopalin_data->sopar->iparm[IPARM_STARPU] == API_NO)
#endif /* STARPU_INIT_SMP */
      SYNCHRO_X_THREAD(SOLV_THRDNBR, *barrier);
    if (me == 0)
    {
      if (iparm[IPARM_FACTORIZATION] != API_FACT_LU)
      {
        if (*transcsc != NULL)
          memFree_null(*transcsc);
      }
      else
      {
        if (iparm[IPARM_SYM] == API_SYM_YES || iparm[IPARM_SYM] == API_SYM_HER) /* Symmetric */
          *transcsc = NULL;
        else /* Unsymmetric */
          memFree_null(*transcsc);
      }
    }
  }
  else  /* fake factorisation */
  {

    /* deadcode */
    pastix_int_t itercol;

    /* Initialisation de la matrice � 0 et 1 ou 2 */
    pastix_int_t task;
    pastix_int_t bubnum  = me;

#ifdef PASTIX_DYNSCHED
    while (bubnum != -1)
    {
      pastix_int_t fcandnum = datacode->btree->nodetab[bubnum].fcandnum;
      pastix_int_t lcandnum = datacode->btree->nodetab[bubnum].lcandnum;
      for (i=(me-fcandnum);i < datacode->ttsknbr[bubnum]; i+=(lcandnum-fcandnum+1))
#else
        for (i=0; i < datacode->ttsknbr[bubnum]; i++)
#endif /* PASTIX_DYNSCHED */

        {
          task = datacode->ttsktab[bubnum][i];
          itercblk = TASK_CBLKNUM(task);
          if ( me != datacode->cblktab[itercblk].procdiag )
            continue;
          coefnbr  = SOLV_STRIDE(itercblk) * (SYMB_LCOLNUM(itercblk) - SYMB_FCOLNUM(itercblk) + 1);

          for (j=0 ; j < coefnbr ; j++)
          {
            if (iparm[IPARM_FILL_MATRIX] == API_NO)
              SOLV_COEFTAB(itercblk)[j] = ZERO;
            else
              SOLV_COEFTAB(itercblk)[j] = UN;
            if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
            {
              if (iparm[IPARM_FILL_MATRIX] == API_NO)
                SOLV_UCOEFTAB(itercblk)[j] = ZERO;
              else
                SOLV_UCOEFTAB(itercblk)[j] = DEUX;
            }
          }
#ifdef _UNUSED_
        }
#endif
    }

#ifdef PASTIX_DYNSCHED
    bubnum = BFATHER(datacode->btree, bubnum);
  }
#endif /* PASTIX_DYNSCHED */

  /* 2 eme phase de l'initialisation de la matrice */
  for (i=0; i < SOLV_TTSKNBR; i++)
  {
    itercblk = TASK_CBLKNUM(SOLV_TTSKTAB(i));
    coefnbr  = SOLV_STRIDE(itercblk) * (SYMB_LCOLNUM(itercblk) - SYMB_FCOLNUM(itercblk) + 1);

    z_ooc_wait_for_cblk(sopalin_data, itercblk, me);

    /* initialisation du bloc colonne */
    for (j=0 ; j < coefnbr ; j++)
    {
      SOLV_COEFTAB(itercblk)[j] = UN;
      if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
        SOLV_UCOEFTAB(itercblk)[j] = DEUX;
    }

    /* if we are on a diagonal bloc */
    if (SYMB_FCOLNUM(itercblk) == SYMB_FROWNUM(SYMB_BLOKNUM(itercblk)))
    {
      pastix_int_t index  = SOLV_COEFIND(SYMB_BLOKNUM(itercblk));
      pastix_int_t size   = SYMB_LCOLNUM(itercblk) - SYMB_FCOLNUM(itercblk) + 1;
      pastix_int_t stride = SOLV_STRIDE(itercblk);

      for (itercol=0; itercol<size; itercol++)
      {
        /* On s'assure que la matrice est diagonale dominante */
        SOLV_COEFTAB(itercblk)[index+itercol*stride+itercol] = (pastix_complex64_t) (UPDOWN_GNODENBR*UPDOWN_GNODENBR);
      }
      /* copie de la partie de block diag de U dans L */
      if (iparm[IPARM_FACTORIZATION] == API_FACT_LU)
      {
        pastix_int_t iterrow;
        for (itercol=0; itercol<size; itercol++)
        {
          for (iterrow=itercol+1; iterrow<size; iterrow++)
          {
            SOLV_COEFTAB(itercblk)[index+iterrow*stride+itercol] = SOLV_UCOEFTAB(itercblk)[index+itercol*stride+iterrow];
          }
        }
      }
    }

    z_ooc_save_coef(sopalin_data, SOLV_TTSKTAB(i), itercblk, me);
  }
#ifdef _UNUSED_
}
#endif
printf("fin false fill-in\n");
}


if (iparm[IPARM_FREE_CSCPASTIX] == API_CSC_FREE)
 {
   SYNCHRO_X_THREAD(SOLV_THRDNBR, *barrier);

   /* Internal csc is useless if we don't want to do refinement step */
   if (me == 0)
   {
     if ((iparm[IPARM_END_TASK] < API_TASK_SOLVE) ||
         ((iparm[IPARM_END_TASK] < API_TASK_REFINE) &&
          (iparm[IPARM_RHS_MAKING] == API_RHS_B)))
     {
       z_CscExit(sopalin_data->sopar->cscmtx);
     }
     else
     {
       errorPrintW("The internal CSC can't be freed if you want to use refinement or if you don't give one RHS.\n");
     }
   }
 }
}

/*
 Function: z_CoefMatrix_Free

 Free the z_solver matrix coefficient tabular : coeftab and ucoeftab.

 WARNING: Call it with one unnique thread.

 Parameters:
 datacode   - solverMatrix
 factotype  - factorisation type (<API_FACT>)

 */
void z_CoefMatrix_Free(z_SopalinParam *sopar,
                     z_SolverMatrix *datacode,
                     pastix_int_t           factotype)
{
  pastix_int_t i;

  if ( (factotype == API_FACT_LU)
       || ( (factotype == API_FACT_LDLT) && sopar->iparm[IPARM_ESP]) )
  {
    for (i=0 ; i < SYMB_CBLKNBR; i++)
      if (SOLV_UCOEFTAB(i) != NULL)
        memFree_null(SOLV_UCOEFTAB(i));
  }
  for (i=0 ; i < SYMB_CBLKNBR; i++)
    if (SOLV_COEFTAB(i) != NULL)
      memFree_null(SOLV_COEFTAB(i));
}
