/**
 * File: z_step-by-step.c
 *
 *  Simple example calling PaStiX in step-by-step mode.
 *  Runs first step then 2 factorizations with 2 solves each.
 *
 *  @version 1.0.0
 *  @author Mathieu Faverge
 *  @author Pierre Ramet
 *  @author Xavier Lacoste
 *  @date 2011-11-11
 *  @precisions normal z -> c d s
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
/* to access functions from the libpastix, respect this order */
#include "z_pastix.h"
#include "z_read_matrix.h"
#include "get_options.h"
#include "z_utils.h"

#ifdef FORCE_NOMPI
#define MPI_COMM_WORLD 0
#endif

int main (int argc, char **argv)
{

  z_pastix_data_t    *pastix_data = NULL; /* Pointer to a storage structure needed by pastix           */
  pastix_int_t      ncol;               /* Size of the matrix                                        */
  pastix_int_t     *colptr      = NULL; /* Indexes of first element of each column in row and values */
  pastix_int_t     *rows        = NULL; /* Row of each element of the matrix                         */
  pastix_complex64_t   *values      = NULL; /* Value of each element of the matrix                       */
  pastix_complex64_t   *rhs         = NULL; /* right hand side                                           */
  pastix_complex64_t   *rhssaved    = NULL; /* right hand side                                           */
  pastix_complex64_t   *ax          = NULL; /* A times X product                                         */
  pastix_int_t      iparm[IPARM_SIZE];  /* integer parameters for pastix                             */
  double            dparm[DPARM_SIZE];  /* floating parameters for pastix                            */
  pastix_int_t     *perm        = NULL; /* Permutation tabular                                       */
  pastix_int_t     *invp        = NULL; /* Reverse permutation tabular                               */
  char             *type        = NULL; /* type of the matrix                                        */
  char             *rhstype     = NULL; /* type of the right hand side                               */
#ifndef FORCE_NOMPI
  int               required;           /* MPI thread level required                                 */
  int               provided;           /* MPI thread level provided                                 */
#endif
  driver_type_t    *driver_type;        /* Matrix driver(s) requested by user                        */
  char            **filename;           /* Filename(s) given by user                                 */
  int               nbmatrices;         /* Number of matrices given by user                          */
  int               nbthread;           /* Number of thread wanted by user                           */
  int               verbosemode;        /* Level of verbose mode (0, 1, 2)                           */
  int               ordering;           /* Ordering to use                                           */
  int               incomplete;         /* Indicate if we want to use incomplete factorisation       */
  int               level_of_fill;      /* Level of fill for incomplete factorisation                */
  int               amalgamation;       /* Level of amalgamation for Kass                            */
  int               ooc;                /* OOC limit (Mo/percent depending on compilation options)   */
  int               mpid;               /* Global MPI rank                                           */
  pastix_int_t      mat_type;
  long              i,j;
  double            norme1, norme2;
  int               nfact = 2;
  int               nsolv = 2;
  int               nbrhs = 1;

  /*******************************************/
  /*          MPI initialisation             */
  /*******************************************/
#ifndef FORCE_NOMPI
  required = MPI_THREAD_MULTIPLE;
  provided = -1;
  MPI_Init_thread(&argc, &argv, required, &provided);
  switch (provided)
    {
    case MPI_THREAD_SINGLE:
      printf("MPI_Init_thread level = MPI_THREAD_SINGLE\n");
      break;
    case MPI_THREAD_FUNNELED:
      printf("MPI_Init_thread level = MPI_THREAD_FUNNELED\n");
      break;
    case MPI_THREAD_SERIALIZED:
      printf("MPI_Init_thread level = MPI_THREAD_SERIALIZED\n");
      break;
    case MPI_THREAD_MULTIPLE:
      printf("MPI_Init_thread level = MPI_THREAD_MULTIPLE\n");
      break;
    default:
      printf("MPI_Init_thread level = ???\n");
    }
  MPI_Comm_rank (MPI_COMM_WORLD, &mpid);
#else
  mpid = 0;
#endif
  /*******************************************/
  /*    Get options from command line        */
  /*******************************************/
  if (EXIT_FAILURE == get_options(argc, argv,     &driver_type,
                                  &filename,      &nbmatrices,
                                  &nbthread,      &verbosemode,
                                  &ordering,      &incomplete,
                                  &level_of_fill, &amalgamation,
                                  &ooc,           &ncol))
    return EXIT_FAILURE;

  if (nbmatrices != 1)
    {
      /* Matrices for each iteration must have the same patern, this is why we only
         authorize one matrix in this exemple.
         But it could be used with several matrices with same patern and different values.
      */
      fprintf(stderr,"WARNING: should have only one matrix\n");
    }

  /*******************************************/
  /*      Read Matrice                       */
  /*******************************************/
  z_read_matrix(filename[0], &ncol, &colptr, &rows, &values,
                &rhs, &type, &rhstype, driver_type[0], MPI_COMM_WORLD);

  for (i = 0; i < nbmatrices; i++)
    if (filename[i] != NULL)
      free(filename[i]);
  free(filename);
  free(driver_type);

  mat_type = API_SYM_NO;
  if (MTX_ISSYM(type)) mat_type = API_SYM_YES;
  if (MTX_ISHER(type)) mat_type = API_SYM_HER;
  /*******************************************/
  /*    Check Matrix format                  */
  /*******************************************/
  /*
   * Matrix needs :
   *    - to be in fortran numbering
   *    - to have only the lower triangular part in symmetric case
   *    - to have a graph with a symmetric structure in unsymmetric case
   */
  z_pastix_checkMatrix(MPI_COMM_WORLD, verbosemode,
                       mat_type, API_YES,
                       ncol, &colptr, &rows, &values, NULL, 1);

  /*******************************************/
  /* Initialize parameters to default values */
  /*******************************************/
  iparm[IPARM_MODIFY_PARAMETER] = API_NO;
  z_pastix(&pastix_data, MPI_COMM_WORLD,
           ncol, colptr, rows, values,
           perm, invp, rhs, 1, iparm, dparm);

  /*******************************************/
  /*       Customize some parameters         */
  /*******************************************/
  iparm[IPARM_THREAD_NBR] = nbthread;
  iparm[IPARM_SYM] = mat_type;
  switch (mat_type)
  {
  case API_SYM_YES:
    iparm[IPARM_FACTORIZATION] = API_FACT_LDLT;
    break;
  case API_SYM_HER:
    iparm[IPARM_FACTORIZATION] = API_FACT_LDLH;
    break;
  default:
    iparm[IPARM_FACTORIZATION] = API_FACT_LU;
  }
  iparm[IPARM_MATRIX_VERIFICATION] = API_YES;
  iparm[IPARM_VERBOSE]             = verbosemode;
  iparm[IPARM_ORDERING]            = ordering;
  iparm[IPARM_INCOMPLETE]          = incomplete;
  iparm[IPARM_OOC_LIMIT]           = ooc;

  if (incomplete == 1)
    {
      dparm[DPARM_EPSILON_REFINEMENT] = 1e-7;
    }
  iparm[IPARM_LEVEL_OF_FILL]       = level_of_fill;
  iparm[IPARM_AMALGAMATION_LEVEL]  = amalgamation;
  iparm[IPARM_RHS_MAKING]          = API_RHS_B;

  /* reread parameters to set IPARM/DPARM */
  if (EXIT_FAILURE == d_get_idparm(argc, argv, iparm, dparm))
    return EXIT_FAILURE;

  perm = malloc(ncol*sizeof(pastix_int_t));
  invp = malloc(ncol*sizeof(pastix_int_t));

  /*******************************************/
  /*      Step 1 - Ordering / Scotch         */
  /*  Perform it only when the pattern of    */
  /*  matrix change.                         */
  /*  eg: mesh refinement                    */
  /*  In many cases users can simply go from */
  /*  API_TASK_ORDERING to API_TASK_ANALYSE  */
  /*  in one call.                           */
  /*******************************************/
  iparm[IPARM_START_TASK] = API_TASK_ORDERING;
  iparm[IPARM_END_TASK]   = API_TASK_ORDERING;

  z_pastix(&pastix_data, MPI_COMM_WORLD,
           ncol, colptr, rows, values,
           perm, invp, rhs, 1, iparm, dparm);

  /*******************************************/
  /*      Step 2 - Symbolic factorization    */
  /*  Perform it only when the pattern of    */
  /*  matrix change.                         */
  /*******************************************/
  iparm[IPARM_START_TASK] = API_TASK_SYMBFACT;
  iparm[IPARM_END_TASK]   = API_TASK_SYMBFACT;

  z_pastix(&pastix_data, MPI_COMM_WORLD,
         ncol, colptr, rows, values,
         perm, invp, rhs, 1, iparm, dparm);

  /*******************************************/
  /* Step 3 - Mapping and Compute scheduling */
  /*  Perform it only when the pattern of    */
  /*  matrix change.                         */
  /*******************************************/
  iparm[IPARM_START_TASK] = API_TASK_ANALYSE;
  iparm[IPARM_END_TASK]   = API_TASK_ANALYSE;

  z_pastix(&pastix_data, MPI_COMM_WORLD,
           ncol, colptr, rows, values,
           perm, invp, rhs, 1, iparm, dparm);

  /*******************************************/
  /*           Save the rhs                  */
  /*    (it will be replaced by solution)    */
  /*******************************************/
  rhssaved = malloc(nbrhs*ncol*sizeof(pastix_complex64_t));
  for(i=0; i<nbrhs; i++)
    memcpy(rhssaved+(i*ncol), rhs, ncol*sizeof(pastix_complex64_t));

  /* Do nfact factorization */
  for (i = 0; i < nfact; i++)
    {

      /*******************************************/
      /*     Step 4 - Numerical Factorisation    */
      /* Perform it each time the values of the  */
      /* matrix changed.                         */
      /*******************************************/
#ifndef FORCE_NOMPI
      if (mpid == 0)
#endif
        fprintf(stdout, "\t> Factorisation number %ld <\n", (long)(i+1));
      iparm[IPARM_START_TASK] = API_TASK_NUMFACT;
      iparm[IPARM_END_TASK]   = API_TASK_NUMFACT;

      z_pastix(&pastix_data, MPI_COMM_WORLD,
               ncol, colptr, rows, values,
               perm, invp, NULL, 1, iparm, dparm);

      /* Do two solve */
      for (j = 0; j < nsolv; j++)
        {
          /* Restore RHS (in this example, we have n times the same rhs ) */
          {
            int iter;
            for(iter=0; iter<nbrhs; iter++)
              memcpy(rhssaved+(iter*ncol), rhs, ncol*sizeof(pastix_complex64_t));
          }

          /*******************************************/
          /*     Step 5 - Solve and Refinnement      */
          /* For each one of your Right-hand-side    */
          /* members.                                */
          /* Also consider using multiple            */
          /* right-hand-side members.                */
          /*******************************************/
          iparm[IPARM_START_TASK] = API_TASK_SOLVE;
          iparm[IPARM_END_TASK]   = API_TASK_REFINE;

          PRINT_RHS("RHS", rhssaved, ncol, mpid, iparm[IPARM_VERBOSE]);

#ifndef FORCE_NOMPI
          if (mpid == 0)
#endif
            fprintf(stdout, "\t>> Solve And Refine step number %ld  <<\n", (long)(j+1));
          z_pastix(&pastix_data, MPI_COMM_WORLD,
                   ncol, colptr, rows, values,
                   perm, invp, rhssaved, nbrhs, iparm, dparm);

          PRINT_RHS("SOL", rhssaved, ncol, mpid, iparm[IPARM_VERBOSE]);
          CHECK_SOL(rhssaved, rhs, ncol, mpid);

          /* If you want separate Solve and Refinnement Steps */
          j++;
          /* Restore RHS */
          {
            int iter;
            for(iter=0; iter<nbrhs; iter++)
              memcpy(rhssaved+(iter*ncol), rhs, ncol*sizeof(pastix_complex64_t));
          }

          /*******************************************/
          /*     Step 5.1 - Solve                    */
          /* If you don't need iterative refinement  */
          /*******************************************/
          iparm[IPARM_START_TASK] = API_TASK_SOLVE;
          iparm[IPARM_END_TASK]   = API_TASK_SOLVE;

#ifndef FORCE_NOMPI
          if (mpid == 0)
#endif
            fprintf(stdout, "\t>> Solve step number %ld  <<\n", (long)(j+1));
          z_pastix(&pastix_data, MPI_COMM_WORLD,
                   ncol, colptr, rows, values,
                   perm, invp, rhssaved, nbrhs, iparm, dparm);

          PRINT_RHS("SOL", rhssaved, ncol, mpid, iparm[IPARM_VERBOSE]);
          CHECK_SOL(rhssaved, rhs, ncol, mpid);

          /* Restore RHS */
          {
            int iter;
            for(iter=0; iter<nbrhs; iter++)
              memcpy(rhssaved+(iter*ncol), rhs, ncol*sizeof(pastix_complex64_t));
          }
          /*******************************************/
          /*     Step 5.2 - Refinnement              */
          /*******************************************/
          iparm[IPARM_START_TASK] = API_TASK_REFINE;
          iparm[IPARM_END_TASK]   = API_TASK_REFINE;

#ifndef FORCE_NOMPI
          if (mpid == 0)
#endif
            fprintf(stdout, "\t>> Refine step number %ld  <<\n", (long)(j+1));
          z_pastix(&pastix_data, MPI_COMM_WORLD,
                   ncol, colptr, rows, values,
                   perm, invp, rhssaved, nbrhs, iparm, dparm);

          PRINT_RHS("RAF", rhssaved, ncol, mpid, iparm[IPARM_VERBOSE]);
          CHECK_SOL(rhssaved, rhs, ncol, mpid);
        }
    }

  /*******************************************/
  /*     Step 6 - Clean structures           */
  /* When you don't need PaStiX anymore      */
  /*******************************************/
  iparm[IPARM_START_TASK] = API_TASK_CLEAN;
  iparm[IPARM_END_TASK]   = API_TASK_CLEAN;

  z_pastix(&pastix_data, MPI_COMM_WORLD,
           ncol, colptr, rows, values,
           perm, invp, rhssaved, nbrhs, iparm, dparm);

  free(rhssaved);
  free(colptr);
  free(rows);
  free(values);
  free(perm);
  free(invp);
  free(rhs);
  free(type);
  free(rhstype);
#ifndef FORCE_NOMPI
  MPI_Finalize();
#endif
  return EXIT_SUCCESS;
}