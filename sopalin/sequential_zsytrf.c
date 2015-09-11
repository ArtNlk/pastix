/**
 *
 * @file sopalin_zsytrf.c
 *
 *  PaStiX factorization routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 * @precisions normal z -> s d c
 *
 **/
#include "common.h"
#include "isched.h"
#include "sopalin_data.h"
#include "pastix_zcores.h"

#if defined(PASTIX_WITH_PARSEC)
#include <dague.h>
#include <dague/data.h>
#include <dague/data_distribution.h>
#include "parsec/sparse-matrix.h"
#endif

void
sequential_zsytrf( pastix_data_t  *pastix_data,
                   sopalin_data_t *sopalin_data )
{
    SolverMatrix *datacode = pastix_data->solvmatr;
    SolverCblk   *cblk;
    double        threshold = sopalin_data->diagthreshold;
    pastix_int_t  i;
    (void)pastix_data;

    cblk = datacode->cblktab;
    for (i=0; i<datacode->cblknbr; i++, cblk++){
        /* Compute */
        core_zsytrfsp1d( datacode, cblk, threshold );
    }

#if defined(PASTIX_DEBUG_FACTO)
    coeftab_zdump( datacode, "sytrf_L.txt" );
#endif
}

void
thread_pzsytrf( int rank, void *args )
{
    sopalin_data_t *sopalin_data = (sopalin_data_t*)args;
    SolverMatrix *datacode = sopalin_data->solvmtx;
    SolverCblk   *cblk;
    Task         *t;
    pastix_int_t  i, ii;
    pastix_int_t  tasknbr, *tasktab;

    tasknbr = datacode->ttsknbr[rank];
    tasktab = datacode->ttsktab[rank];

    for (ii=0; ii<tasknbr; ii++) {
        i = tasktab[ii];
        t = datacode->tasktab + i;
        cblk = datacode->cblktab + t->cblknum;

        /* Compute */
        core_zsytrfsp1d( datacode, cblk, sopalin_data->diagthreshold );
    }

#if defined(PASTIX_DEBUG_FACTO)
    isched_barrier_wait( &(((isched_t*)(sopalin_data->sched))->barrier) );
    if (rank == 0) {
        coeftab_zdump( datacode, "sytrf_L.txt" );
    }
    isched_barrier_wait( &(((isched_t*)(sopalin_data->sched))->barrier) );
#endif
}


void
thread_zsytrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data )
{
    isched_parallel_call( pastix_data->isched, thread_pzsytrf, sopalin_data );
}

#if defined(PASTIX_WITH_PARSEC)
void
parsec_zsytrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data )
{
    sparse_matrix_desc_t desc;
    dague_context_t *ctx;
    int argc = 0;

    /* Start PaRSEC */
    if (pastix_data->parsec == NULL) {
        pastix_data->parsec = dague_init( -1, &argc, NULL );
    }
    ctx = pastix_data->parsec;

    /* Create the matrix descriptor */
    sparse_matrix_init( &desc, sopalin_data->solvmtx,
                        pastix_size_of( PastixComplex64 ), 1, 0 );

    /* Run the facto */
    dsparse_zsytrf_sp( ctx, &desc, sopalin_data );

    /* Destroy the decriptor */
    sparse_matrix_destroy( &desc );

    dague_fini( &(pastix_data->parsec) );
}
#endif

static void (*zsytrf_table[4])(pastix_data_t *, sopalin_data_t *) = {
    sequential_zsytrf,
    thread_zsytrf,
#if defined(PASTIX_WITH_PARSEC)
    parsec_zsytrf,
#else
    NULL,
#endif
    NULL
};

void
sopalin_zsytrf( pastix_data_t  *pastix_data,
                sopalin_data_t *sopalin_data )
{
    int sched = pastix_data->iparm[IPARM_SCHEDULER];
    void (*zsytrf)(pastix_data_t *, sopalin_data_t *) = zsytrf_table[ sched ];

    if (zsytrf == NULL) {
        zsytrf = thread_zsytrf;
    }
    zsytrf( pastix_data, sopalin_data );
}
