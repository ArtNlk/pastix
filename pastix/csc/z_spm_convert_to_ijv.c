/**
 *
 * @file z_spm_convert_to_ijv.c
 *
 *  PaStiX csc routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Mathieu Faverge
 * @author Theophile Terraz
 * @date 2015-01-01
 *
 * @precisions normal z -> c d s p
 **/
#include "common.h"
#include "csc.h"

int
z_spmConvertCSC2IJV( int ofmttype, pastix_csc_t *spm )
{
    pastix_int_t *col_ijv;
    pastix_int_t i, j, count, baseval, nnz;

    /*
     * Check the baseval
     */
    baseval = pastix_imin( *(spm->colptr), *(spm->rows) );
    nnz=spm->colptr[spm->gN]-baseval;
    spm->fmttype=PastixIJV;

    col_ijv = malloc(nnz*sizeof(pastix_int_t));

    assert( col_ijv );

    count=0;
    for(i=baseval;i<spm->gN+baseval;i++)
    {
        for(j=spm->colptr[i-baseval];j<spm->colptr[i-baseval+1];j++)
        {
            col_ijv[count]=i;
            count++;
        }
    }

    memFree_null(spm->colptr);
    spm->colptr=col_ijv;

    return PASTIX_SUCCESS;
}

int
z_spmConvertCSR2IJV( int ofmttype, pastix_csc_t *spm )
{
    pastix_int_t *row_ijv;
    pastix_int_t i, j, count, baseval, nnz;

    /*
     * Check the baseval
     */
    baseval = pastix_imin( *(spm->colptr), *(spm->rows) );
    nnz=spm->rows[spm->gN]-baseval;
    spm->fmttype=PastixIJV;

    row_ijv = malloc(nnz*sizeof(pastix_int_t));

    assert( row_ijv );

    count=0;
    for(i=baseval;i<spm->gN+baseval;i++)
    {
        for(j=spm->rows[i-baseval];j<spm->rows[i-baseval+1];j++)
        {
            row_ijv[count]=i;
            count++;
        }
    }

    memFree_null(spm->rows);
    spm->rows  =row_ijv;

    return PASTIX_SUCCESS;
}