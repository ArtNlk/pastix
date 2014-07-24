#include <stdlib.h>
#include <stdio.h>


#include "common.h"
#include "z_ftgt.h"
#include "queue.h"
#include "bulles.h"
#include "z_updown.h"
#include "z_solver.h"
/* #include "assert.h" */
#include "solverRealloc.h"

/*+ Realloc the solver matrix in a contiguous way +*/
void solverRealloc(z_SolverMatrix *solvmtx)
{
    z_SolverMatrix *tmp;
    z_SolverBlok   *solvblok;
    z_SolverCblk   *solvcblk;
    pastix_int_t i;

    MALLOC_INTERN(tmp, 1, z_SolverMatrix);
    /** copy general info **/
    memcpy(tmp, solvmtx, sizeof(z_SolverMatrix));

    /**OIMBE il faudra faire le REALLOC pour ooc ! **/

    /** Copy tasktab **/
    MALLOC_INTERN(solvmtx->tasktab, solvmtx->tasknbr, z_Task);
    memcpy(solvmtx->tasktab, tmp->tasktab, solvmtx->tasknbr*sizeof(z_Task));
#ifdef DEBUG_BLEND
    for(i=0;i<solvmtx->tasknbr;i++)
      ASSERT((solvmtx->tasktab[i].btagptr == NULL), MOD_BLEND);
#endif

    /** Copy cblktab and bloktab **/
    MALLOC_INTERN(solvmtx->cblktab, solvmtx->cblknbr+1, z_SolverCblk);
    memcpy(solvmtx->cblktab, tmp->cblktab,
           (solvmtx->cblknbr+1)*sizeof(z_SolverCblk));

    MALLOC_INTERN(solvmtx->bloktab, solvmtx->bloknbr, z_SolverBlok);
    memcpy(solvmtx->bloktab, tmp->bloktab,
           solvmtx->bloknbr*sizeof(z_SolverBlok));

    solvblok = solvmtx->bloktab;
    for (solvcblk = solvmtx->cblktab; solvcblk  < solvmtx->cblktab + solvmtx->cblknbr; solvcblk++) {
        pastix_int_t bloknbr = (solvcblk+1)->fblokptr - solvcblk->fblokptr;
        solvcblk->fblokptr = solvblok;
        solvblok+= bloknbr;
    }
    solvcblk->fblokptr = solvblok;

#if defined(PASTIX_WITH_STARPU)
    if ( tmp->gcblk2halo ) {
        MALLOC_INTERN(solvmtx->gcblk2halo, solvmtx->gcblknbr, pastix_int_t);
        memcpy(solvmtx->gcblk2halo, tmp->gcblk2halo,
               solvmtx->gcblknbr*sizeof(pastix_int_t));
    }
    if ( tmp->hcblktab ) {
        MALLOC_INTERN(solvmtx->hcblktab, solvmtx->hcblknbr+1, z_SolverCblk);
        memcpy(solvmtx->hcblktab, tmp->hcblktab,
               (solvmtx->hcblknbr+1)*sizeof(z_SolverCblk));
        MALLOC_INTERN(solvmtx->hbloktab, tmp->hcblktab[tmp->hcblknbr].fblokptr - tmp->hbloktab,
                      z_SolverBlok);
        memcpy(solvmtx->hbloktab, tmp->hbloktab,
               (tmp->hcblktab[tmp->hcblknbr].fblokptr - tmp->hbloktab)*sizeof(z_SolverBlok));

        solvblok = solvmtx->hbloktab;
        for (solvcblk = solvmtx->hcblktab;
             solvcblk  < solvmtx->hcblktab + solvmtx->hcblknbr;
             solvcblk++) {
            pastix_int_t bloknbr = (solvcblk+1)->fblokptr - solvcblk->fblokptr;
            solvcblk->fblokptr = solvblok;
            solvblok+= bloknbr;
        }
        solvcblk->fblokptr = solvblok;
    }

    if (pastix_starpu_with_fanin() == API_YES) {
        /* FANIN info */
        pastix_int_t clustnum;
        MALLOC_INTERN(solvmtx->fcblktab, solvmtx->clustnbr, z_SolverCblk*);
        MALLOC_INTERN(solvmtx->fbloktab, solvmtx->clustnbr, z_SolverBlok*);
        MALLOC_INTERN(solvmtx->fcblknbr, solvmtx->clustnbr, pastix_int_t);
        memset(solvmtx->fcblknbr, 0, solvmtx->clustnbr*sizeof(pastix_int_t));
        memset(solvmtx->fcblktab, 0, solvmtx->clustnbr*sizeof(z_SolverCblk*));
        memset(solvmtx->fbloktab, 0, solvmtx->clustnbr*sizeof(z_SolverBlok*));

        memcpy(solvmtx->fcblknbr, tmp->fcblknbr,
               solvmtx->clustnbr*sizeof(pastix_int_t));
        for (clustnum = 0; clustnum < solvmtx->clustnbr; clustnum++) {
            pastix_int_t bloknbr;
            MALLOC_INTERN(solvmtx->fcblktab[clustnum],
                          solvmtx->fcblknbr[clustnum]+1,
                          z_SolverCblk);
            if ( solvmtx->fcblknbr[clustnum] > 0 ) {
                memcpy(solvmtx->fcblktab[clustnum],
                       tmp->fcblktab[clustnum],
                       (solvmtx->fcblknbr[clustnum]+1)*sizeof(z_SolverCblk));
                bloknbr = tmp->fcblktab[clustnum][tmp->fcblknbr[clustnum]].fblokptr - tmp->fbloktab[clustnum];
                MALLOC_INTERN(solvmtx->fbloktab[clustnum],
                              bloknbr,
                              z_SolverBlok);
                memcpy(solvmtx->fbloktab[clustnum],
                       tmp->fbloktab[clustnum],
                       (bloknbr)*sizeof(z_SolverBlok));
                solvblok = solvmtx->fbloktab[clustnum];
                for (solvcblk = solvmtx->fcblktab[clustnum];
                     solvcblk  < solvmtx->fcblktab[clustnum] +
                         solvmtx->fcblknbr[clustnum];
                     solvcblk++) {

                    pastix_int_t bloknbr = (solvcblk+1)->fblokptr - solvcblk->fblokptr;
                    solvcblk->fblokptr = solvblok;
                    solvblok+= bloknbr;
                }
                solvcblk->fblokptr = solvblok;

            }

        }
    }

#endif /* defined(PASTIX_WITH_STARPU) */

    /** Copy ftgttab **/
    if (solvmtx->ftgtnbr != 0)
      {
        MALLOC_INTERN(solvmtx->ftgttab, solvmtx->ftgtnbr, z_FanInTarget);
        memcpy(solvmtx->ftgttab, tmp->ftgttab,
               solvmtx->ftgtnbr*sizeof(z_FanInTarget));
      }
    /** copy infotab of fan intarget **/
    /*for(i=0;i<tmp->ftgtnbr;i++)
      memcpy(solvmtx->ftgttab[i].infotab, tmp->ftgttab[i].infotab, MAXINFO*sizeof(pastix_int_t));*/

    /** Copy indtab **/
    MALLOC_INTERN(solvmtx->indtab, solvmtx->indnbr, pastix_int_t);
    memcpy(solvmtx->indtab, tmp->indtab, solvmtx->indnbr*sizeof(pastix_int_t));


    /** Copy ttsktab & ttsknbr **/
    if (solvmtx->bublnbr>0)
      {
        MALLOC_INTERN(solvmtx->ttsknbr, solvmtx->bublnbr, pastix_int_t);
        memcpy(solvmtx->ttsknbr, tmp->ttsknbr, solvmtx->bublnbr*sizeof(pastix_int_t));
        MALLOC_INTERN(solvmtx->ttsktab, solvmtx->bublnbr, pastix_int_t*);
        for (i=0;i<solvmtx->bublnbr;i++)
          {
            solvmtx->ttsktab[i] = NULL;
            MALLOC_INTERN(solvmtx->ttsktab[i], solvmtx->ttsknbr[i], pastix_int_t);
            memcpy(solvmtx->ttsktab[i], tmp->ttsktab[i],
                   solvmtx->ttsknbr[i]*sizeof(pastix_int_t));
          }
      }
    else
      {
        solvmtx->ttsknbr = NULL;
        solvmtx->ttsktab = NULL;
      }

    MALLOC_INTERN(solvmtx->proc2clust, solvmtx->procnbr, pastix_int_t);
    memcpy(solvmtx->proc2clust, tmp->proc2clust,
           solvmtx->procnbr * sizeof(pastix_int_t));

    /** Free the former solver matrix **/
    solverExit(tmp);
    memFree_null(tmp);
}


void solverExit(z_SolverMatrix *solvmtx)
{
    pastix_int_t i;

    /** Free arrays of solvmtx **/
    if(solvmtx->cblktab)
      {
        for (i = 0; i < solvmtx->cblknbr; i++)
          {
            if (solvmtx->cblktab[i].coeftab)
              memFree_null(solvmtx->cblktab[i].coeftab);

            if (solvmtx->cblktab[i].ucoeftab)
              memFree_null(solvmtx->cblktab[i].ucoeftab);
          }
        memFree_null(solvmtx->cblktab);
      }
    if(solvmtx->bloktab)
      memFree_null(solvmtx->bloktab);
    /*if(solvmtx->coeftab)
      memFree_null(solvmtx->coeftab);*/
    if(solvmtx->ftgttab)
      memFree_null(solvmtx->ftgttab);
    if(solvmtx->tasktab)
      memFree_null(solvmtx->tasktab);
    if(solvmtx->indtab)
      memFree_null(solvmtx->indtab);
    memFree_null(solvmtx->ttsknbr);
    for (i=0;i<solvmtx->bublnbr;i++)
      {
        if (solvmtx->ttsktab[i] != NULL)
          memFree_null(solvmtx->ttsktab[i]);
      }
    memFree_null(solvmtx->ttsktab);
    memFree_null(solvmtx->proc2clust);
    /*memFree_null(solvmtx);*/
#if defined(PASTIX_WITH_STARPU)
    memFree_null(solvmtx->hcblktab);
    memFree_null(solvmtx->hbloktab);
    memFree_null(solvmtx->gcblk2halo);
    if (pastix_starpu_with_fanin() == API_YES) {
        pastix_int_t clustnum;
        for (clustnum = 0; clustnum < solvmtx->clustnbr; clustnum++) {
            if (solvmtx->fcblknbr[clustnum] > 0) {
                memFree_null(solvmtx->fcblktab[clustnum]);
                memFree_null(solvmtx->fbloktab[clustnum]);
            }
        }
        memFree_null(solvmtx->fcblknbr);
        memFree_null(solvmtx->fcblktab);
        memFree_null(solvmtx->fbloktab);
    }
#endif
}


void solverInit(z_SolverMatrix *solvmtx)
{
  solvmtx->cblktab = NULL;
  solvmtx->bloktab = NULL;
  solvmtx->coefnbr = 0;
  solvmtx->ftgtnbr = 0;

  solvmtx->ftgttab = NULL;
  solvmtx->coefmax = 0;
  memset(solvmtx, 0, sizeof (z_SolverMatrix));

  solvmtx->baseval = 0;
  solvmtx->cblknbr = 0;
  solvmtx->bloknbr = 0;
  solvmtx->nodenbr = 0;
#if defined(PASTIX_WITH_STARPU)
  solvmtx->hcblktab = NULL;
  solvmtx->hbloktab = NULL;
  solvmtx->gcblk2halo = NULL;
  solvmtx->fcblktab = NULL;
  solvmtx->fbloktab = NULL;
  solvmtx->fcblknbr = NULL;
#endif /* defined(PASTIX_WITH_STARPU) */
}
