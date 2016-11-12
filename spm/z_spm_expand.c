/**
 *
 * @file z_spm_expand.c
 *
 *  PaStiX spm routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Mathieu Faverge
 * @author Alban Bellot
 * @date 2015-01-01
 *
 * @precisions normal z -> c d s p
 **/
#include "common.h"
#include "spm.h"
#include "z_spm.h"

pastix_spm_t *
z_spmCSCExpand(const pastix_spm_t *spm)
{
    pastix_spm_t       *newspm;
    pastix_int_t        i, j, k, ii, jj, dofi, dofj, col, row, baseval, lda;
    pastix_int_t        diag, height;
    pastix_int_t       *newcol, *newrow, *oldcol, *oldrow, *dofs;
    pastix_complex64_t *newval, *oldval, *oldval2;

    if ( spm->dof == 1 ) {
        return (pastix_spm_t*)spm;
    }

    if ( spm->layout != PastixColMajor ) {
        pastix_error_print( "Unsupported layout\n" );
        return NULL;
    }

    newspm = malloc( sizeof(pastix_spm_t) );
    memcpy( newspm, spm, sizeof(pastix_spm_t) );

    baseval = spmFindBase( spm );
    oldcol = spm->colptr;
    oldrow = spm->rowptr;
    dofs   = spm->dofs;
#if !defined(PRECISION_p)
    oldval = oldval2 = (pastix_complex64_t*)(spm->values);
#endif

    newspm->n = spm->nexp;
    newspm->colptr = newcol = malloc(sizeof(pastix_int_t)*(spm->nexp+1));

    /**
     * First loop to compute the new colptr
     */
    *newcol = baseval;
    for(j=0; j<spm->n; j++, oldcol++)
    {
        diag = 0;
        dofj = (spm->dof > 0 ) ? spm->dof : dofs[j+1] - dofs[j];

        /* Sum the heights of the elements in the column */
        newcol[1] = newcol[0];
        for(k=oldcol[0]; k<oldcol[1]; k++)
        {
            i = oldrow[k-baseval] - baseval;
            dofi = (spm->dof > 0 ) ? spm->dof : dofs[i+1] - dofs[i];
            newcol[1] += dofi;

            diag = (diag || (i == j));
        }

        diag = (diag & (spm->mtxtype != PastixGeneral));
        height = newcol[1] - newcol[0];
        newcol++;

        /* Add extra columns */
        for(jj=1; jj<dofj; jj++, newcol++)
        {
            newcol[1] = newcol[0] + height;

            if ( diag ) {
                newcol[1] -= jj;
            }
        }
    }
    assert( ((spm->mtxtype == PastixGeneral) && ((newcol[0]-baseval) == spm->nnzexp)) ||
            ((spm->mtxtype != PastixGeneral) && ((newcol[0]-baseval) <= spm->nnzexp)) );

    newspm->nnz = newcol[0] - baseval;
    newspm->rowptr = newrow = malloc(sizeof(pastix_int_t)*newspm->nnz);
#if !defined(PRECISION_p)
    newspm->values = newval = malloc(sizeof(pastix_complex64_t)*newspm->nnz);
#endif

    /**
     * Second loop to compute the new rowptr and valptr
     */
    oldcol = spm->colptr;
    oldrow = spm->rowptr;
    newcol = newspm->colptr;
    for(j=0, col=0; j<spm->n; j++, oldcol++)
    {
        /**
         * Backup current position in oldval because we will pick
         * interleaved data inside the buffer
         */
        lda = newcol[1] - newcol[0];
        oldval2 = oldval;

        if ( spm->dof > 0 ) {
            dofj = spm->dof;
            assert( col == spm->dof * j );
        }
        else {
            dofj = dofs[j+1] - dofs[j];
            assert( col == (dofs[j] - baseval) );
        }

        for(jj=0; jj<dofj; jj++, col++, newcol++)
        {
            assert( ((spm->mtxtype == PastixGeneral) && (lda == (newcol[1] - newcol[0]))) ||
                    ((spm->mtxtype != PastixGeneral) && (lda >= (newcol[1] - newcol[0]))) );

            /* Move to the top of the column jj in element (oldcol[j],j) */
            oldval = oldval2;

            for(k=oldcol[0]; k<oldcol[1]; k++)
            {
                i = oldrow[k-baseval] - baseval;

                if ( spm->dof > 0 ) {
                    dofi = spm->dof;
                    row  = spm->dof * i;
                }
                else {
                    dofi = dofs[i+1] - dofs[i];
                    row  = dofs[i] - baseval;
                }

                /* Move to the top of the jj column in the current element */
                oldval += dofi * jj;

                for(ii=0; ii<dofi; ii++, row++)
                {
                    if ( (spm->mtxtype == PastixGeneral) ||
                         (i != j) ||
                         ((i == j) && (row >= col)) )
                    {
                        (*newrow) = row + baseval; newrow++;
#if !defined(PRECISION_p)
                        (*newval) = *oldval; newval++;
#endif
                    }
                    oldval++;
                }
                /* Move to the top of the next element */
                oldval += dofi * (dofj-jj-1);
            }
        }
    }

    newspm->gN      = newspm->n;
    newspm->gnnz    = newspm->nnz;

    newspm->gNexp   = newspm->gN;
    newspm->nexp    = newspm->n;
    newspm->gnnzexp = newspm->gnnz;
    newspm->nnzexp  = newspm->nnz;

    newspm->dof     = 1;
    newspm->dofs    = NULL;
    newspm->layout  = PastixColMajor;

    assert(spm->loc2glob == NULL);//to do

    (void)newval;
    return newspm;
}

pastix_spm_t *
z_spmCSRExpand(const pastix_spm_t *spm)
{
    pastix_spm_t       *newspm;
    pastix_int_t        i, j, k, ii, jj, dofi, dofj, col, row, baseval, lda;
    pastix_int_t        diag, height;
    pastix_int_t       *newcol, *newrow, *oldcol, *oldrow, *dofs;
    pastix_complex64_t *newval, *oldval, *oldval2;

    if ( spm->dof == 1 ) {
        return (pastix_spm_t*)spm;
    }

    if ( spm->layout != PastixColMajor ) {
        pastix_error_print( "Unsupported layout\n" );
        return NULL;
    }

    newspm = malloc( sizeof(pastix_spm_t) );
    memcpy( newspm, spm, sizeof(pastix_spm_t) );

    baseval = spmFindBase( spm );
    oldcol = spm->colptr;
    oldrow = spm->rowptr;
    dofs   = spm->dofs;
#if !defined(PRECISION_p)
    oldval = oldval2 = (pastix_complex64_t*)(spm->values);
#endif

    newspm->n = spm->nexp;
    newspm->rowptr = newrow = malloc(sizeof(pastix_int_t)*(spm->nexp+1));

    /**
     * First loop to compute the new rowptr
     */
    *newrow = baseval;
    for(i=0; i<spm->n; i++, oldrow++)
    {
        diag = 0;
        dofi = (spm->dof > 0 ) ? spm->dof : dofs[i+1] - dofs[i];

        /* Sum the heights of the elements in the rowumn */
        newrow[1] = newrow[0];
        for(k=oldrow[0]; k<oldrow[1]; k++)
        {
            j = oldcol[k-baseval] - baseval;
            dofj = (spm->dof > 0 ) ? spm->dof : dofs[j+1] - dofs[j];
            newrow[1] += dofj;

            diag = (diag || (i == j));
        }

        diag = (diag & (spm->mtxtype != PastixGeneral));
        height = newrow[1] - newrow[0];
        newrow++;

        /* Add extra rowumns */
        for(ii=1; ii<dofi; ii++, newrow++)
        {
            newrow[1] = newrow[0] + height;

            if ( diag ) {
                newrow[1] -= ii;
            }
        }
    }
    assert( ((spm->mtxtype == PastixGeneral) && ((newrow[0]-baseval) == spm->nnzexp)) ||
            ((spm->mtxtype != PastixGeneral) && ((newrow[0]-baseval) <= spm->nnzexp)) );

    newspm->nnz = newrow[0] - baseval;
    newspm->colptr = newcol = malloc(sizeof(pastix_int_t)*newspm->nnz);
#if !defined(PRECISION_p)
    newspm->values = newval = malloc(sizeof(pastix_complex64_t)*newspm->nnz);
#endif

    /**
     * Second loop to compute the new colptr and valptr
     */
    oldcol = spm->colptr;
    oldrow = spm->rowptr;
    newrow = newspm->rowptr;
    for(i=0, row=0; i<spm->n; i++, oldrow++)
    {
        /**
         * Backup current position in oldval because we will pick
         * interleaved data inside the buffer
         */
        lda = newrow[1] - newrow[0];
        oldval2 = oldval;

        if ( spm->dof > 0 ) {
            dofi = spm->dof;
            assert( row == spm->dof * i );
        }
        else {
            dofi = dofs[i+1] - dofs[i];
            assert( row == dofs[i] - baseval );
        }

        for(ii=0; ii<dofi; ii++, row++, newrow++)
        {
            assert( ((spm->mtxtype == PastixGeneral) && (lda == (newrow[1] - newrow[0]))) ||
                    ((spm->mtxtype != PastixGeneral) && (lda >= (newrow[1] - newrow[0]))) );

            /* Move to the top of the rowumn ii in element (oldrow[j],j) */
            oldval = oldval2 + ii;

            for(k=oldrow[0]; k<oldrow[1]; k++)
            {
                j = oldcol[k-baseval] - baseval;

                if ( spm->dof > 0 ) {
                    dofj = spm->dof;
                    col  = spm->dof * j;
                }
                else {
                    dofj = dofs[j+1] - dofs[j];
                    col  = dofs[j] - baseval;
                }

                for(jj=0; jj<dofj; jj++, col++)
                {
                    if ( (spm->mtxtype == PastixGeneral) ||
                         (i != j) ||
                         ((i == j) && (row <= col)) )
                    {
                        (*newcol) = col + baseval; newcol++;
#if !defined(PRECISION_p)
                        (*newval) = *oldval; newval++;
#endif
                    }
                    oldval += dofi;
                }
            }
        }
        /* Move to the top of the next row of elements */
        oldval -= (dofi-1);
    }

    newspm->gN      = newspm->n;
    newspm->gnnz    = newspm->nnz;

    newspm->gNexp   = newspm->gN;
    newspm->nexp    = newspm->n;
    newspm->gnnzexp = newspm->gnnz;
    newspm->nnzexp  = newspm->nnz;

    newspm->dof     = 1;
    newspm->dofs    = NULL;
    newspm->layout  = PastixColMajor;

    assert(spm->loc2glob == NULL);//to do

    (void)newval;
    return newspm;
}

pastix_spm_t *
z_spmIJVExpand(const pastix_spm_t *spm)
{
    pastix_spm_t       *newspm;
    pastix_int_t        i, j, k, ii, jj, dofi, dofj, col, row, baseval;
    pastix_int_t       *newcol, *newrow, *oldcol, *oldrow, *dofs;
    pastix_complex64_t *newval, *oldval;

    if ( spm->dof == 1 ) {
        return (pastix_spm_t*)spm;
    }

    if ( spm->layout != PastixColMajor ) {
        pastix_error_print( "Unsupported layout\n" );
        return NULL;
    }

    newspm = malloc( sizeof(pastix_spm_t) );
    memcpy( newspm, spm, sizeof(pastix_spm_t) );

    baseval = spmFindBase( spm );
    oldcol = spm->colptr;
    oldrow = spm->rowptr;
    dofs   = spm->dofs;
#if !defined(PRECISION_p)
    oldval = (pastix_complex64_t*)(spm->values);
#endif

    /**
     * First loop to compute the size of the vectores
     */
    newspm->n = spm->nexp;
    if (spm->mtxtype == PastixGeneral) {
        newspm->nnz = spm->nnzexp;
    }
    else {
        newspm->nnz = 0;
        for(k=0; k<spm->nnz; k++, oldrow++, oldcol++)
        {
            i = *oldrow - baseval;
            j = *oldcol - baseval;

            if ( spm->dof > 0 ) {
                dofi = spm->dof;
                dofj = spm->dof;
            }
            else {
                dofi = dofs[i+1] - dofs[i];
                dofj = dofs[j+1] - dofs[j];
            }

            if ( i != j ) {
                newspm->nnz += dofi * dofj;
            }
            else {
                assert( dofi == dofj );
                newspm->nnz += (dofi * (dofi+1)) / 2;
            }
        }
        assert( newspm->nnz <= spm->nnzexp );
    }

    newspm->rowptr = newrow = malloc(sizeof(pastix_int_t)*newspm->nnz);
    newspm->colptr = newcol = malloc(sizeof(pastix_int_t)*newspm->nnz);
#if !defined(PRECISION_p)
    newspm->values = newval = malloc(sizeof(pastix_complex64_t)*newspm->nnz);
#endif

    /**
     * Second loop to compute the new colptr and valptr
     */
    oldrow = spm->rowptr;
    oldcol = spm->colptr;
    for(k=0; k<spm->nnz; k++, oldrow++, oldcol++)
    {
        i = *oldrow - baseval;
        j = *oldcol - baseval;

        if ( spm->dof > 0 ) {
            dofi = spm->dof;
            row  = spm->dof * i;
            dofj = spm->dof;
            col  = spm->dof * j;
        }
        else {
            dofi = dofs[i+1] - dofs[i];
            row  = dofs[i];
            dofj = dofs[j+1] - dofs[j];
            col  = dofs[j];
        }

        if ( spm->layout == PastixColMajor ) {
            for(jj=0; jj<dofj; jj++, col++)
            {
                for(ii=0; ii<dofi; ii++, row++, oldval++)
                {
                    if ( (spm->mtxtype == PastixGeneral) ||
                         (i != j) ||
                         ((i == j) && (row >= col)) )
                    {
                        (*newcol) = col + baseval; newcol++;
                        (*newrow) = row + baseval; newrow++;
#if !defined(PRECISION_p)
                        (*newval) = *oldval; newval++;
#endif
                    }
                }
            }
        }
        else {
            for(ii=0; ii<dofi; ii++, row++)
            {
                for(jj=0; jj<dofj; jj++, col++, oldval++)
                {
                    if ( (spm->mtxtype == PastixGeneral) ||
                         (i != j) ||
                         ((i == j) && (row >= col)) )
                    {
                        (*newcol) = col + baseval; newcol++;
                        (*newrow) = row + baseval; newrow++;
#if !defined(PRECISION_p)
                        (*newval) = *oldval; newval++;
#endif
                    }
                }
            }
        }
    }

    newspm->gN      = newspm->n;
    newspm->gnnz    = newspm->nnz;

    newspm->gNexp   = newspm->gN;
    newspm->nexp    = newspm->n;
    newspm->gnnzexp = newspm->gnnz;
    newspm->nnzexp  = newspm->nnz;

    newspm->dof     = 1;
    newspm->dofs    = NULL;
    newspm->layout  = PastixColMajor;

    assert(spm->loc2glob == NULL);//to do

    (void)newval;
    return newspm;
}

pastix_spm_t *
z_spmExpand( const pastix_spm_t *spm )
{
    switch (spm->fmttype) {
    case PastixCSC:
        return z_spmCSCExpand( spm );
    case PastixCSR:
        return z_spmCSRExpand( spm );
    case PastixIJV:
        return z_spmIJVExpand( spm );
    }
    return NULL;
}

