/**
 *
 * @file starpu_dense_matrix.c
 *
 * PaStiX dense matrix descriptor for StarPU.
 *
 * @copyright 2016-2023 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.3.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @date 2023-01-17
 *
 * @ingroup pastix_starpu
 * @{
 *
 **/
#include "common.h"
#include "blend/solver.h"
#include "pastix_starpu.h"
#include <starpu_data.h>

/**
 *******************************************************************************
 *
 * @brief Generate the StarPU descriptor of the dense matrix.
 *
 * This function creates the StarPU descriptor that will provide tha data
 * mapping and memory location to StarPU for the computation.
 *
 *******************************************************************************
 *
 * @param[inout] solvmtx
 *          The solver matrix structure that describes the dense matrix for
 *          PaStiX.
 *
 * @param[in] ncol
 *          The number of columns of the given matrix. The number of rows is
 *          given by the solvmtx structure.
 *
 * @param[in] A
 *          The pointer to the matrix.
 *
 * @param[in] lda
 *          The leading dimension of the matrix A.
 *
 * @param[in] typesize
 *          The memory size of the arithmetic used to store the matrix
 *          coefficients.
 *
 * @param[in] nodes
 *          The number of processes used to solve the problem.
 *
 * @param[in] myrank
 *          The rank of the calling process.
 *
 ******************************************************************************/
void
starpu_dense_matrix_init( SolverMatrix *solvmtx,
                          pastix_int_t ncol, char *A, pastix_int_t lda,
                          int typesize, int nodes, int myrank )
{
    starpu_data_handle_t *handler;
    SolverCblk   *cblk;
    pastix_int_t  cblknbr, cblknum;
    pastix_int_t  nrow;

    starpu_dense_matrix_desc_t *spmtx = solvmtx->starpu_desc_rhs;
    if ( spmtx != NULL ) {
        if ( (ncol == spmtx->ncol) &&
             (A    == spmtx->dataptr) ) {
            return;
        }
        starpu_dense_matrix_destroy( spmtx );
    }
    else {
        spmtx = (starpu_dense_matrix_desc_t*)malloc(sizeof(starpu_dense_matrix_desc_t));
    }

    cblknbr = solvmtx->cblknbr;

    spmtx->mpitag  = pastix_starpu_tag_book( solvmtx->gcblknbr );
    spmtx->ncol    = ncol;
    spmtx->typesze = typesize;
    spmtx->solvmtx = solvmtx;
    spmtx->handletab = malloc( cblknbr * sizeof(starpu_data_handle_t) );
    spmtx->dataptr   = A;

    /* Initialize 1D cblk handlers */
    cblk    = spmtx->solvmtx->cblktab;
    handler = spmtx->handletab;
    for( cblknum = 0;
         cblknum < cblknbr;
         cblknum++, cblk++, handler++ )
    {
        nrow = cblk_colnbr( cblk );

        if( cblk->ownerid == myrank ) {
            starpu_matrix_data_register( handler, STARPU_MAIN_RAM,
                                         (uintptr_t)(A + (cblk->lcolidx * typesize)),
                                         lda, nrow, ncol, typesize );
        }
        else {
            starpu_matrix_data_register( handler, -1, 0,
                                         lda, nrow, ncol, typesize );
        }
#if defined(PASTIX_WITH_MPI)
        starpu_mpi_data_register( *handler, spmtx->mpitag + ((int64_t)cblknum), cblk->ownerid );
#endif
    }

    solvmtx->starpu_desc_rhs = spmtx;

    (void)nodes;
    (void)myrank;
}

/**
 *******************************************************************************
 *
 * @brief Submit asynchronous calls to retrieve the data on main memory.
 *
 *******************************************************************************
 *
 * @param[inout] spmtx
 *          The dense matrix descriptor to retrieve on main memory.
 *
 ******************************************************************************/
void
starpu_dense_matrix_getoncpu( starpu_dense_matrix_desc_t *spmtx )
{
    starpu_data_handle_t *handler = spmtx->handletab;
    SolverCblk *cblk;
    pastix_int_t cblknbr, cblknum;

    cblk    = spmtx->solvmtx->cblktab;
    cblknbr = spmtx->solvmtx->cblknbr;
    for(cblknum=0; cblknum<cblknbr; cblknum++, cblk++, handler++)
    {
        assert( handler );

#if defined(PASTIX_WITH_MPI)
        starpu_mpi_cache_flush( spmtx->solvmtx->solv_comm, *handler );
#endif
        if ( cblk->ownerid == spmtx->solvmtx->clustnum ) {
            starpu_data_acquire_cb( *handler, STARPU_R,
                                    (void (*)(void*))&starpu_data_release,
                                    *handler );
        }
    }
}

/**
 *******************************************************************************
 *
 * @brief Free the StarPU descriptor of the dense matrix.
 *
 * This function destroys the StarPU descriptor, but do not free the matrix data
 * that are managed by PaStiX.
 *
 *******************************************************************************
 *
 * @param[inout] spmtx
 *          The descriptor to free.
 *
 ******************************************************************************/
void
starpu_dense_matrix_destroy( starpu_dense_matrix_desc_t *spmtx )
{
    starpu_data_handle_t *handler = spmtx->handletab;
    SolverCblk *cblk;
    pastix_int_t cblknbr, cblknum;

    cblk    = spmtx->solvmtx->cblktab;
    cblknbr = spmtx->solvmtx->cblknbr;
    for(cblknum=0; cblknum<cblknbr; cblknum++, cblk++, handler++)
    {
        assert( handler );
        starpu_data_unregister( *handler );
    }

    free( spmtx->handletab );
    spmtx->handletab = NULL;

     pastix_starpu_tag_release( spmtx->mpitag );
}

/**
 * @}
 */
