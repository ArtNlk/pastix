#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "elimin.h"
#include "cost.h"
#include "extendVector.h"
#include "cand.h"
#include "queue.h"
#include "bulles.h"
#include "param_blend.h"
/* #include "param_comm.h" */
/* #include "symbol.h" */
/* #include "ftgt.h" */
/* #include "simu.h" */
/* #include "assert.h" */
#include "blendctrl.h"
#include "perf.h"


/*void perfcluster(pastix_int_t procsrc, pastix_int_t procdst, netperf *np, BlendCtrl *ctrl)
{
#ifdef DEBUG_BLEND
  ASSERT(procsrc>=0 && procsrc < ctrl->procnbr,MOD_BLEND);
  ASSERT(procdst>=0 && procdst < ctrl->procnbr,MOD_BLEND);
#endif

  if(procsrc == procdst)
    {
      np->startup   = 0;
      np->bandwidth = 0;
      return;
    }

  if(CLUSTNUM(procsrc) == CLUSTNUM(procdst))
  {
      np->startup   = TIME_STARTUP_1;
      np->bandwidth = TIME_BANDWIDTH_1;
      return;
    }
  else
    {
      np->startup   = CLUSTER_STARTUP_1;
      np->bandwidth = CLUSTER_BANDWIDTH_1;
      return;
    }
}
*/

void perfcluster2(pastix_int_t clustsrc, pastix_int_t clustdst, pastix_int_t sync_comm_nbr, netperf *np, BlendCtrl *ctrl)
{
#ifdef DEBUG_BLEND
  ASSERT(clustsrc>=0 && clustsrc < ctrl->clustnbr,MOD_BLEND);
  ASSERT(clustdst>=0 && clustdst < ctrl->clustnbr,MOD_BLEND);
  ASSERT(sync_comm_nbr>0 && sync_comm_nbr <= ctrl->clustnbr,MOD_BLEND);
#endif


  if(clustsrc == clustdst)
    {
      np->startup   = 0;
      np->bandwidth = 0;
      return;
    }


  if(SMPNUM(clustsrc) == SMPNUM(clustdst))
    {

      /*fprintf(stderr, "MPI_SHARED for %ld \n", (long)sync_comm_nbr);*/
      if(sync_comm_nbr <=2)
        {
          np->startup   = SHARED_STARTUP_1;
          np->bandwidth = SHARED_BANDWIDTH_1;
          return;
        }
      if(sync_comm_nbr<=4)
        {
          np->startup   = SHARED_STARTUP_2;
          np->bandwidth = SHARED_BANDWIDTH_2;
          return;
        }
      if(sync_comm_nbr<=8)
        {
          np->startup   = SHARED_STARTUP_4;
          np->bandwidth = SHARED_BANDWIDTH_4;
          return;
        }
      if(sync_comm_nbr > 8)
        {
          /*fprintf(stdout, "intra %ld extra %ld\n", intra, extra);*/
          np->startup   = SHARED_STARTUP_8;
          np->bandwidth = SHARED_BANDWIDTH_8;
          return;
        }

    }
  else
    {
      /*fprintf(stderr, "MPI for %ld \n", (long)sync_comm_nbr);*/
      /*      extra++;*/
      if(sync_comm_nbr<=2)
        {
          np->startup   = CLUSTER_STARTUP_1;
          np->bandwidth = CLUSTER_BANDWIDTH_1;
          return;
        }
      if(sync_comm_nbr<=4)
        {
          np->startup   = CLUSTER_STARTUP_2;
          np->bandwidth = CLUSTER_BANDWIDTH_2;
          return;
        }
      if(sync_comm_nbr<=8)
        {
          np->startup   = CLUSTER_STARTUP_4;
          np->bandwidth = CLUSTER_BANDWIDTH_4;
          return;
        }
      if(sync_comm_nbr > 8)
        {
          /*fprintf(stdout, "intra %ld extra %ld\n", intra, extra);*/
          np->startup   = CLUSTER_STARTUP_8;
          np->bandwidth = CLUSTER_BANDWIDTH_8;
          return;
        }
    }

}

pastix_int_t blendCtrlInit(BlendCtrl *ctrl,
                  pastix_int_t clustnbr,
                  pastix_int_t thrdlocnbr,
                  pastix_int_t cudanbr,
                  pastix_int_t clustnum,
                  BlendParam *param)
{
    pastix_int_t i;

    ctrl->option  = param;
    MALLOC_INTERN(ctrl->perfptr, 1, netperf);

    /* Nombre et num�ro de processus MPI */
    ctrl->clustnbr   = clustnbr;
    ctrl->clustnum   = clustnum;
    ctrl->cudanbr    = cudanbr;

#ifdef PASTIX_DYNSCHED
    /* Le nombre de coeur par cpu est est donn� par iparm[IPARM_CPU_BY_NODE]
       et a defaut par sysconf(_SC_NPROCESSORS_ONLN)                          */
    if (ctrl->option->iparm[IPARM_CPU_BY_NODE] != 0)
      ctrl->option->procnbr = ctrl->option->iparm[IPARM_CPU_BY_NODE];

    /* Calcul du nombre de cpu total a notre disposition */
    if (ctrl->option->smpnbr)
      ctrl->procnbr = ctrl->option->procnbr * ctrl->option->smpnbr;
    else
      ctrl->procnbr = ctrl->option->procnbr * clustnbr;

    /* Cas ou on a moins de proc que de processus MPI */
    if (ctrl->clustnbr > ctrl->procnbr)
      {
        errorPrintW("blenctrlinit: plus de processus MPI que de processeurs disponible");
        ctrl->procnbr = ctrl->clustnbr;
/*      ctrl->option->procnbr = (int)ceil((double)ctrl->procnbr / (double)ctrl->option->smpnbr); */
      }

    /* Nombre de processeurs utilis�s pour un processus MPI */
    /* et nombre de threads demand�s pour un processus MPI  */
    ctrl->proclocnbr = ctrl->procnbr / ctrl->clustnbr;
    ctrl->thrdlocnbr = thrdlocnbr;
#else

    ctrl->proclocnbr = thrdlocnbr;
    ctrl->thrdlocnbr = thrdlocnbr;
    ctrl->procnbr    = ctrl->proclocnbr * ctrl->clustnbr;
#endif

    ctrl->thrdnbr = ctrl->thrdlocnbr * clustnbr;
    ctrl->bublnbr = ctrl->thrdlocnbr;

    /* Tableau d'affectation de processeur par processus MPI */
    MALLOC_INTERN(ctrl->proc2clust, ctrl->procnbr, pastix_int_t);
    for(i=0;i<ctrl->procnbr;i++)
      ctrl->proc2clust[i] = CLUSTNUM(i);

    ctrl->egraph  = NULL;
    ctrl->etree   = NULL;
    ctrl->costmtx = NULL;
    ctrl->candtab = NULL;
    MALLOC_INTERN(ctrl->lheap, 1, Queue);
    queueInit(ctrl->lheap, 1000);
    MALLOC_INTERN(ctrl->intvec, 1, ExtendVectorINT);
    MALLOC_INTERN(ctrl->intvec2, 1, ExtendVectorINT);
    /* if(param->tracegen) */
    /*   OUT_OPENFILEINDIR(param->iparm, ctrl->tracefile, param->trace_filename, "w"); */

#ifdef PASTIX_DYNSCHED
    MALLOC_INTERN(ctrl->btree, 1, BubbleTree);
#endif
    return ( (extendint_Init(ctrl->intvec, 10) != NULL) && (extendint_Init(ctrl->intvec2, 10) != NULL));
}


void blendCtrlExit(BlendCtrl *ctrl)
{
  /* if(ctrl->option->tracegen) */
  /*   OUT_CLOSEFILEINDIR(ctrl->tracefile); */
  if(ctrl->perfptr)
    memFree_null(ctrl->perfptr);
  queueExit(ctrl->lheap);
  memFree_null(ctrl->lheap);
  extendint_Exit(ctrl->intvec);
  memFree_null(ctrl->intvec);
  extendint_Exit(ctrl->intvec2);
  memFree_null(ctrl->intvec2);

  if(ctrl->proc2clust)
    memFree_null(ctrl->proc2clust);
  if(ctrl->candtab)
    memFree_null(ctrl->candtab);

  memFree_null(ctrl);
}
