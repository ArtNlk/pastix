/**
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Théophile terraz
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
/*
  File: bcsc_zcompute.c

  Functions computing operations on the BCSC.

*/

#include "common.h"
#include "bcsc.h"
#include "math.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscGemv - compute the matrix-vector product y=alpha*A**trans*x+beta*y.
 * A is a PastixGeneral bcsc,
 * trans specifies the operation to be performed as follows:
 *              trans = PastixNoTrans   y := alpha*A*x + beta*y
 *              trans = PastixTrans     y := alpha*A**T*x + beta*y
 *              trans = PastixConjTrans y := alpha*A**H*x + beta*y
 * x and y are two vectors of size n,
 * alpha and beta are scalars.
 *
 *******************************************************************************
 *
 * @param[in] trans
 *          The operation to be performed.
 *
 * @param[in] n
 *          Number of columns of the matrix.
 *
 * @param[in] alpha
 *          A scalar.
 *
 * @param[in] bcsc
 *          The PastixGeneral bcsc.
 *
 * @param[in] x
 *          The vector x.
 *
 * @param[in] beta
 *          A scalar.
 *
 * @param[in,out] y
 *          The vector y.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the y vector has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscGemv(pastix_trans_t      trans,
           pastix_int_t        n, /* size of the matrix */
           pastix_complex64_t  alpha,
           pastix_bcsc_t      *bcsc,
           void               *x,
           pastix_complex64_t  beta,
           void               *y )
{
    pastix_complex64_t *Lvalptr = NULL;
//     pastix_complex64_t *Uvalptr = NULL;
    pastix_complex64_t *yptr    = (pastix_complex64_t*)y;
    pastix_complex64_t *xptr    = (pastix_complex64_t*)x;
    pastix_int_t        bloc, col, i, j;

    if(bcsc==NULL || y==NULL || x== NULL)
    {
        return PASTIX_ERR_BADPARAMETER;
    }
    Lvalptr = bcsc->Lvalues;
//     Uvalptr = bcsc->Uvalues;

    /* first, y = beta*y */
    if( beta != (pastix_complex64_t)0.0 )
    {
        for( col = 0; col < n; col++ )
        {
            yptr[col] *= beta;
        }
    }
    else if( beta == (pastix_complex64_t)0.0 )
    {
        memset(yptr,0.0,n*sizeof(pastix_complex64_t));
    }

    switch (trans) {
    case PastixNoTrans:
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
                    yptr[bcsc->rowtab[i]] += alpha * Lvalptr[i] * xptr[col];
                }
                col += 1;
            }
        }
    break;
    case PastixTrans:
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
//                     yptr[bcsc->rowtab[i]] += alpha * Uvalptr[i] * xptr[col];
                    yptr[col] += alpha * Lvalptr[i] * xptr[bcsc->rowtab[i]];
                }
                col += 1;
            }
        }
    break;
#if defined(PRECISION_c) || defined(PRECISION_z)
    case PastixConjTrans:
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
//                     yptr[bcsc->rowtab[i]] += alpha * conj( Uvalptr[i] ) * xptr[col];
#if defined(PRECISION_z)
                    yptr[col] += alpha * conj( Lvalptr[i] ) * xptr[bcsc->rowtab[i]];
#elseif defined(PRECISION_c)
                    yptr[col] += alpha * conjf( Lvalptr[i] ) * xptr[bcsc->rowtab[i]];
#endif
                }
                col += 1;
            }
        }
    break;
#endif
    default:
        return PASTIX_ERR_BADPARAMETER;
    }

    return PASTIX_SUCCESS;

}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscNormMax - compute the max norm of a general matrix.
 *
 *******************************************************************************
 *
 * @param[in] values
 *          The values array of the matrix.
 *
 * @param[out] norm
 *          The norm of the matrix A.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the norm has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNormMax( pastix_bcsc_t *bcsc,
               double       *norm )
{
    double temp;
    pastix_complex64_t *valptr = bcsc->values;
    pastix_int_t i, j, bloc;

    *norm = 0.;
    for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
    {
        for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
        {
            for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
            {
                temp = cabs(valptr[i]);
                if(*norm < temp)
                {
                    *norm = temp;
                }
            }
            col += 1;
        }
    }

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscNormInf - compute the infinity norm of a general matrix.
 * The infinity norm is equal to the maximum value of the sum of the
 * Absolute values of the elements of each rows.
 *
 *******************************************************************************
 *
 * @param[in] bcsc
 *          The Pastix bcsc.
 *
 * @param[in] n
 *          The size of the matrix.
 *
 * @param[out] norm
 *          The norm of the matrix A.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the norm has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNormInf( pastix_bcsc_t *bcsc,
               pastix_int_t  n,
               double        *norm )
{
    double temp;
    double *summcol;
    pastix_complex64_t *valptr = bcsc->Lvalues;
    int i,j,bloc

    MALLOC_INTERN( summcol, n, double);
    memset( summcol, 0, n * sizeof(double) );
    *norm = 0.;
    col = 0;
    for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
    {
        for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
        {
            for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
            {
                summcol[bcsc->rowtab[i]] += cabs(valptr[i]);
            }
            col += 1;
        }
    }
    switch (bcsc->mtxtype) {
    case PastixSymetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
    case PastixHermitian:
#endif
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]+1; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
                    summcol[col] += cabs(valptr[i]);
                }
                col += 1;
            }
        }
    break;
    case PastixGeneral:
    break;
    default:
        memFree_null( summcol );
        return PASTIX_ERR_BADPARAMETER;
    }

    for( i=0; i<n; i++)
    {
        if(*norm < summcol[i])
        {
            *norm = summcol[i];
        }
    }
    memFree_null( sumcol );

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscNormOne - compute the norm 1 of a general matrix.
 * Norm 1 is equal to the maximum value of the sum of the
 * Absolute values of the elements of each columns.
 *
 *******************************************************************************
 *
 * @param[in] bcsc
 *          The Pastix bcsc.
 *
 * @param[in] n
 *          The size of the matrix.
 *
 * @param[out] norm
 *          The norm of the matrix A.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the norm has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNormOne( pastix_bcsc_t *bcsc,
               pastix_int_t  n,
               double        *norm )
{
    double *summrow;
    pastix_complex64_t *valptr = bcsc->Lvalues;
    int i,j,bloc

    MALLOC_INTERN( summrow, n, double);
    memset( summrow, 0, n * sizeof(double) );
    *norm = 0.;
    col = 0;
    for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
    {
        for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
        {
            for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
            {
                summrow[col] += cabs(valptr[i]);
            }
            col += 1;
        }
    }  
    switch (bcsc->mtxtype) {
    case PastixSymetric:
#if defined(PRECISION_z) || defined(PRECISION_c)
    case PastixHermitian:
#endif
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]+1; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
                    summrow[bcsc->rowtab[i]] += cabs(valptr[i]);
                }
                col += 1;
            }
        }
    break;
    case PastixGeneral:
    break;
    default:
        memFree_null( sumcol );
        return PASTIX_ERR_BADPARAMETER;
    }

    for( i=0; i<n; i++)
    {
        if(*norm < summrow[i])
        {
            *norm = summrow[i];
        }
    }
    memFree_null( summrow );

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscFrobeniusNorm - compute the frobenius norm of a general matrix.
 *
 *******************************************************************************
 *
 * @param[in] bcsc
 *          The Pastix bcsc.
 *
 * @param[out] norm
 *          The norm of the matrix.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the norm has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscFrobeniusNorm( pastix_bcsc_t *bcsc,
                     double       *norm )
{
    double scale = 0.;
    double sum = 1.;
    double temp, nb;
    pastix_complex64_t *valptr = bcsc->values;
    pastix_int_t i, j;

    if (bcsc->mtxtype == PastixGeneral)
    {
        nb = 1.;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
                    if(temp != 0.)
                    {
                        if(scale < temp)
                        {
                            sum = nb + sum*pow((scale / temp), 2.);
                            scale = temp;
                        }else{
                            sum = sum + nb*pow((double)(temp / scale), 2.);
                        }
                    }
                }
                col += 1;
            }
        }
    }else{
        for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
        {
            for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
            {
                temp = cabs(valptr[i]);
                if(rowptr[i] == col)
                {
                    nb = 1.;
                }else{
                    nb = 2.;
                }
                if(temp != 0.)
                {
                    if(scale < temp)
                    {
                        sum = nb + sum*pow((scale / temp), 2.);
                        scale = temp;
                    }else{
                        sum = sum + nb*pow((double)(temp / scale), 2.);
                    }
                }
            }
            col += 1;
        }
    }    
    *norm = scale*sqrt(sum);

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscBerr - Compute the operation $$ berr= max_{i}(\\frac{|r1_{i}|}{|r2_{i}|}) $$.
 *
 *******************************************************************************
 *
 * @param[in] r1
 *          The vector r1.
 *
 * @param[in] r2
 *          The vector r2.
 *
 * @param[in] n
 *          The size of the vectors.
 *
 * @param[out] berr
 *          The returned result.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the berr has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscBerr( void         *r1,
            void         *r2,
            pastix_int_t  n,
            double       *berr )
{
    pastix_complex64_t *r1ptr = (pastix_complex64_t*)r1;
    pastix_complex64_t *r2ptr = (pastix_complex64_t*)r2;
    pastix_complex64_t module1, module2;
    pastix_int_t i;

    if(r1==NULL || r1== NULL)
        return PASTIX_ERR_BADPARAMETER;

    *berr = 0.;

    for( i = 0; i < n; i++)
    {
        module1 = cabs(r1ptr[i]);
        module2 = cabs(r2ptr[i]);
        if( module2 > 0)
            if( module1 / module2 > *berr )
                *berr = module1 / module2;
    }

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscNormErr - Computes the norm 2 of r1 and the norm 2 of r2
 *                 and return the quotient of these two vectors.
 *
 *******************************************************************************
 *
 * @param[in] r1
 *          The vector r1.
 *
 * @param[in] r2
 *          The vector r2.
 *
 * @param[in] n
 *          The size of the vectors.
 *
 * @param[out] err
 *          The returned result.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the err has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNormErr( void         *r1,
               void         *r2,
               pastix_int_t  n,
               double       *err )
{
    double norm2r1;
    double norm2r2;
    double scale = 0.;
    double sum = 1.;
    double temp, nb;

    if(r1==NULL || r2== NULL)
        return PASTIX_ERR_BADPARAMETER;

    for( i = 0; i < n; i++ )
    {
        temp = cabs(r1[i]);
        if(temp != 0.)
        {
            if(scale < temp)
            {
                sum = 1. + sum*pow((scale / temp), 2.);
                scale = temp;
            }else{
                sum = sum + pow((double)(temp / scale), 2.);
            }
        }
    }
    norm2r1 = scale*sqrt(sum);

    scale = 0.;
    sum=1.;
    for( i = 0; i < n; i++ )
    {
        temp = cabs(r2[i]);
        if(temp != 0.)
        {
            if(scale < temp)
            {
                sum = 1. + sum*pow((scale / temp), 2.);
                scale = temp;
            }else{
                sum = sum + pow((double)(temp / scale), 2.);
            }
        }
    }
    norm2r2 = scale*sqrt(sum);

    *err = norm2r1/norm2r2;

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscScal - Multiply a vector by a scalaire x <- alpha*x.
 *
 *******************************************************************************
 *
 * @param[in,out] x
 *          The vector x.
 *
 * @param[in] alpha
 *          The scalar alpha.
 *
 * @param[in] n
 *          The size of the vectors.
 *
 * @param[in] smxnbr
 *          The number of vectors (multi-right-hand-side method).
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the x vector has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscScal( void               *x,
            pastix_complex64_t  alpha,
            pastix_int_t        n,
            pastix_int_t        smxnbr )
{
    pastix_complex64_t *xptr = (pastix_complex64_t*)x;
    pastix_int_t i;

    if(x==NULL)
        return PASTIX_ERR_BADPARAMETER;

    if( alpha == (pastix_complex64_t)0.0 )
    {
        memset(xptr,0.0,smxnbr*n*sizeof(pastix_complex64_t));
        return PASTIX_SUCCESS;
    }

    for( i = 0; i < n*smxnbr; i++)
    {
        *xptr *=alpha;
        xptr ++;
    }

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscAxpy - compute y<-alpha*x+y.
 *
 *******************************************************************************
 *
 * @param[in] alpha
 *          A scalar.
 *
 * @param[in] x
 *          The vector x.
 *
 * @param[in] n
 *          The size of the vectors.
 *
 * @param[in,out] y
 *          The vector y.
 *
 * @param[in] smxnbr
 *          The number of vectors (multi-right-hand-side method).
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the y vector has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscAxpy(pastix_complex64_t  alpha,
           void               *x,
           pastix_int_t        n, /* size of the matrix */
           void               *y,
           pastix_int_t        smxnbr )
{
    pastix_complex64_t *xptr = (pastix_complex64_t*)x;
    pastix_complex64_t *yptr = (pastix_complex64_t*)y;
    pastix_int_t i;

    if(y==NULL || x== NULL)
    {
        return PASTIX_ERR_BADPARAMETER;
    }
    if( alpha == (pastix_complex64_t)0.0 )
    {
        return PASTIX_SUCCESS;
    }

    for(i = 0; i < n*smxnbr; i++)
    {
        *yptr = *yptr + alpha * *xptr;
        yptr++;
        xptr++;
    }

    return PASTIX_SUCCESS;
}


/* Pivot : */
/* r=b-ax */
/* r'=|A||x|+|b| */
/* tmp_berr =  max_i(|r_i|/|r'_i|)*/
/* rberror = ||r||/||b|| */
/* Copy */
/* GEMM */
