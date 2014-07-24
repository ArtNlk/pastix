/*
 * File: raff_functions.c
 *
 * Functions computing operations for reffinement methods
 *
 */

#include "common.h"
#include <pthread.h>
#include "tools.h"
#ifdef PASTIX_EZTRACE
#  include "pastix_eztrace.h"
#else
#  include "trace.h"
#endif
#include "sopalin_define.h"
#include "symbol.h"
#include "d_csc.h"
#include "d_updown.h"
#include "queue.h"
#include "bulles.h"
#include "d_ftgt.h"
#include "d_solver.h"
#include "sopalin_thread.h"
#include "stack.h"
#include "sopalin3d.h"
#include "sopalin_init.h"
#include "perf.h"
#include "out.h"
#include "coefinit.h"
#include "ooc.h"
#include "order.h"
#include "debug_dump.h"
#include "sopalin_acces.h"
#include "d_csc_intern_compute.h"
#ifdef PASTIX_WITH_STARPU
#  include "starpu_submit_tasks.h"
#endif

static pastix_float_t fun   = 1.0;
#define up_down_smp API_CALL(up_down_smp)
void* up_down_smp ( void *arg );
#define sopalin_updo_comm API_CALL(sopalin_updo_comm)
void *sopalin_updo_comm ( void *arg );


#include "raff_functions.h"


/*** ALLOCATIONS ET SYNCHRONISATIONS ***/

/* Synchronise le vecteur x dans la nb-ieme variable de la structure */
pastix_float_t *Pastix_Synchro_Vect(void *arg, void *x, int nb)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  PASTIX_INT        me           = argument->me;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MONOTHREAD_BEGIN;
  sopalin_data->ptr_raff[nb] = x;
  MONOTHREAD_END;
  SYNCHRO_THREAD;
  return (pastix_float_t*) sopalin_data->ptr_raff[nb];
}

/* Alloue un vecteur de taille size octets */
void *Pastix_Malloc(void *arg, size_t size)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  PASTIX_INT        me           = argument->me;
  void *x = NULL;
  MONOTHREAD_BEGIN;
  MALLOC_INTERN(x, size, char);
  MONOTHREAD_END;
  return x;
}

/* Libere un vecteur */
void Pastix_Free(void *arg, void *x)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  PASTIX_INT        me           = argument->me;
  MONOTHREAD_BEGIN;
  memFree_null(x);
  MONOTHREAD_END;
}


/*** GESTION DE L'INTERFACE ***/

/* Affichage à chaque itération et communication de certaines informations à la structure */
void Pastix_Verbose(void *arg, double t0, double t3, double tmp, PASTIX_INT nb_iter)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  SopalinParam     *sopar        = sopalin_data->sopar;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  sopalin_data->count_iter = nb_iter;
  sopalin_data->stop = tmp;
  MONOTHREAD_BEGIN;
  if (sopar->iparm[IPARM_VERBOSE] > API_VERBOSE_NOT)
    {
      double rst = 0.0;
      double stt, rtt;
      double err, stop = tmp;

      stt = t3 - t0;
      MyMPI_Reduce(&stop, &err, 1, MPI_DOUBLE, MPI_MAX, 0, pastix_comm);
      MyMPI_Reduce(&stt,  &rtt, 1, MPI_DOUBLE, MPI_MAX, 0, pastix_comm);

      if (SOLV_PROCNUM == 0)
        {
          fprintf(stdout, OUT_ITERRAFF_ITER, (int)sopalin_data->count_iter);
          if (sopar->iparm[IPARM_ONLY_RAFF] == API_NO)
            fprintf(stdout, OUT_ITERRAFF_TTS, rst);
          fprintf(stdout, OUT_ITERRAFF_TTT, stt);
          fprintf(stdout, OUT_ITERRAFF_ERR, err);
        }
    }
  MONOTHREAD_END;
}

/* Affichage final */
void Pastix_End(void* arg, pastix_float_t tmp, PASTIX_INT nb_iter, double t, pastix_float_t *x)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  SopalinParam     *sopar        = sopalin_data->sopar;
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;

  sopalin_data->stop = tmp;
  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, x, UPDOWN_SM2XTAB,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  SYNCHRO_THREAD;

  sopar->rberror = tmp;
  sopar->itermax = nb_iter;

  if (sopar->iparm[IPARM_PRODUCE_STATS] == API_YES) {
    pastix_float_t *r, *s;

    MONOTHREAD_BEGIN;
    MALLOC_INTERN(r, UPDOWN_SM2XSZE, pastix_float_t);
    MALLOC_INTERN(s, UPDOWN_SM2XSZE, pastix_float_t);
    sopalin_data->ptr_raff[0] = (void *)r;
    sopalin_data->ptr_raff[1] = (void *)s;
    MONOTHREAD_END;
    SYNCHRO_THREAD;

    r = (pastix_float_t *)sopalin_data->ptr_raff[0];
    s = (pastix_float_t *)sopalin_data->ptr_raff[1];
    MULTITHREAD_BEGIN;
    /* compute r = b - Ax */
    d_CscbMAx(sopalin_data, me, r, sopar->b, sopar->cscmtx,
            &(datacode->updovct), datacode, PASTIX_COMM,
            sopar->iparm[IPARM_TRANSPOSE_SOLVE]);
    /* |A||x| + |b| */
    d_CscAxPb( sopalin_data, me, s, sopar->b, sopar->cscmtx,
             &(datacode->updovct), datacode, PASTIX_COMM,
             sopar->iparm[IPARM_TRANSPOSE_SOLVE]);
    d_CscBerr(sopalin_data, me, r, s, UPDOWN_SM2XSZE,
            1, &(sopalin_data->sopar->dparm[DPARM_SCALED_RESIDUAL]),
            PASTIX_COMM);
    MULTITHREAD_END(1);
  }

  MONOTHREAD_BEGIN;

  if (THREAD_COMM_ON)
    {
      if (sopar->iparm[IPARM_END_TASK] >= API_TASK_REFINE)
        {
          MUTEX_LOCK(&(sopalin_data->mutex_comm));
          sopalin_data->step_comm = COMMSTEP_END;
          print_debug(DBG_THCOMM, "%s:%d END\n", __FILE__, __LINE__);
          MUTEX_UNLOCK(&(sopalin_data->mutex_comm));
          pthread_cond_broadcast(&(sopalin_data->cond_comm));
        }
    }
#ifdef OOC
  ooc_stop_thread(sopalin_data);
#endif
  set_dparm(sopar->dparm, DPARM_RAFF_TIME, t);
  MONOTHREAD_END;
  SYNCHRO_THREAD;
}

/* Vecteur solution X */
void Pastix_X(void *arg, pastix_float_t *x)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  SopalinParam     *sopar        = sopalin_data->sopar;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  PASTIX_INT        i;

  if (sopar->iparm[IPARM_ONLY_RAFF] == API_NO)
    for (i=0;i<UPDOWN_SM2XSZE*UPDOWN_SM2XNBR;i++)
      UPDOWN_SM2XTAB[i]=0.0;
  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, UPDOWN_SM2XTAB, x,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(1);
  SYNCHRO_THREAD;
}

/* Taille d'un vecteur */
PASTIX_INT Pastix_n(void *arg)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  return UPDOWN_SM2XSZE;
}

/* Nombre de second membres */
PASTIX_INT Pastix_m(void *arg)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  return UPDOWN_SM2XNBR;
}

/* Second membre */
void Pastix_B(void *arg, pastix_float_t *b)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  SopalinParam     *sopar        = sopalin_data->sopar;
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, sopar->b, b,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  SYNCHRO_THREAD;
}

/* Epsilon */
pastix_float_t Pastix_Eps(void *arg)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  SopalinParam     *sopar        = sopalin_data->sopar;
  return sopar->epsilonraff;
}

/* Itermax */
PASTIX_INT Pastix_Itermax(void *arg)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  SopalinParam     *sopar        = sopalin_data->sopar;
  return sopar->itermax;
}


/* Itermax */
PASTIX_INT Pastix_Krylov_Space(void *arg)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  SopalinParam     *sopar        = sopalin_data->sopar;
  return sopar->gmresim;
}

/*** OPERATIONS DE BASE ***/
/* Multiplication pour plusieurs second membres */
void Pastix_Mult(void *arg, pastix_float_t *alpha, pastix_float_t *beta, pastix_float_t *zeta, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  PASTIX_INT        me           = argument->me;
  MONOTHREAD_BEGIN;
#ifdef MULT_SMX_RAFF
  {
    PASTIX_INT itersmx;
    for(itersmx=0; itersmx<UPDOWN_SM2XNBR;itersmx++)
      {
        zeta[itersmx]=alpha[itersmx]/beta[itersmx];
      }
  }
#else
  zeta[0]=alpha[0]*beta[0];
#endif
  MONOTHREAD_END;
  if (flag)
    SYNCHRO_THREAD;
}

/* Division pour plusieurs second membres */
void Pastix_Div(void *arg, pastix_float_t *alpha, pastix_float_t *beta, pastix_float_t *zeta, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  PASTIX_INT        me           = argument->me;
  MONOTHREAD_BEGIN;
#ifdef MULT_SMX_RAFF
  {
    PASTIX_INT itersmx;
    for(itersmx=0; itersmx<UPDOWN_SM2XNBR;itersmx++)
      {
        zeta[itersmx]=alpha[itersmx]/beta[itersmx];
      }
  }
#else
  zeta[0]=alpha[0]/beta[0];
#endif
  MONOTHREAD_END;
  if (flag)
    SYNCHRO_THREAD;
}

/* Calcul de la norme de frobenius */
pastix_float_t Pastix_Norm2(void* arg, pastix_float_t *x)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  double            normx;
  MULTITHREAD_BEGIN;
  normx = d_CscNormFro(sopalin_data, me, x,
                     UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(1);
  NOSMP_SYNC_COEF(normx);
  return normx;
}

/* Copie d'un vecteur */
void Pastix_Copy(void *arg, pastix_float_t *s, pastix_float_t *d, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, s, d,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);

  if (flag)
    SYNCHRO_THREAD;
}

/* Application du préconditionneur */
void Pastix_Precond(void *arg, pastix_float_t *s, pastix_float_t *d, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  SopalinParam     *sopar        = sopalin_data->sopar;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;

  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, s, UPDOWN_SM2XTAB,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(1);
  /* M-1 updo -> updo */
#ifdef PRECOND
  if (sopar->iparm[IPARM_ONLY_RAFF] == API_NO)
    {
      SYNCHRO_THREAD;
      API_CALL(up_down_smp)(arg);
      SYNCHRO_THREAD;
    }
#endif
  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, UPDOWN_SM2XTAB, d,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  if (flag)
    SYNCHRO_THREAD;
}

/* Calcul de alpha * x */
void Pastix_Scal(void *arg, pastix_float_t alpha, pastix_float_t *x, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscScal(sopalin_data, me, alpha, x,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  if (flag)
    SYNCHRO_THREAD;
}

/* Calcul du produit scalaire */
void Pastix_Dotc(void *arg, pastix_float_t *x, pastix_float_t *y, pastix_float_t *r, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscGradBeta(sopalin_data, me, x, y,
              UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, r, pastix_comm);
  MULTITHREAD_END(0);
  if (flag)
    SYNCHRO_THREAD;
}

void Pastix_Dotc_Gmres(void *arg, pastix_float_t *x, pastix_float_t *y, pastix_float_t *r, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscGmresBeta(sopalin_data, me, x, y,
               UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, r, pastix_comm);
  MULTITHREAD_END(0);
  SYNC_COEF(*r);
}

/* Produit matrice vecteur */
void Pastix_Ax(void *arg, pastix_float_t *x, pastix_float_t *r)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  SopalinParam     *sopar        = sopalin_data->sopar;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  MULTITHREAD_BEGIN;
  d_CscAx(sopalin_data, me, sopalin_data->sopar->cscmtx, x, r,
        datacode, &(datacode->updovct), pastix_comm,
        sopar->iparm[IPARM_TRANSPOSE_SOLVE]);
  MULTITHREAD_END(1);
}


/*** A MODIFIER! ***/
void Pastix_bMAx(void *arg, pastix_float_t *b, pastix_float_t *x, pastix_float_t *r)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  SopalinParam     *sopar        = sopalin_data->sopar;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;

  MULTITHREAD_BEGIN;
  d_CscCopy(sopalin_data, me, x, UPDOWN_SM2XTAB,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  SYNCHRO_THREAD;
  MULTITHREAD_BEGIN;
  d_CscbMAx(sopalin_data, me, r, b, sopalin_data->sopar->cscmtx,
          &(datacode->updovct), datacode, pastix_comm,
          sopar->iparm[IPARM_TRANSPOSE_SOLVE]);
  MULTITHREAD_END(1);
}

void Pastix_BYPX(void *arg, pastix_float_t *beta, pastix_float_t *y, pastix_float_t *x, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;

#ifdef MULT_SMX_RAFF
  {
    PASTIX_INT itersmx;
    for (itersmx=0; itersmx<UPDOWN_SM2XNBR; itersmx++)
      {
        MULTITHREAD_BEGIN;
        d_CscScal(sopalin_data, me, beta[itersmx], x+(itersmx*UPDOWN_SM2XSZE),
                UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
        MULTITHREAD_END(0);
        SYNCHRO_THREAD;
      }
  }
  MONOTHREAD_BEGIN;
  SOPALIN_GEAM("N","N",UPDOWN_SM2XSZE,UPDOWN_SM2XNBR, fun,
               y, UPDOWN_SM2XSZE, x, UPDOWN_SM2XSZE);
  MONOTHREAD_END;
#else
  MULTITHREAD_BEGIN;
  d_CscScal(sopalin_data, me, beta[0], x,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
  SYNCHRO_THREAD;
  MULTITHREAD_BEGIN;
  d_CscAXPY(sopalin_data, me, fun, y, x,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
#endif
  if (flag)
    SYNCHRO_THREAD;
}


void Pastix_AXPY(void *arg, double coeff, pastix_float_t *alpha, pastix_float_t *x, pastix_float_t *y, int flag)
{
  sopthread_data_t *argument     = (sopthread_data_t *)arg;
  Sopalin_Data_t   *sopalin_data = (Sopalin_Data_t *)(argument->data);
  d_SolverMatrix     *datacode     = sopalin_data->datacode;
  MPI_Comm          pastix_comm  = PASTIX_COMM;
  PASTIX_INT        me           = argument->me;
  pastix_float_t      tmp_flt;
#ifdef MULT_SMX_RAFF
  {
    PASTIX_INT itersmx;
    for(itersmx=0; itersmx<UPDOWN_SM2XNBR; itersmx++)
      {
        tmp_flt = (pastix_float_t) alpha[itersmx] * coeff;
        MULTITHREAD_BEGIN;
        d_CscAXPY(sopalin_data, me, tmp_flt, y+(itersmx*UPDOWN_SM2XSZE), x+(itersmx*UPDOWN_SM2XSZE),
                UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
        MULTITHREAD_END(1);
      }
  }
#else
  tmp_flt = (pastix_float_t) alpha[0] * coeff;
  MULTITHREAD_BEGIN;
  d_CscAXPY(sopalin_data, me, tmp_flt, y, x,
          UPDOWN_SM2XSZE, UPDOWN_SM2XNBR, pastix_comm);
  MULTITHREAD_END(0);
#endif
  if (flag)
    SYNCHRO_THREAD;
}


PASTIX_INT Pastix_me(void *arg)
{
  sopthread_data_t *argument = (sopthread_data_t *)arg;
  PASTIX_INT        me       = argument->me;
  return me;
}

void Pastix_Solveur(struct solver *solveur)
{
  /*** ALLOCATIONS ET SYNCHRONISATIONS ***/
  solveur->Synchro     = &Pastix_Synchro_Vect;
  solveur->Malloc      = &Pastix_Malloc;
  solveur->Free        = &Pastix_Free;

  /*** GESTION DE L'INTERFACE ***/
  solveur->Verbose = &Pastix_Verbose;
  solveur->End     = &Pastix_End;
  solveur->X       = &Pastix_X;
  solveur->N       = &Pastix_n;
  solveur->B       = &Pastix_B;
  solveur->Eps     = &Pastix_Eps;
  solveur->Itermax = &Pastix_Itermax;
  solveur->me      = &Pastix_me;
  solveur->Krylov_Space = &Pastix_Krylov_Space;

  /*** OPERATIONS DE BASE ***/
  solveur->Mult     = &Pastix_Mult;
  solveur->Div      = &Pastix_Div;
  solveur->Dotc_Gmres = &Pastix_Dotc_Gmres;

  solveur->Norm    = &Pastix_Norm2;
  solveur->Copy    = &Pastix_Copy;
  solveur->Precond = &Pastix_Precond;

  solveur->Scal    = &Pastix_Scal;
  solveur->Dotc    = &Pastix_Dotc;
  solveur->Ax      = &Pastix_Ax;

  solveur->AXPY    = &Pastix_AXPY;
  solveur->bMAx    = &Pastix_bMAx;
  solveur->BYPX    = &Pastix_BYPX;
}

/*
 ** Section: Function creating threads
 */
/*
 Function: method)

 Launch sopaparam->nbthrdcomm threads which will compute
 <method_smp)>.

 Parameters:
 datacode  - PaStiX <d_SolverMatrix> structure.
 sopaparam - <SopalinParam> parameters structure.
 */
void raff_thread(d_SolverMatrix *datacode, SopalinParam *sopaparam, void*(*method)(void *))
{
  Sopalin_Data_t *sopalin_data = NULL;
  BackupSolve_t b;

  MALLOC_INTERN(sopalin_data, 1, Sopalin_Data_t);

  solve_backup(datacode,&b);
  sopalin_init(sopalin_data, datacode, sopaparam, 0);

  sopalin_launch_thread(sopalin_data,
                        SOLV_PROCNUM,          SOLV_PROCNBR,                datacode->btree,
                        sopalin_data->sopar->iparm[IPARM_VERBOSE],
                        SOLV_THRDNBR,          method,                      sopalin_data,
                        sopaparam->nbthrdcomm, API_CALL(sopalin_updo_comm), sopalin_data,
                        OOC_THREAD_NBR,        ooc_thread,                  sopalin_data);

  sopalin_clean(sopalin_data, 2);
  solve_restore(datacode,&b);
  memFree_null(sopalin_data);
}
