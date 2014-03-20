#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "ftgt.h"
#include "symbol.h"
#include "queue.h"
#include "bulles.h"
#include "csc.h"
#include "updown.h"
#include "solver.h"
#include "elimin.h"
#include "cost.h"
#include "cand.h"
#include "extendVector.h"
#include "dof.h"
/* #include "extrastruct.h" */
/* #include "param_comm.h" */
#include "blendctrl.h"
#include "simu.h"
/* #include "perf.h" */
/* #include "costfunc.h" */
#include "task.h"
#include "solver_check.h"

/*#define DEBUG_PRIO*/

void solverCheck(SolverMatrix *solvmtx)
{
    pastix_int_t i, j, k = 0;
    pastix_int_t cblknum, bloknum, ftgtnum;
    pastix_int_t indnum, tasknum;
    pastix_int_t total;

    BlockCoeff *bcofptr = NULL;
    (void)bcofptr;

    /** Check the task **/
    for(i=0;i<solvmtx->tasknbr;i++)
      {
        cblknum = solvmtx->tasktab[i].cblknum;
        bloknum = solvmtx->tasktab[i].bloknum;
        indnum  = solvmtx->tasktab[i].indnum;
        ASSERT(cblknum < solvmtx->cblknbr,MOD_BLEND);
        ASSERT(bloknum < solvmtx->bloknbr,MOD_BLEND);
        if(indnum >= solvmtx->indnbr)
          printf("tasknbr %ld Tasknum %ld type %ld indnum %ld indnbr %ld \n", (long)solvmtx->tasknbr, (long)i, (long)solvmtx->tasktab[i].taskid, (long)indnum, (long)solvmtx->indnbr);
        /** OIMBE ce test foire (ligne precedente) si il y a du 1D jusqu'au bout !! mais a priori on s'en fout **/
        /*if(solvmtx->tasktab[i].taskid != 0 && solvmtx->cblktab[solvmtx->tasktab[i].cblknum]
          ASSERT(indnum < solvmtx->indnbr,MOD_BLEND);*/
        if(indnum >= solvmtx->indnbr)
          printf("cblknbr %ld cblknum %ld indnum %ld indnbr %ld \n", (long)solvmtx->cblknbr, (long)solvmtx->tasktab[i].cblknum, (long)indnum, (long)solvmtx->indnbr);
        ASSERT(solvmtx->tasktab[i].taskid >= 0,MOD_BLEND);
        ASSERT(solvmtx->tasktab[i].prionum >= 0,MOD_BLEND);
        switch(solvmtx->tasktab[i].taskid)
          {
            case COMP_1D:
              ASSERT(bloknum  == solvmtx->cblktab[cblknum].bloknum,MOD_BLEND);
              for(j = bloknum+1;j<solvmtx->cblktab[cblknum+1].bloknum;j++)
                {
                  for(k=j;k<solvmtx->cblktab[cblknum+1].bloknum;k++)
                    {
                      /*#ifdef NAPA*/
                      if(solvmtx->indtab[indnum] > solvmtx->ftgtnbr) /** No ftgt **/
                        {
                          indnum++;
                          continue;
                        }
                      /*#endif*/
                      if(solvmtx->indtab[indnum] < 0)
                        {
                          tasknum = -solvmtx->indtab[indnum];
                          switch(solvmtx->tasktab[tasknum].taskid)
                              {
                              case COMP_1D:
                                {
                                  pastix_int_t facebloknum, facecblknum;
                                  facecblknum = solvmtx->bloktab[j].cblknum;
                                  ASSERT(facecblknum >= 0,MOD_BLEND);
                                  ASSERT(facecblknum == solvmtx->tasktab[tasknum].cblknum,MOD_BLEND);
                                  facebloknum = solvmtx->cblktab[facecblknum].bloknum;

                                  while ( ! (    ( solvmtx->bloktab[k].frownum >= solvmtx->bloktab[facebloknum].frownum &&
                                                   solvmtx->bloktab[k].frownum <= solvmtx->bloktab[facebloknum].lrownum)
                                              || ( solvmtx->bloktab[k].lrownum >= solvmtx->bloktab[facebloknum].frownum &&
                                                   solvmtx->bloktab[k].lrownum <= solvmtx->bloktab[facebloknum].lrownum)
                                              || ( solvmtx->bloktab[k].frownum <= solvmtx->bloktab[facebloknum].frownum &&
                                                   solvmtx->bloktab[k].lrownum >= solvmtx->bloktab[facebloknum].lrownum)))
                                    facebloknum++;

                                  ASSERT(solvmtx->bloktab[k].frownum >= solvmtx->bloktab[facebloknum].frownum,MOD_BLEND);
                                  ASSERT(solvmtx->bloktab[k].lrownum <= solvmtx->bloktab[facebloknum].lrownum,MOD_BLEND);
                                  ASSERT(solvmtx->bloktab[j].frownum >= solvmtx->cblktab[facecblknum].fcolnum,MOD_BLEND);
                                  ASSERT(solvmtx->bloktab[j].lrownum <= solvmtx->cblktab[facecblknum].lcolnum,MOD_BLEND);
                                }
                                break;
                              default:
                                  errorPrint("tasknum %ld tasknbr %ld taskid %ld",
                                             (long)tasknum, (long)solvmtx->tasknbr,
                                             (long)solvmtx->tasktab[tasknum].taskid);
                                  EXIT(MOD_BLEND,INTERNAL_ERR);
                              }
                        }
                      else
                        {
                          ftgtnum = solvmtx->indtab[indnum];


                          ASSERT(solvmtx->bloktab[j].frownum >= solvmtx->ftgttab[ftgtnum].infotab[FTGT_FCOLNUM],MOD_BLEND);
                          ASSERT(solvmtx->bloktab[j].lrownum <= solvmtx->ftgttab[ftgtnum].infotab[FTGT_LCOLNUM],MOD_BLEND);
                          ASSERT(solvmtx->bloktab[k].frownum >= solvmtx->ftgttab[ftgtnum].infotab[FTGT_FROWNUM],MOD_BLEND);
                          ASSERT(solvmtx->bloktab[k].lrownum <= solvmtx->ftgttab[ftgtnum].infotab[FTGT_LROWNUM],MOD_BLEND);

                          /*ASSERT( (solvmtx->ftgttab[ftgtnum].infotab[FTGT_LCOLNUM] - solvmtx->ftgttab[ftgtnum].infotab[FTGT_FCOLNUM] + 1)* (solvmtx->ftgttab[ftgtnum].infotab[FTGT_LROWNUM] - solvmtx->ftgttab[ftgtnum].infotab[FTGT_FROWNUM] + 1) <= solvmtx->cpftmax,MOD_BLEND);*/

                          solvmtx->ftgttab[ftgtnum].infotab[FTGT_CTRBCNT]--;

#ifdef DEBUG_PRIO
                          if( solvmtx->ftgttab[ftgtnum].infotab[FTGT_CTRBCNT] == 0)
                            if(solvmtx->ftgttab[ftgtnum].infotab[FTGT_PRIONUM] != solvmtx->tasktab[i].prionum)
                              fprintf(stdout, "Task1D %ld FTGT %ld  taskprio %ld ftgtprio %ld \n", (long)i, (long)ftgtnum, (long)solvmtx->tasktab[i].prionum, (long)solvmtx->ftgttab[ftgtnum].infotab[FTGT_PRIONUM]);
#endif
                          /*fprintf(stdout ," [ %ld %ld ] [%ld %ld ] \n", (long)solvmtx->ftgttab[ftgtnum].infotab[FTGT_FCOLNUM],
                                  (long)solvmtx->ftgttab[ftgtnum].infotab[FTGT_LCOLNUM],
                                  (long)solvmtx->ftgttab[ftgtnum].infotab[FTGT_FROWNUM],
                                  (long)solvmtx->ftgttab[ftgtnum].infotab[FTGT_LROWNUM]);*/
                        }
                      indnum++;
                    }
                }
              break;
          default:
            fprintf(stderr, "solver_check: The task %ld has no type \n", (long)i);
            EXIT(MOD_BLEND,INTERNAL_ERR);
          }
      }
    for(i=0;i<solvmtx->ftgtnbr;i++)
      {
        ASSERT(solvmtx->ftgttab[i].infotab[FTGT_CTRBNBR]>0,MOD_BLEND);
        ASSERT(solvmtx->ftgttab[i].infotab[FTGT_CTRBCNT]==0,MOD_BLEND);
      }

    /** Reset the ftgt ctrbcnt **/
    for(i=0;i<solvmtx->ftgtnbr;i++)
      {
        ASSERT(solvmtx->ftgttab[i].infotab[FTGT_CTRBNBR]>0,MOD_BLEND);
        ASSERT(solvmtx->ftgttab[i].infotab[FTGT_CTRBCNT]==0,MOD_BLEND);
        solvmtx->ftgttab[i].infotab[FTGT_CTRBCNT] = solvmtx->ftgttab[i].infotab[FTGT_CTRBNBR];
      }

    /** Test the task partition on the thread of the cluster **/
    total = 0;
    for(i=0;i<solvmtx->bublnbr;i++){
        printf("i = %d, ttsknbr = %d\n", (int)i, (int)(solvmtx->ttsknbr[i]));
      total += solvmtx->ttsknbr[i];
    }
    if(total != solvmtx->tasknbr)
      fprintf(stderr, " total %ld tasknbr %ld \n", (long)total, (long)solvmtx->tasknbr);

    ASSERT(total == solvmtx->tasknbr,MOD_BLEND);

#ifdef DEBUG_BLEND
    {
      pastix_int_t * flag;
      MALLOC_INTERN(flag, solvmtx->tasknbr, pastix_int_t);
      bzero(flag, sizeof(pastix_int_t)*solvmtx->tasknbr);

      for(i=0;i<solvmtx->bublnbr;i++)
        for(j=0;j<solvmtx->ttsknbr[i];j++)
          {
            if(flag[solvmtx->ttsktab[i][j]] != 0)
              fprintf(stderr, "flag %ld thread %ld task %ld already on another thread \n", (long)flag[solvmtx->ttsktab[i][j]], (long)i, (long)solvmtx->ttsktab[i][j]);
            flag[solvmtx->ttsktab[i][j]]++;

          }

      for(i=0;i<solvmtx->tasknbr;i++)
        ASSERT(flag[i] == 1,MOD_BLEND);

      memFree(flag);
    }
#endif

#ifdef PASTIX_DYNSCHED
    {
      int k, bubnbr = solvmtx->bublnbr;

      for(k = 0; k<bubnbr; k++)
        {
          int father = BFATHER( solvmtx->btree, k );
          if ( (father != -1) &&
               (  solvmtx->btree->nodetab[k].priomax >  solvmtx->btree->nodetab[father].priomin ) )
            fprintf(stderr, "We have a problem of task distribution\n"
                    " Bubble[%d] priorities (%ld,%ld) intersect with bubble[%d](%ld,%ld)\n",
                    k,      (long)solvmtx->btree->nodetab[k].priomin,      (long)solvmtx->btree->nodetab[k].priomax,
                    father, (long)solvmtx->btree->nodetab[father].priomin, (long)solvmtx->btree->nodetab[father].priomax) ;
        }
    }
#endif

}
