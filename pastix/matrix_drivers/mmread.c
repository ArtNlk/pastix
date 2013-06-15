/*
 *  File: mmread.c
 *
 *  Interface to MatrixMarket driver writen in <mmio.c>
 *
 *  This driver can read complex and real matrices.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

/* must be before complex definition */
#include "mmio.h"
/* must be after complex definition */
#include "pastix.h"
#include "common_drivers.h"
#include "mmread.h"

#define MIN(x,y) (((x)<(y))?(x):(y))

/*
 *  Function: MatrixMarketRead
 *
 *  Reads a matrix in matrix market format
 *
 *  For more information about matrix market format see mmio.c/mmio.h
 *
 * Parameters:
 *   dirname - Path to the directory containing matrix
 *   Ncol    - Number of columns
 *   Nrow    - Number of rows
 *   Nnzero  - Number of non zeros
 *   col     - Index of first element of each column in *row* and *val*
 *   row     - Row of eah element
 *   val     - Value of each element
 *   Type    - Type of the matrix
 *   RhsType - Type of the right-hand-side.
 *
 */
void MatrixMarketRead(char const      *filename,
                      pastix_int_t    *Ncol,
                      pastix_int_t    *Nrow,
                      pastix_int_t    *Nnzero,
                      pastix_int_t   **col,
                      pastix_int_t   **row,
                      pastix_float_t **val,
                      char           **Type,
                      char           **RhsType)
{

  FILE * file;
  pastix_int_t * tempcol;
  pastix_int_t iter,baseval;
  pastix_int_t * temprow;
  pastix_float_t * tempval;
  pastix_int_t total;
  pastix_int_t tmp;
  pastix_int_t pos;
  pastix_int_t limit;
  MM_typecode matcode;
  int tmpncol,tmpnrow,tmpnnzero;

  *Type = (char *) malloc(4*sizeof(char));
  *RhsType = (char *) malloc(1*sizeof(char));
  (*RhsType)[0] = '\0';

  file = fopen (filename,"r");
  if (file==NULL)
  {
    fprintf(stderr,"cannot load %s\n", filename);
    exit(-1);
  }

  if (mm_read_banner(file, &matcode) != 0)
  {
    fprintf(stderr,"Could not process Matrix Market banner.\n");
    exit(1);
  }

#ifdef    TYPE_COMPLEX
  (*Type)[0] = 'C';
  if (!mm_is_complex(matcode))
  {
    fprintf(stderr, "\nWARNING : Matrix should be complex. Imaginary part will be 0.\n\n");
  }
#else  /* TYPE_COMPLEX */
  (*Type)[0] = 'R';
  if (mm_is_complex(matcode))
  {
    fprintf(stderr, "\nWARNING : Matrix should not be complex. Only real part will be taken.\n\n");
  }
#endif /* TYPE_COMPLEX */

  (*Type)[1] = 'U';
  if (mm_is_symmetric(matcode))
  {
    (*Type)[1] = 'S';
  }
  else {
    if (mm_is_hermitian(matcode))
    {
      (*Type)[1] = 'H';
    }
  }
  (*Type)[2] = 'A';
  (*Type)[3] = '\0';
  /* find out size of sparse matrix .... */

  if (mm_read_mtx_crd_size(file, &tmpnrow, &tmpncol, &tmpnnzero) !=0)
    exit(1);

  *Ncol = tmpncol;
  *Nrow = tmpnrow;
  *Nnzero = tmpnnzero;

  /* Allocation memoire */
  tempcol = (pastix_int_t *) malloc((*Nnzero)*sizeof(pastix_int_t));
  temprow = (pastix_int_t *) malloc((*Nnzero)*sizeof(pastix_int_t));
  tempval = (pastix_float_t *) malloc((*Nnzero)*sizeof(pastix_float_t));

  if ((tempcol==NULL) || (temprow == NULL) || (tempval == NULL))
  {
    fprintf(stderr, "MatrixMarketRead : Not enough memory (Nnzero %ld)\n",(long)*Nnzero);
    exit(-1);
  }

  /* Remplissage */
  {
    long temp1,temp2;
    double re,im;
    im = 0.0;

    if (mm_is_complex(matcode))
    {
      for (iter=0; iter<(*Nnzero); iter++)
      {
        if (4 != fscanf(file,"%ld %ld %lg %lg\n", &temp1, &temp2, &re, &im))
        {
          fprintf(stderr, "ERROR: reading matrix (line %ld)\n",
                  (long int)iter);
          exit(1);
        }

        temprow[iter]=(pastix_int_t)temp1;
        tempcol[iter]=(pastix_int_t)temp2;
#ifdef    TYPE_COMPLEX
        tempval[iter]=(pastix_float_t)(re+im*I);
#else  /* TYPE_COMPLEX */
        tempval[iter]=(pastix_float_t)(re);
#endif /* TYPE_COMPLEX */
      }
    }
    else
    {
      for (iter=0; iter<(*Nnzero); iter++)
      {
        if (3 != fscanf(file,"%ld %ld %lg\n", &temp1, &temp2, &re))
        {
          fprintf(stderr, "ERROR: reading matrix (line %ld)\n",
                  (long int)iter);
          exit(1);
        }
        temprow[iter]=(pastix_int_t)temp1;
        tempcol[iter]=(pastix_int_t)temp2;
#ifdef    TYPE_COMPLEX
        tempval[iter]=(pastix_float_t)(re+im*I);
#else  /* TYPE_COMPLEX */
        tempval[iter]=(pastix_float_t)(re);
#endif /* TYPE_COMPLEX */
      }
    }
  }

  (*col) = (pastix_int_t *) malloc((*Nrow+1)*sizeof(pastix_int_t));
  memset(*col,0,(*Nrow+1)*sizeof(pastix_int_t));
  (*row) = (pastix_int_t *) malloc((*Nnzero)*sizeof(pastix_int_t));
  memset(*row,0,(*Nnzero)*sizeof(pastix_int_t));
  (*val) = (pastix_float_t *) malloc((*Nnzero)*sizeof(pastix_float_t));
  if (((*col)==NULL) || ((*row) == NULL) || ((*val) == NULL))
  {
    fprintf(stderr, "MatrixMarketRead : Not enough memory (Nnzero %ld)\n",(long)*Nnzero);
    exit(-1);
  }

  /* Detection de la base */
  baseval = 1;
  for(iter=0; iter<(*Nnzero); iter++)
    baseval = MIN(baseval, tempcol[iter]);
  if (baseval == 0)
  {
    for(iter=0; iter<(*Nnzero); iter++)
    {
      tempcol[iter]++;
      temprow[iter]++;
    }
  }

  for (iter = 0; iter < (*Nnzero); iter ++)
  {
    (*col)[tempcol[iter]-1]++;
  }

  baseval=1; /* Attention on base a 1 */
  total = baseval;

  for (iter = 0; iter < (*Ncol)+1; iter ++)
  {
    tmp = (*col)[iter];
    (*col)[iter]=total;
    total+=tmp;
  }

  for (iter = 0; iter < (*Nnzero); iter ++)
  {

    pos = (*col)[tempcol[iter]-1]-1;
    limit = (*col)[tempcol[iter]]-1;
    while((*row)[pos] != 0 && pos < limit)
    {
      pos++;
    }
    if (pos == limit)
      fprintf(stderr, "Erreur de lecture\n");

    (*row)[pos] = temprow[iter];
    (*val)[pos] = tempval[iter];
  }

  memFree_null(tempval);
  memFree_null(temprow);
  memFree_null(tempcol);
}
