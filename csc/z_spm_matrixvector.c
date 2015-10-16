/**
 *
 * @file z_spm_matrixvector.c
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
 * @precisions normal z -> c d s
 **/
#include "common.h"
#include "csc.h"
#include "z_spm.h"

/**
 *******************************************************************************
 *
 * @ingroup pastix_csc
 *
 * z_spmGeCSCv - compute the matrix-vector product:
 *          y = alpha * op( A ) + beta * y
 *
 * A is a PastixGeneral csc, where op( X ) is one of
 *
 *    op( X ) = X  or op( X ) = X' or op( X ) = conjg( X' )
 *
 *  alpha and beta are scalars, and x and y are vectors.
 *
 *******************************************************************************
 *
 * @param[in] trans
 *          Specifies whether the matrix spm is transposed, not transposed or
 *          conjugate transposed:
 *          = PastixNoTrans:   A is not transposed;
 *          = PastixTrans:     A is transposed;
 *          = PastixConjTrans: A is conjugate transposed.
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha
 *
 * @param[in] csc
 *          The PastixGeneral csc.
 *
 * @param[in] x
 *          The vector x.
 *
 * @param[in] beta
 *          beta specifies the scalar beta
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
z_spmGeCSCv(      int                 trans,
                  pastix_complex64_t  alpha,
            const pastix_csc_t       *csc,
            const pastix_complex64_t *x,
                  pastix_complex64_t  beta,
                  pastix_complex64_t *y )
{
    const pastix_complex64_t *valptr = (pastix_complex64_t*)csc->values;
    const pastix_complex64_t *xptr   = x;
    pastix_complex64_t *yptr = y;
    pastix_int_t col, row, i, baseval;

    if ( (csc == NULL) || (x == NULL) || (y == NULL ) )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    if( csc->mtxtype != PastixGeneral )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    baseval = spmFindBase( csc );

    /* first, y = beta*y */
    if( beta == 0. ) {
        memset( yptr, 0, csc->gN * sizeof(pastix_complex64_t) );
    }
    else {
        for( i=0; i<csc->gN; i++, yptr++ ) {
            (*yptr) *= beta;
        }
        yptr = y;
    }

    if( alpha != 0. ) {
        /**
         * PastixNoTrans
         */
        if( trans == PastixNoTrans )
        {
            for( col=0; col < csc->gN; col++ )
            {
                for( i=csc->colptr[col]; i<csc->colptr[col+1]; i++ )
                {
                    row = csc->rowptr[i-baseval]-baseval;
                    yptr[row] += alpha * valptr[i-baseval] * xptr[col];
                }
            }
        }
        /**
         * PastixTrans
         */
        else if( trans == PastixTrans )
        {
            for( col=0; col < csc->gN; col++ )
            {
                for( i=csc->colptr[col]; i<csc->colptr[col+1]; i++ )
                {
                    row = csc->rowptr[i-baseval]-baseval;
                    yptr[col] += alpha * valptr[i-baseval] * xptr[row];
                }
            }
        }
#if defined(PRECISION_c) || defined(PRECISION_z)
        else if( trans == PastixConjTrans )
        {
            for( col=0; col < csc->gN; col++ )
            {
                for( i=csc->colptr[col]; i<csc->colptr[col+1]; i++ )
                {
                    row = csc->rowptr[i-baseval]-baseval;
                    yptr[col] += alpha * conj( valptr[i-baseval] ) * xptr[row];
                }
            }
        }
#endif
        else
        {
            return PASTIX_ERR_BADPARAMETER;
        }
    }

    return PASTIX_SUCCESS;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_csc
 *
 * z_spmSYCSCv - compute the matrix-vector product:
 *          y = alpha * A + beta * y
 *
 * A is a PastixSymmetric csc, alpha and beta are scalars, and x and y are
 * vectors, and A a symm.
 *
 *******************************************************************************
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha
 *
 * @param[in] csc
 *          The PastixSymmetric csc.
 *
 * @param[in] x
 *          The vector x.
 *
 * @param[in] beta
 *          beta specifies the scalar beta
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
z_spmSyCSCv(      pastix_complex64_t  alpha,
            const pastix_csc_t       *csc,
            const pastix_complex64_t *x,
                  pastix_complex64_t  beta,
                  pastix_complex64_t *y )
{
    const pastix_complex64_t *valptr = (pastix_complex64_t*)csc->values;
    const pastix_complex64_t *xptr   = x;
    pastix_complex64_t *yptr = y;
    pastix_int_t col, row, i, baseval;

    if ( (csc == NULL) || (x == NULL) || (y == NULL ) )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    if( csc->mtxtype != PastixSymmetric )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    baseval = spmFindBase( csc );

    /* First, y = beta*y */
    if( beta == 0. ) {
        memset( yptr, 0, csc->gN * sizeof(pastix_complex64_t) );
    }
    else {
        for( i=0; i<csc->gN; i++, yptr++ ) {
            (*yptr) *= beta;
        }
        yptr = y;
    }

    if( alpha != 0. ) {
        for( col=0; col < csc->gN; col++ )
        {
            for( i=csc->colptr[col]; i < csc->colptr[col+1]; i++ )
            {
                row = csc->rowptr[i-baseval]-baseval;
                yptr[row] += alpha * valptr[i-baseval] * xptr[col];
                if( col != row )
                {
                    yptr[col] += alpha * valptr[i-baseval] * xptr[row];
                }
            }
        }
    }

    return PASTIX_SUCCESS;
}

#if defined(PRECISION_c) || defined(PRECISION_z)
/**
 *******************************************************************************
 *
 * @ingroup pastix_csc
 *
 * z_spmHeCSCv - compute the matrix-vector product:
 *          y = alpha * A + beta * y
 *
 * A is a PastixHermitian csc, alpha and beta are scalars, and x and y are
 * vectors, and A a symm.
 *
 *******************************************************************************
 *
 * @param[in] alpha
 *          alpha specifies the scalar alpha
 *
 * @param[in] csc
 *          The PastixHermitian csc.
 *
 * @param[in] x
 *          The vector x.
 *
 * @param[in] beta
 *          beta specifies the scalar beta
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
z_spmHeCSCv(      pastix_complex64_t  alpha,
            const pastix_csc_t       *csc,
            const pastix_complex64_t *x,
                  pastix_complex64_t  beta,
                  pastix_complex64_t *y )
{
    const pastix_complex64_t *valptr = (pastix_complex64_t*)csc->values;
    const pastix_complex64_t *xptr   = x;
    pastix_complex64_t *yptr = y;
    pastix_int_t col, row, i, baseval;

    if ( (csc == NULL) || (x == NULL) || (y == NULL ) )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    if( csc->mtxtype != PastixHermitian )
    {
        return PASTIX_ERR_BADPARAMETER;
    }

    /* First, y = beta*y */
    if( beta == 0. ) {
        memset( yptr, 0, csc->gN * sizeof(pastix_complex64_t) );
    }
    else {
        for( i=0; i<csc->gN; i++, yptr++ ) {
            (*yptr) *= beta;
        }
        yptr = y;
    }

    baseval = spmFindBase( csc );

    if( alpha != 0. ) {
        for( col=0; col < csc->gN; col++ )
        {
            for( i=csc->colptr[col]; i < csc->colptr[col+1]; i++ )
            {
                row=csc->rowptr[i-baseval]-baseval;
                yptr[row] += alpha * valptr[i-baseval] * xptr[col];
                if( col != row )
                    yptr[col] += alpha * conj( valptr[i-baseval] ) * xptr[row];
            }
        }
    }

    return PASTIX_SUCCESS;
}
#endif