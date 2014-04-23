/**
 *
 * @file starpu_zpotrfsp_cpu.c
 *
 *  PaStiX kernel routines for StarPU
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c
 *
 **/
#include <assert.h>
#include <starpu.h>

#include "common.h"
#include "sopalin3d.h"
#include "solver.h"
#include "pastix_zcores.h"
#include "sopalin_acces.h"
#include "starpu_defines.h"
#include "starpu_zsubmit.h"

static pastix_complex64_t zone  =  1.;
static pastix_complex64_t mzone = -1.;
static pastix_complex64_t zzero =  0.;


/**
 *******************************************************************************
 *
 * @ingroup pastix_starpu_kernel
 *
 * starpu_zhetrfsp1d_hetrf_cpu - Computes the LLt factorization of one panel.
 *
 *******************************************************************************
 *
 * @param[in, out] buffers
 *          Array of 2 buffers:
 *            -# L : The pointer to the lower matrix storing the coefficients
 *                   of the panel. Must be of size cblk.stride -by- cblk.width
 *            -# work : Scratch buffers of size SOLV_COEFMAX.
 *
 * @param[in, out] _args
 *          Package of arguments to unpack with starpu_codelet_unpack_args():
 *            -# sopalin_data : common sopalin structure.
 *            -# cblk : Pointer to the structure representing the panel to
 *                      factorize in the cblktab array. Next column blok must
 *                      be accessible through cblk[1].
 *
 *
 *******************************************************************************/
void starpu_zhetrfsp1d_hetrf_cpu(void * buffers[], void * _args)
{
    Sopalin_Data_t     *sopalin_data;
    SolverCblk         *cblk;
    pastix_complex64_t *L      = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_complex64_t *work   = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_int_t        stride = STARPU_MATRIX_GET_LD(buffers[0]);
    int                 me     = starpu_worker_get_id();

    starpu_codelet_unpack_args(_args, &sopalin_data, &cblk);

    sopalin_data->thread_data[me]->nbpivot +=
        core_zhetrfsp1d_hetrf(cblk, L,
                              sopalin_data->critere, work);
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_starpu_kernel
 *
 * starpu_zhetrfsp1d_trsm_cpu - Apply all the trsm updates on one panel.
 *
 *******************************************************************************
 *
 * @param[in, out] buffers
 *          Array of 2 buffers:
 *            -# L : The pointer to the lower matrix storing the coefficients
 *                   of the panel. Must be of size cblk.stride -by- cblk.width
 *
 * @param[in, out] _args
 *          Package of arguments to unpack with starpu_codelet_unpack_args():
 *            -# sopalin_data : common sopalin structure.
 *            -# cblk : Pointer to the structure representing the panel to
 *                      factorize in the cblktab array. Next column blok must
 *                      be accessible through cblk[1].
 *
 *
 *******************************************************************************/
void starpu_zhetrfsp1d_trsm_cpu(void * buffers[], void * _args)
{
    Sopalin_Data_t     *sopalin_data;
    SolverCblk         *cblk;
    pastix_complex64_t *L      = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_int_t        stride = STARPU_MATRIX_GET_LD(buffers[0]);

    starpu_codelet_unpack_args(_args, &sopalin_data, &cblk);

    core_zhetrfsp1d_trsm(cblk, L);

    SUBMIT_GEMMS_IF_NEEDED;
}


/**
 *******************************************************************************
 *
 * @ingroup pastix_starpu_kernel
 *
 * starpu_zhetrfsp1d_cpu - Computes the LU factorization of one panel and apply
 * all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in, out] buffers
 *          Array of 2 buffers:
 *            -# L : The pointer to the lower matrix storing the coefficients
 *                   of the panel. Must be of size cblk.stride -by- cblk.width
 *            -# work : Scratch buffers of size SOLV_COEFMAX.
 *
 * @param[in, out] _args
 *          Package of arguments to unpack with starpu_codelet_unpack_args():
 *            -# sopalin_data : common sopalin structure.
 *            -# cblk : Pointer to the structure representing the panel to
 *                      factorize in the cblktab array. Next column blok must
 *                      be accessible through cblk[1].
 *
 *
 *******************************************************************************/
void starpu_zhetrfsp1d_cpu(void * buffers[], void * _args)
{
    Sopalin_Data_t     *sopalin_data;
    SolverCblk         *cblk;
    pastix_complex64_t *L      = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_complex64_t *work   = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_int_t        stride = STARPU_MATRIX_GET_LD(buffers[0]);
    int                 me     = starpu_worker_get_id();

    starpu_codelet_unpack_args(_args, &sopalin_data, &cblk);

    sopalin_data->thread_data[me]->nbpivot +=
        core_zhetrfsp1d(cblk,
                        L,
                        sopalin_data->critere,
                        work);

    SUBMIT_GEMMS_IF_NEEDED;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_starpu_kernel
 *
 * starpu_zhetrfsp1d_gemm_cpu - Computes the Cholesky factorization of one panel
 * and apply all the trsm updates to this panel.
 *
 *******************************************************************************
 *
 * @param[in, out] buffers
 *          Array of 2 buffers:
 *            -# L : The pointer to the lower matrix storing the coefficients
 *                   of the panel. Must be of size cblk.stride -by- cblk.width
 *            -# Cl : The pointer to the lower matrix storing the coefficients
 *                    of the updated panel. Must be of size cblk.stride -by-
 *                    cblk.width
 *            -# work + work2 : Scratch buffers of size SOLV_COEFMAX.
 *
 * @param[in, out] _args
 *          Package of arguments to unpack with starpu_codelet_unpack_args():
 *            -# sopalin_data : common sopalin structure.
 *            -# cblk : Pointer to the structure representing the panel to
 *                      factorize in the cblktab array. Next column blok must
 *                      be accessible through cblk[1].
 *            -# blok : The pointer to the data structure that describes the
 *                      blok from which we compute the contributions.
 *            -# fcblk : The pointer to the data structure that describes the
 *                       panel on which we compute the contributions. Next
 *                       column blok must be accessible through fcblk[1].
 *
 *
 *******************************************************************************/
void starpu_zhetrfsp1d_gemm_cpu(void * buffers[], void * _args)
{
    Sopalin_Data_t     *sopalin_data;
    SolverCblk         *cblk;
    SolverBlok         *blok;
    SolverCblk         *fcblk;
    pastix_int_t        stride   = STARPU_MATRIX_GET_LD(buffers[0]);
    pastix_complex64_t *L    = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_complex64_t *Cl   = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
    pastix_complex64_t *work = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[2]);
    pastix_int_t        ldw  = (pastix_int_t)STARPU_VECTOR_GET_NX(buffers[2])/2;
    pastix_complex64_t *work2= work + ldw;

    starpu_codelet_unpack_args(_args, &sopalin_data, &cblk, &blok, &fcblk);

    core_zhetrfsp1d_gemm(cblk,
                         blok,
                         fcblk,
                         L, Cl,
                         work,
                         work2);
    SUBMIT_TRF_IF_NEEDED;
}


void
starpu_zgetrfsp1d_geadd_cpu(void * buffers[], void * _args) {
    Sopalin_Data_t * sopalin_data;
    SolverCblk *cblk1, *cblk2;
    pastix_complex64_t *L    = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[0]);
    pastix_complex64_t *Cl   = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(buffers[1]);
    starpu_codelet_unpack_args(_args, &sopalin_data, &cblk1, &cblk2);

    core_zgeaddsp1d(cblk1,
                    cblk2,
                    L, Cl,
                    NULL, NULL);
}