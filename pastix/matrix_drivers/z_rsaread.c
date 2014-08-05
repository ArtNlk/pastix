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
  File: rsaread.c

  Interface for the fortran driver writen in skitf.f
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "z_pastix.h"
#include "common_drivers.h"
#include "z_rsaread.h"


/*
  Function: z_rsaReadHeader

  Interface for <wreadmtc> to read header from file *filename*.

  Parameters:
    filename - Path to the file to read from
    Nrow     - Number of row
    Ncol     - Number of column
    Nnzero   - Number of non zeros
    Type     - Type of the matrix
    RhsType  - Type of the right hand side

*/
void z_rsaReadHeader(char const   *filename,
                   pastix_int_t *Nrow,
                   pastix_int_t *Ncol,
                   pastix_int_t *Nnzero,
                   char         *Type,
                   char         *RhsType)
{
#ifdef USE_NOFORTRAN
  fprintf(stderr, "ERROR: rsaread is a fortran driver.\n\tPlease recompile without -DUSE_NOFORTRAN.\n");
  return;
#else
  int     tmp;
  int    *col=NULL;
  int    *row=NULL;
  char    title[72+1];
  char    key[8+1];
  int     nrhs;
  int     len;
  int     ierr;
  double *val=NULL;
  double *crhs=NULL;
  int     tmpNrow;
  int     tmpNcol;
  int     tmpNnzero;

  len=strlen(filename);
  tmp=0;
  FORTRAN_CALL(wreadmtc)
    (&tmp,&tmp,&tmp,filename,&len,val,row,col,crhs,&nrhs,
     RhsType,&tmpNrow,&tmpNcol,&tmpNnzero,title,key,Type,&ierr);
  if(ierr != 0) {
    fprintf(stderr, "cannot read matrix (job=0)\n");
  }
  *Nrow=(pastix_int_t)tmpNrow;
  *Ncol=(pastix_int_t)tmpNcol;
  *Nnzero=(pastix_int_t)tmpNnzero;
  Type[3]='\0';

  /*ASSERT(*Nrow==*Ncol,MOD_SI);*/
  if ((*Nrow==*Ncol) == 0)
    {
      fprintf(stderr,"ERROR : (*Nrow!=*Ncol)\n");
      exit(EXIT_FAILURE);
    }
#endif /* USE_NOFORTRAN */
}

/*
  Function: z_rsaRead

  Read matrix using wreadmtc fortran driver defined in skitf.f.
  The return matrix is in CSC format,
  Nrow is equal to Ncol or you should get an error.

  Parameters:
    filename - Path to the file to read from
    Nrow     - Number of rows in matrix
    Ncol     - Number of columns in matrix
    Nnzero   - Number of non zros in matrix
    col      - Index of first element of each column in *row* and *val*
    row      - Row of eah element
    val      - Value of each element
    Type     - Type of the matrix
    RhsType  - Type of the right hand side.
 */
void z_rsaRead(char const      *filename,
             pastix_int_t    *Nrow,
             pastix_int_t    *Ncol,
             pastix_int_t    *Nnzero,
             pastix_int_t   **col,
             pastix_int_t   **row,
             pastix_complex64_t **val,
             char           **Type,
             char           **RhsType)
{
#ifdef USE_NOFORTRAN
  fprintf(stderr, "ERROR: rsaread is a fortran driver.\n\tPlease recompile without -DUSE_NOFORTRAN.\n");
  return;
#else
  int     i;
  int     tmp;
  char    title[72+1];
  char    key[8+1];
  int     nrhs;
  int     len;
  int     ierr;
  double *crhs=NULL;
  int     tmpNrow;
  int     tmpNcol;
  int     tmpNnzero;
  int    *tmpcol;
  int    *tmprow;
  double *tmpval;
  int     base;

  *Type = (char *) malloc(4*sizeof(char));
  *RhsType = (char *) malloc(4*sizeof(char));


#ifdef TYPE_COMPLEX
  fprintf(stderr, "\nWARNING: This drivers reads non complex matrices, imaginary part will be 0\n\n");
#endif

  z_rsaReadHeader(filename, Nrow, Ncol, Nnzero, *Type, *RhsType);
  printf("RSA: Nrow=%ld Ncol=%ld Nnzero=%ld\n",(long)*Nrow,(long)*Ncol,(long)*Nnzero);
  tmpNrow=(int)*Nrow;
  tmpNcol=(int)*Ncol;
  tmpNnzero=(int)*Nnzero;

  len=strlen(filename);
  *col=(pastix_int_t*)malloc(((*Nrow)+1)*sizeof(pastix_int_t));
  ASSERT(*col!=NULL,MOD_SI);
  tmpcol=(int*)malloc((tmpNrow+1)*sizeof(int));
  ASSERT(tmpcol!=NULL,MOD_SI);
  *row=(pastix_int_t*)malloc((*Nnzero)*sizeof(pastix_int_t));

  ASSERT(*row!=NULL,MOD_SI);
  tmprow=(int*)malloc(tmpNnzero*sizeof(int));
  ASSERT(tmprow!=NULL,MOD_SI);

  *val=(pastix_complex64_t*)malloc((*Nnzero)*sizeof(pastix_complex64_t));
  tmpval=(double*)malloc((*Nnzero)*sizeof(double));

  ASSERT(*val!=NULL,MOD_SI);
  tmp=2;
  nrhs=0;

  FORTRAN_CALL(wreadmtc)
    (&tmpNrow,&tmpNnzero,&tmp,filename,&len,tmpval,tmprow,tmpcol,crhs,
     &nrhs,*RhsType,&tmpNrow,&tmpNcol,&tmpNnzero,title,key,*Type,&ierr);

  (*RhsType)[0]='\0';
  myupcase(*Type);
  if(ierr != 0) {
    fprintf(stderr, "cannot read matrix (job=2)\n");
  }

  if (tmpcol[0] == 0)
    base = 1;
  else
    base = 0;

  for (i=0;i<tmpNrow+1;i++) (*col)[i]=(pastix_int_t)(tmpcol[i] + base);
  for (i=0;i<tmpNnzero;i++) (*row)[i]=(pastix_int_t)(tmprow[i] + base);
#ifdef TYPE_COMPLEX
  for (i=0;i<tmpNnzero;i++) (*val)[i]=(pastix_complex64_t)(tmpval[i] + I*0);
#else
  for (i=0;i<tmpNnzero;i++) (*val)[i]=(pastix_complex64_t)(tmpval[i]);
#endif
  memFree_null(tmpcol);
  memFree_null(tmprow);
  memFree_null(tmpval);
  *Nrow=(pastix_int_t)tmpNrow;
  *Ncol=(pastix_int_t)tmpNcol;
  *Nnzero=(pastix_int_t)tmpNnzero;
#endif /* USE_NOFORTRAN */
}
