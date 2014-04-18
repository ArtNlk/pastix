/**
 * @file starpu_zregister_data.c
 *
 * @author Xavier Lacoste
 * @precisions normal z -> s d c
 */

#include "starpu_defines.h"
#include "common.h"
#include "sopalin3d.h"
#include "starpu_zregister_data.h"
#include "sopalin_acces.h"

void starpu_zfanin_init_cpu_func(void *descr[], void *cl_arg)
{
    pastix_complex64_t *L      = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(descr[0]);
    pastix_int_t        stride = STARPU_MATRIX_GET_LD(descr[0]);
    pastix_int_t        ncol   = STARPU_MATRIX_GET_NX(descr[0]);
    memset(L, 0, stride*ncol*sizeof(pastix_complex64_t));
}

#ifdef STARPU_USE_CUDA
void starpu_zfanin_init_cuda_func(void *descr[], void *cl_arg)
{
    pastix_complex64_t *L      = (pastix_complex64_t*)STARPU_MATRIX_GET_PTR(descr[0]);
    pastix_int_t        stride = STARPU_MATRIX_GET_LD(descr[0]);
    pastix_int_t        ncol   = STARPU_MATRIX_GET_NX(descr[0]);
    cudaMemsetAsync(L, 0, stride*ncol*sizeof(pastix_complex64_t), starpu_cuda_get_local_stream());
}
#endif

static struct starpu_codelet starpu_zfanin_init_codelet =
{
    .where = STARPU_CPU|STARPU_CUDA,
    .cpu_funcs = {starpu_zfanin_init_cpu_func, NULL},
    //.cpu_funcs_name = {"starpu_zfanin_init_cpu_func", NULL},
#ifdef STARPU_USE_CUDA
    .cuda_funcs = {starpu_zfanin_init_cuda_func, NULL},
    .cuda_flags = {STARPU_CUDA_ASYNC},
#endif
#ifdef STARPU_USE_OPENCL
    .opencl_funcs = {init_opencl_func, NULL},
#endif
    .modes = {STARPU_W},
    .nbuffers = 1,
    .name = "init",
};

int
starpu_zregister_fanin(SolverMatrix * solvmtx,
                       starpu_data_handle_t  *** Lfanin_handle,
                       starpu_data_handle_t  *** Ufanin_handle) {
    pastix_int_t clustnum;
    pastix_int_t max_ftgtnbr;
    MALLOC_INTERN(*Lfanin_handle, solvmtx->clustnbr, starpu_data_handle_t*);
    if (Ufanin_handle != NULL)
        MALLOC_INTERN(*Ufanin_handle, solvmtx->clustnbr, starpu_data_handle_t*);
    for (clustnum = 0; clustnum < solvmtx->clustnbr; clustnum++) {
        pastix_int_t coef = 1;
        SolverCblk *fanin;
        starpu_data_handle_t *Lhandle;
        starpu_data_handle_t *Uhandle;
        SolverCblk * ffanin = solvmtx->fcblktab[clustnum];
        SolverCblk * lfanin = ffanin + solvmtx->fcblknbr[clustnum];

        MALLOC_INTERN((*Lfanin_handle)[clustnum],
                      solvmtx->fcblknbr[clustnum], starpu_data_handle_t);
        Lhandle = (*Lfanin_handle)[clustnum];
        if (Ufanin_handle != NULL) {
            MALLOC_INTERN((*Ufanin_handle)[clustnum],
                          solvmtx->fcblknbr[clustnum], starpu_data_handle_t);
            Uhandle = (*Ufanin_handle)[clustnum];
            coef = 2;
        }
        for (fanin = ffanin; fanin < lfanin; fanin++) {
            starpu_matrix_data_register(Lhandle, -1,
                                        (uintptr_t)NULL,
                                        (uint32_t)fanin->stride,
                                        (uint32_t)fanin->stride,
                                        cblk_colnbr(fanin),
                                        sizeof(pastix_complex64_t));
            if (Ufanin_handle != NULL) {
                starpu_matrix_data_register(Uhandle, -1,
                                            (uintptr_t)NULL,
                                            (uint32_t)fanin->stride,
                                            (uint32_t)fanin->stride,
                                            cblk_colnbr(fanin),
                                            sizeof(pastix_complex64_t));
            }
            starpu_mpi_data_register(*Lhandle,
                                     coef * ( solvmtx->gcblknbr +
                                              clustnum * solvmtx->gcblknbr ) +
                                     fanin->gcblknum,
                                     clustnum);
            if (clustnum == solvmtx->clustnum) {
                starpu_data_set_reduction_methods(*Lhandle, NULL,
                                                  &starpu_zfanin_init_codelet);
            }
            Lhandle++;
            if (Ufanin_handle != NULL) {
                starpu_mpi_data_register(*Uhandle,
                                         coef * ( solvmtx->gcblknbr +
                                                  clustnum * solvmtx->gcblknbr + 1) +
                                         fanin->gcblknum,
                                         clustnum);
                if (clustnum == solvmtx->clustnum) {
                    starpu_data_set_reduction_methods(*Uhandle, NULL,
                                                      &starpu_zfanin_init_codelet);
                }
                Uhandle++;
            }
        }
    }
    return PASTIX_SUCCESS;
}

int
starpu_zunregister_fanin( SolverMatrix * solvmtx,
                          starpu_data_handle_t  *** Lfanin_handle,
                          starpu_data_handle_t  *** Ufanin_handle) {
    pastix_int_t clustnum;
    pastix_int_t max_ftgtnbr;
    for (clustnum = 0; clustnum < solvmtx->clustnbr; clustnum++) {
        pastix_int_t coef = 1;
        SolverCblk *fanin;
        starpu_data_handle_t *Lhandle;
        starpu_data_handle_t *Uhandle;
        SolverCblk * ffanin = solvmtx->fcblktab[clustnum];
        SolverCblk * lfanin = ffanin + solvmtx->fcblknbr[clustnum];

        Lhandle = (*Lfanin_handle)[clustnum];
        if (Ufanin_handle != NULL) {
            Uhandle = (*Ufanin_handle)[clustnum];
            coef = 2;
        }
        for (fanin = ffanin; fanin < lfanin; fanin++) {
            starpu_data_unregister(*Lhandle);
            Lhandle++;
            if (Ufanin_handle != NULL) {
                starpu_data_unregister(*Uhandle);
                Uhandle++;
            }
        }
        memFree_null((*Lfanin_handle)[clustnum]);
        if (Ufanin_handle != NULL)
            memFree_null((*Ufanin_handle)[clustnum]);

    }
    memFree_null(*Lfanin_handle);
    if (Ufanin_handle != NULL)
        memFree_null(*Ufanin_handle);
    return PASTIX_SUCCESS;
}

int
starpu_zregister_halo( SolverMatrix * datacode,
                       starpu_data_handle_t ** Lhalo_handle,
                       starpu_data_handle_t ** Uhalo_handle) {
    pastix_int_t itercblk;
    MALLOC_INTERN(*Lhalo_handle, SOLV_HCBLKNBR, starpu_data_handle_t);
    if (Uhalo_handle) {
        MALLOC_INTERN(*Uhalo_handle, SOLV_HCBLKNBR, starpu_data_handle_t);
    }

    for (itercblk=0;itercblk<SOLV_HCBLKNBR;itercblk++) {
        SolverCblk * cblk = datacode->hcblktab + itercblk;

        ASSERT(SOLV_GCBLK2HALO(HCBLK_GCBLK(itercblk)) == itercblk,
               MOD_SOPALIN);

        starpu_matrix_data_register(&((*Lhalo_handle)[itercblk]), -1,
                                    (uintptr_t)NULL,
                                    (uint32_t)cblk->stride,
                                    (uint32_t)cblk->stride,
                                    cblk_colnbr(cblk),
                                    sizeof(pastix_complex64_t));
        starpu_mpi_data_register((*Lhalo_handle)[itercblk],
                                 cblk->gcblknum,
                                 cblk->procdiag);
        if (Uhalo_handle) {
            starpu_matrix_data_register(&((*Uhalo_handle)[itercblk]), -1,
                                        (uintptr_t)NULL,
                                        (uint32_t)cblk->stride,
                                        (uint32_t)cblk->stride,
                                        cblk_colnbr(cblk),
                                        sizeof(pastix_complex64_t));
            starpu_mpi_data_register((*Uhalo_handle)[itercblk],
                                     SOLV_GCBLKNBR + cblk->gcblknum,
                                 cblk->procdiag);
        }
    }
    return PASTIX_SUCCESS;
}

int
starpu_zunregister_halo( SolverMatrix * datacode,
                         starpu_data_handle_t ** Lhalo_handle,
                         starpu_data_handle_t ** Uhalo_handle) {
    pastix_int_t itercblk;

    for (itercblk=0;itercblk<SOLV_HCBLKNBR;itercblk++) {
        ASSERT(SOLV_GCBLK2HALO(HCBLK_GCBLK(itercblk)) == itercblk,
               MOD_SOPALIN);
        starpu_data_unregister((*Lhalo_handle)[itercblk]);
        if (Uhalo_handle) {
            starpu_data_unregister((*Uhalo_handle)[itercblk]);
        }
    }
    memFree_null(*Lhalo_handle);
    if (Uhalo_handle) {
        memFree_null(*Uhalo_handle);
    }
    return PASTIX_SUCCESS;
}

int
starpu_zregister_cblk( SolverMatrix * datacode,
                       starpu_data_handle_t ** L_handle,
                       starpu_data_handle_t ** U_handle ) {
    pastix_int_t itercblk;
    MALLOC_INTERN(*L_handle,     SYMB_CBLKNBR, starpu_data_handle_t);
    if (U_handle) {
        MALLOC_INTERN(*U_handle,    SYMB_CBLKNBR, starpu_data_handle_t);
    }

    for (itercblk=0;itercblk<SYMB_CBLKNBR;itercblk++) {
        SolverCblk * cblk = datacode->cblktab+itercblk;
        starpu_matrix_data_register(&((*L_handle)[itercblk]), 0,
                                    (uintptr_t)cblk->coeftab,
                                    (uint32_t)cblk->stride,
                                    (uint32_t)cblk->stride,
                                    cblk_colnbr(cblk),
                                    sizeof(pastix_complex64_t));
        starpu_mpi_data_register((*L_handle)[itercblk],
                                 UPDOWN_LOC2GLOB(itercblk),
                                 SOLV_PROCNUM);

        if (U_handle) {
            starpu_matrix_data_register(&((*U_handle)[itercblk]), 0,
                                        (uintptr_t)cblk->ucoeftab,
                                        (uint32_t)cblk->stride,
                                        (uint32_t)cblk->stride,
                                        cblk_colnbr(cblk),
                                        sizeof(pastix_complex64_t));
            starpu_mpi_data_register((*U_handle)[itercblk],
                                     SOLV_GCBLKNBR + UPDOWN_LOC2GLOB(itercblk),
                                     SOLV_PROCNUM);
        }
    }
    return PASTIX_SUCCESS;
}

int
starpu_zunregister_cblk( SolverMatrix * datacode,
                         starpu_data_handle_t ** L_handle,
                         starpu_data_handle_t ** U_handle ) {
    pastix_int_t itercblk;
    for (itercblk=0;itercblk<SYMB_CBLKNBR;itercblk++) {
        starpu_data_unregister((*L_handle)[itercblk]);
        if (U_handle) {
            starpu_data_unregister((*U_handle)[itercblk]);
        }
    }
    memFree_null(*L_handle);
    if (U_handle) {
        memFree_null(*U_handle);
    }

    return PASTIX_SUCCESS;
}

int
starpu_zregister_data( Sopalin_Data_t * sopalin_data,
                       starpu_data_handle_t ** L_handle,
                       starpu_data_handle_t ** U_handle,
                       starpu_data_handle_t ** Lhalo_handle,
                       starpu_data_handle_t ** Uhalo_handle,
                       starpu_data_handle_t *** Lfanin_handle,
                       starpu_data_handle_t *** Ufanin_handle)
{
    SolverMatrix       * datacode         = sopalin_data->datacode;
    starpu_zregister_cblk(datacode, L_handle, U_handle);
    /*Either submit halo or fanin */
    {
        char * fanin;
        /* register halo */
        starpu_zregister_halo(datacode,
                              Lhalo_handle,
                              Uhalo_handle);
        if (pastix_starpu_with_fanin() == API_YES) {
            /* register FANIN */
            starpu_zregister_fanin(datacode,
                                   Lfanin_handle,
                                   Ufanin_handle);
        }
    }
    return PASTIX_SUCCESS;
}

int
starpu_zunregister_data( Sopalin_Data_t * sopalin_data,
                         starpu_data_handle_t ** L_handle,
                         starpu_data_handle_t ** U_handle,
                         starpu_data_handle_t ** Lhalo_handle,
                         starpu_data_handle_t ** Uhalo_handle,
                         starpu_data_handle_t *** Lfanin_handle,
                         starpu_data_handle_t *** Ufanin_handle)
{
    SolverMatrix       * datacode         = sopalin_data->datacode;
    starpu_zunregister_cblk(datacode, L_handle, U_handle);
    /*Either submit halo or fanin */
    {
        char * fanin;
        /* register halo */
        starpu_zunregister_halo(datacode,
                                Lhalo_handle,
                                Uhalo_handle);
        if (pastix_starpu_with_fanin() == API_YES) {
            /* register FANIN */
            starpu_zunregister_fanin(datacode,
                                     Lfanin_handle,
                                     Ufanin_handle);
        }
    }
    return PASTIX_SUCCESS;
}
