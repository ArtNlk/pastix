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
#include "z_raff_functions.h"

/*
 ** Section: Functions declarations
 */

/* Raffinement du second membre */
#define z_gmres_smp         API_CALL(z_gmres_smp)
#define z_gmres_thread      API_CALL(z_gmres_thread)

void* z_gmres_smp        (void *arg);

/* Lancement d'une des fonctions seules */
void z_gmres_thread(z_SolverMatrix *datacode, z_SopalinParam *sopaparam);

/*
 ** Section: Threads routines
 */

/*
 Function: API_CALL(z_gmres_smp)

 Function computing GMRES iterative reffinement.

 Parameters:
 arg - Pointer to a <sopthread_data_t> structure containing
 the <z_Sopalin_Data_t> structure and the thread number ID.
 */

typedef struct gmres_s
{
  volatile PASTIX_INT gmresout_flag;     /*+ Flag for GMRES outter loop          +*/
  volatile PASTIX_INT gmresin_flag;      /*+ Flag for GMRES inner loop           +*/
  volatile double     gmresro;           /*+ Norm of GMRES residue               +*/
} gmres_t;

void* API_CALL(z_gmres_smp)(void *arg)
{
  struct z_solver solveur = {NULL};
  z_Pastix_Solveur(&solveur);

  z_RAFF_INT               n            = solveur.N(arg);
  Clock             raff_clk;
  double            t0           = 0;
  double            t3           = 0;
  z_RAFF_FLOAT          *  gmrestemp    = NULL;
  volatile z_RAFF_INT      gmresim      = 0;
  volatile z_RAFF_INT      gmresmaxits  = 0;
  z_RAFF_FLOAT          *  gmresb       = NULL;
  z_RAFF_FLOAT          ** gmresvv      = NULL;
  z_RAFF_FLOAT          ** gmreshh      = NULL;
  z_RAFF_FLOAT          *  gmresc       = NULL;
  z_RAFF_FLOAT          *  gmress       = NULL;
  z_RAFF_FLOAT          *  gmresrs      = NULL;
  z_RAFF_FLOAT          ** gmresw       = NULL;
  z_RAFF_FLOAT             gmresalpha;
  z_RAFF_FLOAT             gmrest;
  volatile z_RAFF_INT      gmresiters   = 0;
  z_RAFF_FLOAT          *  gmreswk1;
  z_RAFF_FLOAT          *  gmreswk2     = NULL;
  volatile double   gmreseps     = 0;
  volatile double   gmresnormb;
  volatile z_RAFF_INT      gmresi1      = 0;
  volatile z_RAFF_INT      i = 0;
  z_RAFF_INT               j, ii, k;
  z_RAFF_FLOAT             beta;
  z_RAFF_FLOAT          *  gmresx       = NULL;
  gmres_t        *  gmresdata;
  gmresim     = solveur.Krylov_Space(arg);
  gmresmaxits = solveur.Itermax(arg);
  gmreseps    = solveur.Eps(arg);

  gmrestemp = solveur.Malloc(arg, n           * sizeof(z_RAFF_FLOAT));
  gmresb    = solveur.Malloc(arg, n           * sizeof(z_RAFF_FLOAT));
  gmresc    = solveur.Malloc(arg, gmresim     * sizeof(z_RAFF_FLOAT));
  gmress    = solveur.Malloc(arg, gmresim     * sizeof(z_RAFF_FLOAT));
  gmresrs   = solveur.Malloc(arg, (gmresim+1) * sizeof(z_RAFF_FLOAT));
  gmresdata = solveur.Malloc(arg, 1           * sizeof(gmres_t));
  gmresx    = solveur.Malloc(arg, n           * sizeof(z_RAFF_FLOAT));

  MONO_BEGIN(arg);
  gmresvv = solveur.Malloc(arg, (gmresim+1) * sizeof(z_RAFF_FLOAT*));
  gmreshh = solveur.Malloc(arg, gmresim     * sizeof(z_RAFF_FLOAT*));
  gmresw  = solveur.Malloc(arg, gmresim     * sizeof(z_RAFF_FLOAT*));
  for (i=0; i<gmresim; i++)
    {
      gmresvv[i] = solveur.Malloc(arg, n           * sizeof(z_RAFF_FLOAT));
      gmreshh[i] = solveur.Malloc(arg, (gmresim+1) * sizeof(z_RAFF_FLOAT));
      gmresw[i]  = solveur.Malloc(arg, n           * sizeof(z_RAFF_FLOAT));
    }
  gmresvv[gmresim] = solveur.Malloc(arg, n * sizeof(z_RAFF_FLOAT));
  MONO_END(arg);
  SYNCHRO(arg);

  /* Synchronisations */
  gmrestemp  = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmrestemp, 0);
  gmresb     = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmresb,    1);
  gmresc     = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmresc,    2);
  gmress     = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmress,    3);
  gmresrs    = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmresrs,   4);
  gmresvv    = (z_RAFF_FLOAT **)solveur.Synchro(arg, (void*) gmresvv,   6);
  gmreshh    = (z_RAFF_FLOAT **)solveur.Synchro(arg, (void*) gmreshh,   7);
  gmresw     = (z_RAFF_FLOAT **)solveur.Synchro(arg, (void*) gmresw,    8);
  gmresdata  = (gmres_t*)solveur.Synchro(arg, (void*) gmresdata, 9);

  gmresnormb = (double)(*((double*)solveur.Synchro(arg, (void*) &gmresnormb, 10)));
  gmresx     = (z_RAFF_FLOAT * )solveur.Synchro(arg, (void*) gmresx,    11);

  gmresdata->gmresro = 0.0;
  gmresdata->gmresout_flag = 1;

  solveur.B(arg, gmresb);
  gmresnormb = solveur.Norm(arg, gmresb);

  solveur.X(arg, gmresx);

  gmresalpha = -1.0;
  gmresiters = 0;

  RAFF_CLOCK_INIT;

  while (gmresdata->gmresout_flag)
    {
      gmreswk2 = gmresvv[0];

      /* gmresvv[0] = b - A * x */
      solveur.bMAx(arg, gmresb, gmresx, gmresvv[0]);

      /* ro = vv[0].vv[0] */
      solveur.Dotc_Gmres(arg, gmresvv[0], gmresvv[0], &beta, 0);

#ifdef TYPE_COMPLEX
      gmresdata->gmresro = (z_RAFF_FLOAT)csqrt(beta);
#else
      gmresdata->gmresro = (z_RAFF_FLOAT)sqrt(beta);
#endif

      if ((double)ABS_FLOAT((z_RAFF_FLOAT)gmresdata->gmresro) <=
          gmreseps)
        {
          gmresdata->gmresout_flag = 0;
          break;
        }

      gmrest = (z_RAFF_FLOAT)(1.0/gmresdata->gmresro);

      solveur.Scal(arg, gmrest, gmresvv[0], 1);

      gmresrs[0] = (z_RAFF_FLOAT)gmresdata->gmresro;
      gmresdata->gmresin_flag = 1;
      i=-1;

      while(gmresdata->gmresin_flag)
        {
          RAFF_CLOCK_STOP;
          t0 = RAFF_CLOCK_GET;

          i++;
          gmresi1 = i+1;

          gmreswk1 = gmresvv[i];
          gmreswk2 = gmresw[i];

          SYNCHRO(arg);
          solveur.Precond(arg, gmreswk1, gmreswk2, 1);

          gmreswk1 = gmresvv[gmresi1];

          /* vv[i1] = A*wk2 */
          solveur.Ax(arg, gmreswk2, gmreswk1);

          /* classical gram - schmidt */
          for (j=0; j<=i; j++)
            {
              /* vv[j]*vv[i1] */
              solveur.Dotc_Gmres(arg,gmresvv[gmresi1], gmresvv[j], &beta, 0);

              gmreshh[i][j] = (z_RAFF_FLOAT)beta;
            }

          SYNCHRO(arg);

          for (j=0;j<=i;j++)
            {
              gmresalpha = -gmreshh[i][j];
              solveur.AXPY(arg, 1.0, &gmresalpha, gmresvv[gmresi1], gmresvv[j], 0);
            }

          SYNCHRO(arg);
          solveur.Dotc_Gmres(arg, gmresvv[gmresi1], gmresvv[gmresi1], &beta, 0);

#ifdef TYPE_COMPLEX
      gmrest = (z_RAFF_FLOAT)csqrt(beta);
#else
      gmrest = (z_RAFF_FLOAT)sqrt(beta);
#endif

          gmreshh[i][gmresi1] = gmrest;

          if (ABS_FLOAT(gmrest) > 10e-50)
            {
              gmrest = fun / gmrest;
              solveur.Scal(arg, gmrest, gmresvv[gmresi1], 0);
            }

          SYNCHRO(arg);
          MONO_BEGIN(arg);

          if (i != 0)
            {
              for (j=1; j<=i;j++)
                {
                  gmrest = gmreshh[i][j-1];
#ifdef TYPE_COMPLEX
                  gmreshh[i][j-1] = (z_RAFF_FLOAT)conj(gmresc[j-1])*gmrest +
                    (z_RAFF_FLOAT)conj(gmress[j-1])*gmreshh[i][j];
#else /* TYPE_COMPLEX */
                  gmreshh[i][j-1] =  gmresc[j-1]*gmrest +
                    gmress[j-1]*gmreshh[i][j];
#endif /* TYPE_COMPLEX */
                  gmreshh[i][j]   = -gmress[j-1]*gmrest +
                    gmresc[j-1]*gmreshh[i][j];
                }
            }
#ifdef TYPE_COMPLEX
          gmrest = (z_RAFF_FLOAT)csqrt(ABS_FLOAT(gmreshh[i][i]*gmreshh[i][i])+
                                       gmreshh[i][gmresi1]*gmreshh[i][gmresi1]);
#else
          gmrest = (z_RAFF_FLOAT)sqrt(gmreshh[i][i]*gmreshh[i][i]+
                                      gmreshh[i][gmresi1]*gmreshh[i][gmresi1]);
#endif
          if (ABS_FLOAT(gmrest) <= gmreseps)
            gmrest = (z_RAFF_FLOAT)gmreseps;

          gmresc[i] = gmreshh[i][i]/gmrest;
          gmress[i] = gmreshh[i][gmresi1]/gmrest;
          gmresrs[gmresi1] = -gmress[i]*gmresrs[i];

#ifdef TYPE_COMPLEX
          gmresrs[i] = (z_RAFF_FLOAT)conj(gmresc[i])*gmresrs[i];
          gmreshh[i][i] = (z_RAFF_FLOAT)conj(gmresc[i])*gmreshh[i][i] +
          gmress[i]*gmreshh[i][gmresi1];
#else
          gmresrs[i] = gmresc[i]*gmresrs[i];
          gmreshh[i][i] = gmresc[i]*gmreshh[i][i] +
          gmress[i]*gmreshh[i][gmresi1];
#endif
          gmresdata->gmresro = ABS_FLOAT(gmresrs[gmresi1]);

          MONO_END(arg);

          gmresiters++;

          MONO_BEGIN(arg);
          if ((i+1 >= gmresim) || (gmresdata->gmresro/gmresnormb <= gmreseps) || (gmresiters >= gmresmaxits))
            {
              gmresdata->gmresin_flag = 0;
            }
          MONO_END(arg);

          RAFF_CLOCK_STOP;
          t3 = RAFF_CLOCK_GET;
          solveur.Verbose(arg, t0, t3, gmresdata->gmresro/gmresnormb, gmresiters);
          SYNCHRO(arg);
        }

      MONO_BEGIN(arg);

      gmresrs[i] = gmresrs[i]/gmreshh[i][i];
      for (ii=2; ii<=i+1; ii++)
        {
          k = i-ii+1;
          gmrest = gmresrs[k];
          for (j=k+1; j<=i; j++)
            {
              gmrest = gmrest - gmreshh[j][k]*gmresrs[j];
            }
          gmresrs[k] = gmrest/gmreshh[k][k];
        }

      MONO_END(arg);
      SYNCHRO(arg);

      for (j=0; j<=i;j++)
        {
          gmrest = gmresrs[j];
          solveur.AXPY(arg, 1.0, &gmrest, gmresx, gmresw[j], 0);
        }
      SYNCHRO(arg);

      if ((gmresdata->gmresro/gmresnormb<= gmreseps) || (gmresiters >= gmresmaxits))
        {
          gmresdata->gmresout_flag = 0;
        }
    }

  RAFF_CLOCK_STOP;
  t3 = RAFF_CLOCK_GET;

  solveur.End(arg, gmresdata->gmresro/gmresnormb, gmresiters, t3, gmresx);

  solveur.Free(arg, (void*) gmrestemp);
  solveur.Free(arg, (void*) gmresb);
  solveur.Free(arg, (void*) gmresc);
  solveur.Free(arg, (void*) gmress);
  solveur.Free(arg, (void*) gmresrs);
  solveur.Free(arg, (void*) gmresdata);
  solveur.Free(arg, (void*) gmresx);

  MONO_BEGIN(arg);
  for (i=0; i<gmresim; i++)
    {
      solveur.Free(arg, gmresvv[i]);
      solveur.Free(arg, gmreshh[i]);
      solveur.Free(arg, gmresw[i]);
    }

  solveur.Free(arg, gmresvv[gmresim]);

  solveur.Free(arg, gmresvv);
  solveur.Free(arg, gmreshh);
  solveur.Free(arg, gmresw);
  MONO_END(arg);

  return 0;
}

/*
** Section: Function creating threads
*/
void API_CALL(z_gmres_thread)(z_SolverMatrix *datacode, z_SopalinParam *sopaparam)
{
  z_raff_thread(datacode, sopaparam, &API_CALL(z_gmres_smp));
}
