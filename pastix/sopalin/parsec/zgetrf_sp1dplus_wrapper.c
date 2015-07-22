/*
 * Copyright (c) 2010      The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 *
 * @precisions normal z -> s d c
 *
 */
#include <dague.h>
#include <dague/data_distribution.h>
#include "common.h"
#include "sopalin_data.h"
#include "parsec/sparse-matrix.h"

typedef struct dague_zgetrf_sp1dplus_handle dague_zgetrf_sp1dplus_handle_t;

extern dague_zgetrf_sp1dplus_handle_t *dague_zgetrf_sp1dplus_new(dague_ddesc_t * dataA /* data dataA */, sopalin_data_t * sopalin_data);

dague_handle_t*
dsparse_zgetrf_sp_New( sparse_matrix_desc_t *A,
                       sopalin_data_t *sopalin_data )
{
    dague_zgetrf_sp1dplus_handle_t *dague_zgetrf_sp = NULL;

    dague_zgetrf_sp = dague_zgetrf_sp1dplus_new( (dague_ddesc_t*)A, sopalin_data );

    /* dague_zgetrf_sp->p_work = (dague_memory_pool_t*)malloc(sizeof(dague_memory_pool_t)); */
    /* dague_private_memory_init( dague_zgetrf_sp->p_work, (A->pastix_data->solvmatr).coefmax * sizeof(dague_complex64_t) ); */

    /* dsparse_add2arena_tile(((dague_zgetrf_Url_handle_t*)dague_zgetrf)->arenas[DAGUE_zgetrf_Url_DEFAULT_ARENA],  */
    /*                        A->mb*A->nb*sizeof(dague_complex64_t), */
    /*                        DAGUE_ARENA_ALIGNMENT_SSE, */
    /*                        MPI_DOUBLE_COMPLEX, A->mb); */

    return (dague_handle_t*)dague_zgetrf_sp;
}

void
dsparse_zgetrf_sp_Destruct( dague_handle_t *o )
{
    (void)o;
    /* dague_zgetrf_sp1dplus_handle_t *dague_zgetrf_sp = NULL; */
    /* dague_zgetrf_sp = (dague_zgetrf_sp1dplus_handle_t *)o; */

    /*dsparse_datatype_undefine_type( &(ogetrf->arenas[DAGUE_zgetrf_Url_DEFAULT_ARENA]->opaque_dtt) );*/

    /* dague_private_memory_fini( dague_zgetrf_sp->p_work ); */
    /* free( dague_zgetrf_sp->p_work ); */

    //DAGUE_INTERNAL_HANDLE_DESTRUCT(o);
}

int dsparse_zgetrf_sp( dague_context_t *dague,
                       sparse_matrix_desc_t *A,
                       sopalin_data_t *sopalin_data )
{
    dague_handle_t *dague_zgetrf_sp = NULL;
    int info = 0;

    dague_zgetrf_sp = dsparse_zgetrf_sp_New( A, sopalin_data );

    if ( dague_zgetrf_sp != NULL )
    {
        dague_enqueue( dague, (dague_handle_t*)dague_zgetrf_sp);
        dague_context_wait( dague );
        dsparse_zgetrf_sp_Destruct( dague_zgetrf_sp );
    }
    return info;
}
