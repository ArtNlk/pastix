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
 * File: z_read_matrix.c
 *
 * Definition of a global function to read all type of matrices.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "z_pastix.h"
#include "z_read_matrix.h"
#include "common_drivers.h"
#include "get_options.h"

/* #include "z_cccread.h" */
/* #include "z_chbread.h" */
#include "z_cscdread.h"
/* #include "z_csparse.h" */
/* #include "z_fdupread.h" */
/* #include "z_hbread.h" */
/* #include "iohb.h" */
#include "z_laplacian.h"
#include "z_mmdread.h"
/* //#include "mmio.h" */
/* #include "z_mmread.h" */
#include "z_mtx.h"
/* #include "olafread.h" */
/* #include "z_peerread.h" */
#include "z_petscread.h"
/* #include "z_rsaread.h" */
#include "z_threefilesread.h"

/*
 * Function: read_matrix_common
 *
 * Common part to <z_read_matrix> and <z_dread_matrix>.
 *
 * Reads a matrix from a file in the format given by
 * driver_type (see <driver_type_enum>).
 *
 * Parameters:
 *   filename    - Name of the file to read from.
 *   ncol        - Number of column in the matrix (output).
 *   colptr      - Indexes in rows and avals of first element of
 *                 each column of the matrix.(output)
 *   rows        - Row of each element of the matrix.
 *   loc2glob    - Local to global column number indirection.
 *   values      - Values of each element of the matrix.
 *   type        - type of the matrix.
 *   rhstype     - type of the right and side.
 *   driver_type - driver to use to read the matrix.
 *   pastix_comm - MPI communicator containing all processes wich
 *                 call z_read_matrix.
 */
static inline
int read_matrix_common(char            *filename,    pastix_int_t    *ncol,
                       pastix_int_t   **colptr,      pastix_int_t   **rows,
                       pastix_int_t   **loc2glob,
                       pastix_complex64_t **values,      pastix_complex64_t **rhs,
                       char           **type,        char           **rhstype,
                       driver_type_t    driver_type, MPI_Comm         pastix_comm)
{
  pastix_int_t   nrows;
  pastix_int_t   nnz;
  int mpid;

  *rhs = NULL;

  MPI_Comm_rank(pastix_comm,&mpid);

  if ( mpid ==0 ||
       driver_type == LAPLACIAN ||
       driver_type == CSCD ||
       driver_type == FDUP_DIST ||
       driver_type == MMD)
    {
      switch(driver_type)
        {
#ifndef USE_NOFORTRAN
        case RSA:
          printf("driver: RSA file: %s\n", filename);
          z_rsaRead(filename,
                  ncol, &nrows, &nnz,
                  colptr, rows, values,
                  type, rhstype);
          break;
#endif /* USE_NOFORTRAN */
        case CHB:
          printf("driver: CHB file: %s\n", filename);
          z_chbRead(filename,
                  ncol, &nrows, &nnz,
                  colptr, rows, values,
                  type, rhstype, rhs);
          break;
        case CCC:
          printf("driver: CCC file: %s\n", filename);
          z_cccRead(filename,
                  ncol, &nrows, &nnz,
                  colptr, rows, values,
                  type, rhstype);
          break;
        case RCC:
          printf("driver: RCC file: %s\n", filename);
          z_cccRead(filename,
                  ncol, &nrows, &nnz,
                  colptr, rows, values,
                  type, rhstype);
          (*type)[0]='R';
          break;
        case OLAF:
          printf("driver: OLAF file: %s\n", filename);
          z_olafRead(filename,
                   ncol, &nrows, &nnz,
                   colptr, rows, values,
                   type, rhstype,
                   rhs);
          break;
        case PEER:
          printf("driver: PEER file: %s\n", filename);
          z_peerRead(filename,
                   ncol, &nrows, &nnz,
                   colptr, rows, values,
                   type, rhstype,
                   rhs);
      /* peerRead2("rsaname", ncol, &nrows, &nnz, colptr, rows, values, type, rhstype, rhs); */
          break;
        case HB:
          printf("driver: HB file: %s\n", filename);
          z_HBRead(filename,
                 ncol, &nrows, &nnz,
                 colptr, rows, values,
                 type, rhstype);
          break;
        case THREEFILES:
          printf("driver: 3files file: %s\n", filename);
          z_threeFilesRead(filename,
                         ncol, &nrows, &nnz,
                         colptr, rows, values,
                         type, rhstype);
          break;
        case MM:
          printf("driver: MatrixMarket file: %s\n", filename);
          z_MatrixMarketRead(filename,
                           ncol, &nrows, &nnz,
                           colptr, rows, values,
                           type, rhstype);
          break;
        case MMD:
          printf("driver: DistributedMatrixMarket file: %s\n", filename);
          z_DistributedMatrixMarketRead(filename,
                                      ncol, &nrows, &nnz,
                                      colptr, rows, values, loc2glob,
                                      type, rhstype);
          break;
        case PETSCS:
        case PETSCU:
        case PETSCH:
          printf("driver: PETSc file: %s\n", filename);
          z_PETScRead(filename,
                    ncol, &nrows, &nnz,
                    colptr, rows, values,
                    type, rhstype);
          if (driver_type == PETSCS) *type[1] = 'S';
          if (driver_type == PETSCH) *type[1] = 'H';
          break;
        case CSCD:
          printf("driver CSCdt file: %s\n", filename);
          z_cscdRead(filename,
                   colptr, rows, loc2glob, values,
                   rhs, ncol, &nnz,
                   pastix_comm);

          *type = (char *) malloc(4*sizeof(char));
          sprintf(*type,"RSA");
          break;
        case LAPLACIAN:
          if (mpid == 0)
            printf("driver Laplacian\n");
          z_genlaplacian(*ncol, &nnz, colptr, rows, values, rhs, type, rhstype);
          return EXIT_SUCCESS;
          break;
#ifdef FDUPROS
        case FDUP:
          printf("driver: FDupros file: %s\n",filename);
          z_driverFdupros(filename,
                        ncol, &nrows, &nnz,
                        colptr, rows, values,
                        rhs,
                        type, rhstype);
          break;
        case FDUP_DIST:
          printf("driver: FDupros file: %s\n",filename);
          z_driverFdupros_dist(filename,
                             ncol, &nrows, &nnz,
                             colptr, rows, loc2glob, values,
                             rhs,
                             type, rhstype,
                             pastix_comm);
          free(*rhs);
          *rhs = NULL;
          break;
#endif
        default:
          printf("default driver\n");
          z_rsaRead("rsaname", ncol, &nrows, &nnz, colptr, rows, values, type, rhstype);
          break;
          /*
           printf("matrix driver unknown\n");
           EXIT(MOD_SOPALIN,NOTIMPLEMENTED_ERR);
           */
        }
    }
#ifndef TYPE_COMPLEX
  if (*type)
    if ((*type)[1] == 'H')
      (*type)[1] = 'S';
#endif

    /* read RHS file */
  if (mpid == 0)
    {
      FILE *file;
      char filename2[256];
      pastix_int_t i;
      double re;

      sprintf(filename2,"%s.rhs",filename);
      fprintf(stderr,"open RHS file : %s\n",filename2);
      file = fopen(filename2,"r");
      if (file==NULL)
        {
          fprintf(stderr,"cannot load %s\n", filename2);
        }
      else
        {
          *rhs = (pastix_complex64_t *) malloc((*ncol)*sizeof(pastix_complex64_t));
          for (i = 0; i < *ncol; i++)
            {
              (*rhs)[i] = 0.0;
              if (1 != fscanf(file,"%lg\n", &re))
                {
                  fprintf(stderr, "ERROR: reading rhs(%ld)\n", (long int)i);
                  exit(1);
                }
              (*rhs)[i] = (pastix_complex64_t)re;
            }
          fclose(file);
        }
    }

  if (*rhs == NULL  && ( mpid == 0 ||
                         driver_type == LAPLACIAN) &&
      driver_type != CSCD && driver_type != FDUP_DIST && driver_type != MMD)
    {
      *rhs = (pastix_complex64_t *) malloc((*ncol)*sizeof(pastix_complex64_t));
      pastix_int_t i,j;
      for (i = 0; i < *ncol; i++)
        (*rhs)[i] = 0.0;

#ifdef TYPE_COMPLEX
      fprintf(stdout, "Setting right-hand-side member such as X[i] = i + i*I\n");
#else
      fprintf(stdout, "Setting right-hand-side member such as X[i] = i\n");
#endif
      for (i = 0; i < *ncol; i++)
        {
          for (j = (*colptr)[i] -1; j < (*colptr)[i+1]-1; j++)
            {
#ifdef TYPE_COMPLEX
              (*rhs)[(*rows)[j]-1] += (i+1 + I*(i+1))*(*values)[j];
#else
              (*rhs)[(*rows)[j]-1] += (i+1)*(*values)[j];
#endif
              if (MTX_ISSYM((*type)) && i != (*rows)[j]-1)
                {
#ifdef TYPE_COMPLEX
                  (*rhs)[i] += ((*rows)[j] + (*rows)[j] * I)*(*values)[j];
#else
                  (*rhs)[i] += ((*rows)[j])*(*values)[j];
#endif
                }
            }
        }
    }
  if (driver_type == CSCD || driver_type == FDUP_DIST || driver_type == MMD)
    {
      pastix_int_t i,j;
      pastix_int_t N;
      pastix_int_t send[2], recv[2];
      int comm_size;
      MPI_Comm_size(pastix_comm,&comm_size);
      send[0] = *ncol;
      send[1] = 0;
      if (*ncol != 0 && *rhs == NULL)
        send[1] = 1;
      MPI_Allreduce(send, recv, 2, PASTIX_MPI_INT, MPI_SUM, pastix_comm);
      N = recv[0];
      if (recv[1] > 0)
        {
          pastix_complex64_t *RHS;
          pastix_complex64_t *RHS_recv;
          RHS  = (pastix_complex64_t *) malloc((N)*sizeof(pastix_complex64_t));

          for (i = 0; i < N; i++)
            (RHS)[i] = 0.0;
          if (mpid == 0)
            fprintf(stdout, "Setting RHS such as X[i] = i\n");
          for (i = 0; i < *ncol; i++)
            {
              for (j = (*colptr)[i] -1; j < (*colptr)[i+1]-1; j++)
                {
                  (RHS)[(*rows)[j]-1] += ((*loc2glob)[i])*(*values)[j];
                  if (MTX_ISSYM((*type)) && i != (*rows)[j]-1)
                    (RHS)[(*loc2glob)[i]-1] += ((*rows)[j])*(*values)[j];
                }
            }
          RHS_recv  = (pastix_complex64_t *) malloc((N)*sizeof(pastix_complex64_t));
          MPI_Allreduce(RHS, RHS_recv, N, PASTIX_MPI_FLOAT, MPI_SUM, pastix_comm);
          free(RHS);
          *rhs = (pastix_complex64_t *) malloc((*ncol)*sizeof(pastix_complex64_t));

          for (i = 0; i < *ncol; i++)
            (*rhs)[i] = RHS_recv[(*loc2glob)[i]-1];
        }
    }

  if ( driver_type != LAPLACIAN && driver_type != CSCD &&
       driver_type != FDUP_DIST && driver_type != MMD)
    {
      MPI_Bcast(ncol,1,PASTIX_MPI_INT,0,pastix_comm);
      MPI_Bcast(&nnz,1,PASTIX_MPI_INT,0,pastix_comm);

      if (mpid!=0)
        {
          *colptr = (pastix_int_t *)   malloc((*ncol+1)*sizeof(pastix_int_t));
          *rows   = (pastix_int_t *)   malloc(nnz*sizeof(pastix_int_t));
          *values = (pastix_complex64_t *) malloc(nnz*sizeof(pastix_complex64_t));
          *rhs    = (pastix_complex64_t *) malloc((*ncol)*sizeof(pastix_complex64_t));
          *type   = (char *)  malloc(4*sizeof(char));
        }

      MPI_Bcast(*colptr, *ncol+1, PASTIX_MPI_INT,   0, pastix_comm);
      MPI_Bcast(*rows,    nnz,    PASTIX_MPI_INT,   0, pastix_comm);
      MPI_Bcast(*values,  nnz,    PASTIX_MPI_FLOAT, 0, pastix_comm);
      MPI_Bcast(*rhs,    *ncol,   PASTIX_MPI_FLOAT, 0, pastix_comm);
      MPI_Bcast(*type,    4,      MPI_CHAR,         0, pastix_comm);
  }

  return EXIT_SUCCESS;
}

/*
 * Function: z_read_matrix
 *
 * Reads a matrix from a file in the format given by
 * driver_type (see <driver_type_enum>).
 *
 * Parameters:
 *   filename    - Name of the file to read from.
 *   ncol        - Number of column in the matrix (output).
 *   colptr      - Indexes in rows and avals of first element of
 *                 each column of the matrix.(output)
 *   rows        - Row of each element of the matrix.
 *   values      - Values of each element of the matrix.
 *   type        - type of the matrix.
 *   rhstype     - type of the right and side.
 *   driver_type - driver to use to read the matrix.
 *   pastix_comm - MPI communicator containing all processes wich
 *                 call z_read_matrix.
 */
int z_read_matrix(char            *filename,    pastix_int_t    *ncol,
                pastix_int_t   **colptr,      pastix_int_t   **rows,
                pastix_complex64_t **values,      pastix_complex64_t **rhs,
                char           **type,        char           **rhstype,
                driver_type_t    driver_type, MPI_Comm         pastix_comm)
{
  pastix_int_t  *loc2glob;
  int mpid;

  MPI_Comm_rank(pastix_comm,&mpid);
  {
    int ret;
    if (EXIT_SUCCESS != ( ret = read_matrix_common(filename,
                                                   ncol, colptr, rows,
                                                   &loc2glob, values, rhs,
                                                   type, rhstype, driver_type,
                                                   pastix_comm)))
      return ret;
  }

  /* if (driver_type == CSCD || driver_type == FDUP_DIST || driver_type == MMD) */
  /*   { */
  /*     pastix_int_t    gn; */
  /*     pastix_int_t   *gcolptr; */
  /*     pastix_int_t   *grow; */
  /*     pastix_complex64_t *gavals; */
  /*     pastix_complex64_t *grhs; */

  /*     PASTIX_EXTERN(cscd2csc)(*ncol,*colptr,  *rows, *values, *rhs,  NULL, NULL, */
  /*                             &gn,  &gcolptr, &grow, &gavals, &grhs, NULL, NULL, */
  /*                             loc2glob, pastix_comm, 1); */

  /*     free(*colptr); */
  /*     free(*rows); */
  /*     free(*values); */
  /*     free(*rhs); */
  /*     free(loc2glob); */
  /*     *ncol   = gn; */
  /*     *colptr = gcolptr; */
  /*     *rows   = grow; */
  /*     *values = gavals; */
  /*     *rhs    = grhs; */

  /*   } */

  return EXIT_SUCCESS;
}


/*
 * Function: z_dread_matrix
 *
 * Reads a matrix from a file in the format given by driver_type
 * (see <driver_type_enum>) and distribute the matrix on the MPI processors.
 *
 * Parameters:
 *   filename    - Name of the file to read from.
 *   ncol        - Number of column in the matrix (output).
 *   colptr      - Indexes in rows and avals of first element of each column of
 *                 the matrix.(output)
 *   rows        - Row of each element of the matrix.
 *   loc2glob    - Local to global column number indirection.
 *   values      - Values of each element of the matrix.
 *   type        - type of the matrix.
 *   rhstype     - type of the right and side.
 *   driver_type - driver to use to read the matrix.
 *   pastix_comm - MPI communicator containing all processes wich call
 *                 <z_dread_matrix>.
 */
int z_dread_matrix(char            *filename,    pastix_int_t    *ncol,
                 pastix_int_t   **colptr,      pastix_int_t   **rows,
                 pastix_int_t   **loc2glob,
                 pastix_complex64_t **values,      pastix_complex64_t **rhs,
                 char           **type,        char           **rhstype,
                 driver_type_t    driver_type, MPI_Comm         pastix_comm)
{
  int mpid;

  MPI_Comm_rank(pastix_comm,&mpid);
  {
    int ret;
    if (EXIT_SUCCESS != ( ret = read_matrix_common(filename,
                                                   ncol, colptr, rows,
                                                   loc2glob, values, rhs,
                                                   type, rhstype, driver_type,
                                                   pastix_comm)))
        return ret;
  }

  /* if (driver_type != CSCD && driver_type != FDUP_DIST && driver_type != MMD) */
  /*   { */

  /*     pastix_int_t    lN; */
  /*     pastix_int_t   *lcolptr; */
  /*     pastix_int_t   *lrow; */
  /*     pastix_complex64_t *lavals; */
  /*     pastix_complex64_t *lrhs; */

  /*     PASTIX_EXTERN(csc_dispatch)(*ncol, *colptr,  *rows, *values, *rhs,  NULL, NULL, */
  /*                                 &lN,   &lcolptr, &lrow, &lavals, &lrhs, NULL, */
  /*                                 loc2glob, CSC_DISP_SIMPLE, MPI_COMM_WORLD); */

  /*     memFree_null(*colptr); */
  /*     memFree_null(*rows); */
  /*     memFree_null(*values); */
  /*     memFree_null(*rhs); */

  /*     *ncol   = lN; */
  /*     *colptr = lcolptr; */
  /*     *rows   = lrow; */
  /*     *values = lavals; */
  /*     *rhs    = lrhs; */
  /*   } */

  return EXIT_SUCCESS;
}


/*
 * structure: couple_
 *
 * Structure used to sort couples.
 */
typedef struct couple_
{
  pastix_int_t i,j;
} couple;

/*
 * Function: z_comparcouple
 *
 * Compares 2 couples (i,j).
 *
 * Parameters:
 *   a - one couple
 *   b - one other couple
 *
 * Returns:
 *  -1 - if a_i < b_i
 *   1 - if a_i > b_i
 *  -1 - if a_i = b_i and a_j < b_j
 *   1 - else
 */
int z_comparcouple(const void *a, const void *b)
{
  couple const * const aa=(const couple*) a;
  couple const * const bb=(const couple*) b;

  if (aa->i < bb->i)
  {
    return -1;
  }
  else
  {
    if (aa->i > bb->i)
    {
      return 1;
    }
    else
    {
      if (aa->j < bb->j)
      {
        return -1;
      }
      else
      {
        return 1;
      }
    }
  }
}

/*
 * Function: z_checkStrucSym
*
 * Check if unsymmetric matrix has a symmetric pattern, if not correct it.
*
 * Parameters:
 *   n      - size of the matrix
 *   nz     - number of elements
 *   colptr - Index of first element of each column in *row* and *avals*
 *   row    - row of each element
 *   avals  - value of each element
 */
void z_checkStrucSym(pastix_int_t     n,
                   pastix_int_t    *nz,
                   pastix_int_t   **colptr,
                   pastix_int_t   **row,
                   pastix_complex64_t **avals)
{
  pastix_int_t    itercol;
  pastix_int_t    iterval;
  pastix_int_t    iterval2;
  pastix_int_t    nbrlostelt=0; /* Number of element to have a symmetric pattern */
  double          maxvalue=0;
  couple         *tablostelt=NULL;
  pastix_int_t    tempnz;
  pastix_int_t   *tempcol;
  pastix_int_t   *temprow;
  pastix_complex64_t *tempval;
  pastix_int_t    iterlost;

  for (itercol=0; itercol<n;itercol++)
  {
    for (iterval=(*colptr)[itercol]-1; iterval<(*colptr)[itercol+1]-1; iterval++)
    {
      if ((*row)[iterval]-1 != itercol)
      {
        const pastix_int_t rowidx=(*row)[iterval]-1;
        pastix_int_t find=0;

        for (iterval2=(*colptr)[rowidx]-1; iterval2<(*colptr)[rowidx+1]-1;iterval2++)
        {
          if ((*row)[iterval2]-1 == itercol)
          {
            find=1;
            break;
          }
        }
        if (find == 0)
        {
          if (ABS_FLOAT((*avals)[iterval]) > maxvalue)
            maxvalue = ABS_FLOAT((*avals)[iterval]);

          nbrlostelt++;
        }

      }
    }
  }

  fprintf(stderr, "nbrlostelt %ld maxvalue %e \n", (long) nbrlostelt, maxvalue);

  if (nbrlostelt!=0) {
      /* repair csc to have symmetric pattern */
      if (!(tablostelt = (couple *) malloc(nbrlostelt*sizeof(couple)))) {
          fprintf(stderr, "Error in allocation of tablostelt\n");
          exit(-1);
      }

    nbrlostelt=0;

    for (itercol=0; itercol<n; itercol++)
    {
      for (iterval=(*colptr)[itercol]-1; iterval<(*colptr)[itercol+1]-1; iterval++)
      {
        if ((*row)[iterval]-1 != itercol)
        {
          const pastix_int_t rowidx = (*row)[iterval]-1;
          pastix_int_t find=0;

          for (iterval2=(*colptr)[rowidx]-1; iterval2<(*colptr)[rowidx+1]-1; iterval2++)
          {
            if ((*row)[iterval2]-1 == itercol)
            {
              find=1;
              break;
            }
          }
          if (find == 0)
          {
            tablostelt[nbrlostelt].i = (*row)[iterval]-1;
            tablostelt[nbrlostelt].j = itercol;
            nbrlostelt++;
          }
        }
      }
    }
    /* sort tablostelt */
    qsort(tablostelt,nbrlostelt,sizeof(couple),z_comparcouple);

    /* rebuild good format */
    tempnz = (*nz)+nbrlostelt;
    if (!(tempcol = (pastix_int_t *) malloc(n*sizeof(pastix_int_t))))
        MALLOC_ERROR("tempcol");
    if (!(temprow = (pastix_int_t *) malloc(tempnz*sizeof(pastix_int_t))))
        MALLOC_ERROR("temprow");
    if (!(tempval = (pastix_complex64_t *) malloc(tempnz*sizeof(pastix_complex64_t))))
        MALLOC_ERROR("tempval");

    iterlost = 0;
    tempcol[0] = (*colptr)[0];
    for (itercol=0; itercol<n; itercol++)
    {
      for (iterval=(*colptr)[itercol]-1; iterval<(*colptr)[itercol+1]-1; iterval++)
      {
        while ((tablostelt[iterlost].i == itercol) && (tablostelt[iterlost].j < (*row)[iterval]-1))
        {
          /* put elt */
          temprow[iterval+iterlost] = tablostelt[iterlost].j+1;
          tempval[iterval+iterlost] = 0;
          iterlost++;
        }
        temprow[iterval+iterlost] = (*row)[iterval];
        tempval[iterval+iterlost] = (*avals)[iterval];
      }

      while (tablostelt[iterlost].i == itercol)
      {
        /* put elt */
        temprow[iterval+iterlost] = tablostelt[iterlost].j+1;
        tempval[iterval+iterlost] = 0;
        iterlost++;
      }
      tempcol[itercol+1] = (*colptr)[itercol+1]+iterlost;
    }
    *nz=tempnz;
    free((*colptr));*colptr = NULL;
    free((*row));   *row    = NULL;
    free((*avals)); *avals  = NULL;
    *colptr=tempcol;
    *row=temprow;
    *avals=tempval;
  }
}
