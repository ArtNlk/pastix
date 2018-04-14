/**
 *
 * @file z_refine_functions.h
 *
 * PaStiX refinement functions implementations.
 *
 * @copyright 2015-2017 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/

#ifndef _z_refine_functions_h_
#define _z_refine_functions_h_

struct z_solver
{
    pastix_int_t    (* getN   )   (pastix_data_t *);
    pastix_fixdbl_t (* getEps )   (pastix_data_t *);
    pastix_int_t    (* getImax)   (pastix_data_t *);
    pastix_int_t    (* getRestart)(pastix_data_t *);

    void* (*malloc)(size_t);
    void  (*free)(void*);

    void   (*output_oneiter)(double, double, double, pastix_int_t);
    void   (*output_final)( pastix_data_t *, pastix_complex64_t, pastix_int_t,
                            double, void*, pastix_complex64_t*);

    void   (*scal)( pastix_int_t, pastix_complex64_t, pastix_complex64_t * );
    pastix_complex64_t (*dot) ( pastix_int_t, const pastix_complex64_t *, const pastix_complex64_t * );
    void   (*copy)( pastix_int_t, const pastix_complex64_t *, pastix_complex64_t * );
    void   (*axpy)( pastix_int_t, pastix_complex64_t, const pastix_complex64_t *, pastix_complex64_t *);
    void   (*spmv)( pastix_data_t *, pastix_complex64_t, const pastix_complex64_t *, pastix_complex64_t, pastix_complex64_t * );
    void   (*spsv)( pastix_data_t *, pastix_complex64_t * );
    double (*norm)( pastix_int_t, const pastix_complex64_t * );
    void   (*gemv)( pastix_int_t, pastix_int_t,
                    pastix_complex64_t, const pastix_complex64_t *, pastix_int_t,
                    const pastix_complex64_t *, pastix_complex64_t, pastix_complex64_t *);
};

void z_Pastix_Solver(struct z_solver *);

void z_gmres_smp   ( pastix_data_t *pastix_data, void *x, void *b );
void z_grad_smp    ( pastix_data_t *pastix_data, void *x, void *b );
void z_pivot_smp   ( pastix_data_t *pastix_data, void *x, void *b );
void z_bicgstab_smp( pastix_data_t *pastix_data, void *x, void *b );

#endif /* _z_refine_functions_h_ */
