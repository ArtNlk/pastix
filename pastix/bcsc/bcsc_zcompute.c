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
 *              TRANS = 'N' or 'n'   y := alpha*A*x + beta*y
 *              TRANS = 'T' or 't'   y := alpha*A**T*x + beta*y
 *              TRANS = 'C' or 'c'   y := alpha*A**H*x + beta*y
 * x and y are two vectors of size csc->gN,
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
 * @param[in] csc
 *          The PastixGeneral csc.
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
z_bcscGemv(char                trans,
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

    if( trans == 'n' || trans == 'N' )
    {
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
    }
    else if( trans == 't' || trans == 'T' )
    {
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
    }
#if defined(PRECISION_c) || defined(PRECISION_z)
    else if( trans == 'c' || trans == 'C' )
    {
        col = 0;
        for( bloc=0; bloc < bcsc->cscfnbr; bloc++ )
        {
            for( j=0; j < bcsc->cscftab[bloc].colnbr; j++ )
            {
                for( i = bcsc->cscftab[bloc].coltab[j]; i < bcsc->cscftab[bloc].coltab[j+1]; i++ )
                {
//                     yptr[bcsc->rowtab[i]] += alpha * conj( Uvalptr[i] ) * xptr[col];
                    yptr[col] += alpha * conj( Lvalptr[i] ) * xptr[bcsc->rowtab[i]];
                }
                col += 1;
            }
        }
    }
#endif
    else
    {
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
 * @param[in] n
 *          The number of elements in the array A.
 * 
 * @param[out] norm
 *          The norm of the matrix A.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the y vector has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNormMax( void         *values,
               pastix_int_t  n,
               double       *norm )
{
    double temp1;
#if defined(PRECISION_c) || defined(PRECISION_z)
    double temp2;
#endif
    pastix_complex64_t *valptr = values;
    pastix_int_t i;
    
    *norm = 0.;
    for( i=0; i < n; i++ )
    {
#if defined(PRECISION_c) || defined(PRECISION_z)
        temp1 = pow(creal(valptr[i]),2.);
        temp2 = pow(cimag(valptr[i]),2.);
        temp1 = sqrt(temp1 + temp2);
#else
        temp1 = fabs((double)valptr[i]);
#endif
        if(*norm < temp1)
        {
            *norm = temp1;
        }
    }
    
    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_bcsc
 *
 * z_bcscNorm2 - compute the frobenius norm of a general matrix.
 *
 *******************************************************************************
 *
 * @param[in] values
 *          The values array of the matrix.
 *
 * @param[in] n
 *          The number of elements in the array A.
 * 
 * @param[out] norm
 *          The norm of the matrix A.
 *
 *******************************************************************************
 *
 * @return
 *      \retval PASTIX_SUCCESS if the y vector has been computed succesfully,
 *      \retval PASTIX_ERR_BADPARAMETER otherwise.
 *
 *******************************************************************************/
int
z_bcscNorm2( void         *values,
             pastix_int_t  n,
             double       *norm )
{
    double scale = 0.;
    double sum = 1.;
    double temp;
    pastix_complex64_t *valptr = values;
    pastix_int_t i;
    
    for( i=0; i < n; i++ )
    {
#if defined(PRECISION_c) || defined(PRECISION_z)
        temp = fabs(creal(valptr[i]));
#else
        temp = fabs((double)valptr[i]);
#endif
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
#if defined(PRECISION_c) || defined(PRECISION_z)
        temp = fabs(cimag(valptr[i]));
        if(temp != 0.)
        {
            if(scale < temp)
            {
                sum = 1. + sum*pow((double)(scale / temp), 2.);
                scale = temp;
            }else{
                sum = sum + pow((double)(temp / scale), 2.);
            }
        }
#endif
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
 *          The vector r.
 * 
 * @param[in] r2
 *          The vector b.
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
 *      \retval PASTIX_SUCCESS if the y vector has been computed succesfully,
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
    *berr = 0.;
    pastix_int_t i;

    for( i = 0; i < n; i++)
    {
        if( fabs(r1ptr[i])/fabs(r2ptr[i]) > *berr )
            *berr = fabs(r1ptr[i])/fabs(r2ptr[i]);
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