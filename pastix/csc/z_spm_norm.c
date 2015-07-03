/**
 * @file z_spm_norm.c
 *
 *  PaStiX spm computational routines.
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Theophile Terraz
 * @date 2015-06-01
 * @precisions normal z -> c d s
 *
 **/
#include "common.h"
#include "csc.h"
#include "z_spm.h"
#include "frobeniusupdate.h"

/**
 *******************************************************************************
 *
 * @ingroup spm_internal
 *
 * z_spmFrobeniusNorm - Compute the Frobenius norm of the non distributed given
 * spm structure.
 *
 *  ||A|| = sqrt( sum( a_ij ^ 2 ) )
 *
 *******************************************************************************
 *
 * @param[in] spm
 *           The spm from which the norm need to be computed.
 *
 *******************************************************************************
 *
 * @return
 *           The computed norm
 *
 *******************************************************************************/
double
z_spmFrobeniusNorm( const pastix_csc_t *spm )
{
    pastix_int_t i, j, baseval;
    double *valptr = (double*)spm->values;
    double scale = 1.;
    double sumsq = 0.;

    if (spm->mtxtype == PastixGeneral) {
        for(i=0; i <spm->nnz; i++, valptr++) {
            frobenius_update( 1, &scale, &sumsq, valptr );

#if defined(PRECISION_z) || defined(PRECISION_c)
            valptr++;
            frobenius_update( 1, &scale, &sumsq, valptr );
#endif
        }
    }
    else {
        pastix_int_t *colptr = spm->colptr;
        pastix_int_t *rowptr = spm->rowptr;
        int nb;
        baseval = spmFindBase( spm );

        switch( spm->fmttype ){
        case PastixCSC:
            for(i=0; i<spm->n; i++, colptr++) {
                for(j=colptr[0]; j<colptr[1]; j++, rowptr++, valptr++) {
                    nb = ( i == (*rowptr-baseval) ) ? 1 : 2;
                    frobenius_update( nb, &scale, &sumsq, valptr );

#if defined(PRECISION_z) || defined(PRECISION_c)
                    valptr++;
                    frobenius_update( nb, &scale, &sumsq, valptr );
#endif
                }
            }
            break;
        case PastixCSR:
            for(i=0; i<spm->n; i++, rowptr++) {
                for(j=rowptr[0]; j<rowptr[1]; j++, colptr++, valptr++) {
                    nb = ( i == (*colptr-baseval) ) ? 1 : 2;
                    frobenius_update( nb, &scale, &sumsq, valptr );

#if defined(PRECISION_z) || defined(PRECISION_c)
                    valptr++;
                    frobenius_update( nb, &scale, &sumsq, valptr );
#endif
                }
            }
            break;
        case PastixIJV:
            for(i=0; i <spm->nnz; i++, valptr++, colptr++, rowptr++) {
                nb = ( *rowptr == *colptr ) ? 1 : 2;
                frobenius_update( nb, &scale, &sumsq, valptr );

#if defined(PRECISION_z) || defined(PRECISION_c)
                valptr++;
                frobenius_update( nb, &scale, &sumsq, valptr );
#endif
            }
            break;
        default:
            fprintf(stderr, "z_spmFrobeniusNorm: Unknown Format\n");
        }
    }

    return scale * sqrt( sumsq );
}

/**
 *******************************************************************************
 *
 * @ingroup spm_internal
 *
 * z_spmMaxNorm - Compute the Max norm of the non distributed given spm
 * structure.
 *
 *  ||A|| = max( abs(a_ij) )
 *
 *******************************************************************************
 *
 * @param[in] spm
 *           The spm from which the norm need to be computed.
 *
 *******************************************************************************
 *
 * @return
 *           The computed norm
 *
 *******************************************************************************/
double
z_spmMaxNorm( const pastix_csc_t *spm )
{
    pastix_int_t i;
    pastix_complex64_t *valptr = (pastix_complex64_t*)spm->values;
    double tmp, norm = 0.;

    for(i=0; i <spm->nnz; i++, valptr++) {
        tmp = cabs( *valptr );
        norm = norm > tmp ? norm : tmp;
    }

    return norm;
}

/**
 *******************************************************************************
 *
 * @ingroup spm_internal
 *
 * z_spmInfNorm - Compute the Infinite norm of the non distributed given spm
 * structure.
 *
 *  ||A|| = max_i( sum_j(|a_ij|) )
 *
 *******************************************************************************
 *
 * @param[in] spm
 *           The spm from which the norm need to be computed.
 *
 *******************************************************************************
 *
 * @return
 *           The computed norm
 *
 *******************************************************************************/
double
z_spmInfNorm( const pastix_csc_t *spm )
{
    pastix_int_t col, row, i, baseval;
    pastix_complex64_t *valptr = (pastix_complex64_t*)spm->values;
    double norm = 0.;
    double *summcol;
    
    MALLOC_INTERN( summcol, spm->gN, double);
    memset( summcol, 0, spm->gN * sizeof(double) );
    baseval = spmFindBase( spm );

    switch( spm->fmttype ){
    case PastixCSC:
        for( col=0; col < spm->gN; col++ )
        {
            for( i=spm->colptr[col]-baseval; i<spm->colptr[col+1]-baseval; i++ )
            {
                summcol[spm->rowptr[i]-baseval] += cabs( valptr[i] );
            }
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
    #if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
    #endif
            for( col=0; col < spm->gN; col++ )
            {
                for( i=spm->colptr[col]-baseval+1; i<spm->colptr[col+1]-baseval; i++ )
                {
                    summcol[col] += cabs( valptr[i] );
                }
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summcol );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    case PastixCSR:
        for( row=0; row < spm->gN; row++ )
        {
            for( i=spm->rowptr[row]-baseval; i<spm->rowptr[row+1]-baseval; i++ )
            {
                summcol[spm->colptr[i]-baseval] += cabs( valptr[i] );
            }
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
#endif
            for( row=0; row < spm->gN; row++ )
            {
                for( i=spm->rowptr[row]-baseval+1; i<spm->rowptr[row+1]-baseval; i++ )
                {
                    summcol[row] += cabs( valptr[i] );
                }
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summcol );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    case PastixIJV:
        for(i=0; i < spm->nnz; i++)
        {
            summcol[spm->colptr[i]-baseval] += cabs( valptr[i] );
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
#endif
            for(i=0; i < spm->nnz; i++)
            {
                if(spm->rowptr[i] != spm->colptr[i])
                    summcol[spm->rowptr[i]-baseval] += cabs( valptr[i] );
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summcol );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    default:
        memFree_null( summcol );
        return PASTIX_ERR_BADPARAMETER;
    }

    for( i=0; i<spm->gN; i++)
    {
        if(norm < summcol[i])
        {
            norm = summcol[i];
        }
    }
    memFree_null( summcol );

    return norm;
}

/**
 *******************************************************************************
 *
 * @ingroup spm_internal
 *
 * z_spmOneNorm - Compute the One norm of the non distributed given spm
 * structure.
 *
 *  ||A|| = max_j( sum_i(|a_ij|) )
 *
 *******************************************************************************
 *
 * @param[in] spm
 *           The spm from which the norm need to be computed.
 *
 *******************************************************************************
 *
 * @return
 *           The computed norm
 *
 *******************************************************************************/
double
z_spmOneNorm( const pastix_csc_t *spm )
{
    pastix_int_t col, row, i, baseval;
    pastix_complex64_t *valptr = (pastix_complex64_t*)spm->values;
    double norm = 0.;
    double *summrow;
    
    MALLOC_INTERN( summrow, spm->gN, double);
    memset( summrow, 0, spm->gN * sizeof(double) );
    baseval = spmFindBase( spm );

    switch( spm->fmttype ){
    case PastixCSC:
        for( col=0; col < spm->gN; col++ )
        {
            for( i=spm->colptr[col]-baseval; i<spm->colptr[col+1]-baseval; i++ )
            {
                summrow[col] += cabs( valptr[i] );
            }
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
#endif
            for( col=0; col < spm->gN; col++ )
            {
                for( i=spm->colptr[col]-baseval+1; i<spm->colptr[col+1]-baseval; i++ )
                {
                    summrow[spm->rowptr[i]-baseval] += cabs( valptr[i] );
                }
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summrow );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    case PastixCSR:
        for( row=0; row < spm->gN; row++ )
        {
            for( i=spm->rowptr[row]-baseval; i<spm->rowptr[row+1]-baseval; i++ )
            {
                summrow[row] += cabs( valptr[i] );
            }
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
#endif
            for( row=0; row < spm->gN; row++ )
            {
                for( i=spm->rowptr[row]-baseval+1; i<spm->rowptr[row+1]-baseval; i++ )
                {
                    summrow[spm->colptr[i]-baseval] += cabs( valptr[i] );
                }
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summrow );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    case PastixIJV:
        for(i=0; i < spm->nnz; i++)
        {
            summrow[spm->rowptr[i]-baseval] += cabs( valptr[i] );
        }
        switch (spm->mtxtype) {
        case PastixSymmetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
        case PastixHermitian:
#endif
            for(i=0; i < spm->nnz; i++)
            {
                if(spm->rowptr[i] != spm->colptr[i])
                    summrow[spm->colptr[i]-baseval] += cabs( valptr[i] );
            }
            break;
        case PastixGeneral:
            break;
        default:
            memFree_null( summrow );
            return PASTIX_ERR_BADPARAMETER;
        }
        break;

    default:
        memFree_null( summrow );
        return PASTIX_ERR_BADPARAMETER;
    }

    for( i=0; i<spm->gN; i++)
    {
        if(norm < summrow[i])
        {
            norm = summrow[i];
        }
    }
    memFree_null( summrow );

    return norm;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_spm
 *
 * z_spmNorm - Compute the norm of an spm matrix
 *
 *******************************************************************************
 *
 * @param[in] type
 *          = PastixMaxNorm: Max norm
 *          = PastixOneNorm: One norm
 *          = PastixInfNorm: Infinity norm
 *          = PastixFrobeniusNorm: Frobenius norm
 *
 * @param[in] spm
 *          The spm structure describing the matrix.
 *
 *******************************************************************************
 *
 * @return
 *      \retval The norm of the matrix spm
 *
 *******************************************************************************/
double
z_spmNorm( int ntype,
           const pastix_csc_t *spm)
{
    double norm = 0.;

    if(spm == NULL)
    {
        return -1.;
    }

    switch( ntype ) {
    case PastixMaxNorm:
        norm = z_spmMaxNorm( spm );
        break;

    case PastixInfNorm:
        norm = z_spmInfNorm( spm );
        break;

    case PastixOneNorm:
        norm = z_spmOneNorm( spm );
        break;

    case PastixFrobeniusNorm:
        norm = z_spmFrobeniusNorm( spm );
        break;

    default:
        fprintf(stderr, "z_spmNorm: invalid norm type\n");
        return -1.;
    }

    return norm;
}