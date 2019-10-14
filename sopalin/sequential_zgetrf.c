/**
 *
 * @file sequential_zgetrf.c
 *
 * @copyright 2012-2018 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.0.1
 * @author Pascal Henon
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2018-07-16
 *
 * @precisions normal z -> s d c
 *
 **/
#include "common.h"
#include "isched.h"
#include "solver.h"
#include "sopalin_data.h"
#include "sopalin/coeftab_z.h"
#include "pastix_zcores.h"

#if defined(PASTIX_WITH_PARSEC)
#include "parsec/pastix_zparsec.h"
#endif

#if defined(PASTIX_WITH_STARPU)
#include "starpu/pastix_zstarpu.h"
#endif

void
sequential_zgetrf( pastix_data_t  *pastix_data,
                   sopalin_data_t *sopalin_data )
{
    SolverMatrix       *datacode = pastix_data->solvmatr;
    SolverCblk         *cblk;
    pastix_complex64_t *work;
    pastix_int_t  i, lwork;
    (void)sopalin_data;

    lwork = datacode->gemmmax;
    if ( datacode->lowrank.compress_when == PastixCompressWhenBegin ) {
        lwork = pastix_imax( lwork, 2 * datacode->blokmax );
    }
    MALLOC_INTERN( work, lwork, pastix_complex64_t );

    cblk = datacode->cblktab;
    for (i=0; i<datacode->cblknbr; i++, cblk++){

        if ( cblk->cblktype & CBLK_IN_SCHUR )
            break;

        /* Compute */
        cpucblk_zgetrfsp1d( datacode, cblk,
                            work, lwork );
    }

    memFree_null( work );
}

void
thread_zgetrf_static( isched_thread_t *ctx, void *args )
{
    sopalin_data_t     *sopalin_data = (sopalin_data_t*)args;
    SolverMatrix       *datacode = sopalin_data->solvmtx;
    SolverCblk         *cblk;
    Task               *t;
    pastix_complex64_t *work;
    pastix_int_t  i, ii, lwork;
    pastix_int_t  tasknbr, *tasktab;
    int rank = ctx->rank;

    lwork = datacode->gemmmax;
    if ( datacode->lowrank.compress_when == PastixCompressWhenBegin ) {
        lwork = pastix_imax( lwork, 2 * datacode->blokmax );
    }
    MALLOC_INTERN( work, lwork, pastix_complex64_t );

    tasknbr = datacode->ttsknbr[rank];
    tasktab = datacode->ttsktab[rank];

    for (ii=0; ii<tasknbr; ii++) {
        i = tasktab[ii];
        t = datacode->tasktab + i;
        cblk = datacode->cblktab + t->cblknum;

        if ( cblk->cblktype & CBLK_IN_SCHUR )
            continue;

        /* Wait */
        do { } while( cblk->ctrbcnt );

        /* Compute */
        cpucblk_zgetrfsp1d( datacode, cblk,
                            work, lwork );
    }

    memFree_null( work );
}

void
static_zgetrf( pastix_data_t  *pastix_data,
               sopalin_data_t *sopalin_data )
{
    isched_parallel_call( pastix_data->isched, thread_zgetrf_static, sopalin_data );
}

struct args_zgetrf_t
{
    sopalin_data_t     *sopalin_data;
    volatile int32_t    taskcnt;
};

void
thread_zgetrf_dynamic( isched_thread_t *ctx, void *args )
{
    struct args_zgetrf_t *arg = (struct args_zgetrf_t*)args;
    sopalin_data_t       *sopalin_data = arg->sopalin_data;
    SolverMatrix         *datacode = sopalin_data->solvmtx;
    SolverCblk           *cblk;
    Task                 *t;
    pastix_queue_t       *computeQueue;
    pastix_complex64_t   *work;
    pastix_int_t          i, ii, lwork;
    pastix_int_t          tasknbr, *tasktab, cblknum;
    int32_t               local_taskcnt = 0;
    int                   rank = ctx->rank;
    int                   dest = (ctx->rank + 1)%ctx->global_ctx->world_size;

    lwork = datacode->gemmmax;
    if ( datacode->lowrank.compress_when == PastixCompressWhenBegin ) {
        lwork = pastix_imax( lwork, 2 * datacode->blokmax );
    }
    MALLOC_INTERN( work, lwork, pastix_complex64_t );
    MALLOC_INTERN( datacode->computeQueue[rank], 1, pastix_queue_t );

    tasknbr      = datacode->ttsknbr[rank];
    tasktab      = datacode->ttsktab[rank];
    computeQueue = datacode->computeQueue[rank];
    pqueueInit( computeQueue, tasknbr );

    for (ii=0; ii<tasknbr; ii++) {
        i = tasktab[ii];
        t = datacode->tasktab + i;

        if ( !(t->ctrbcnt) ) {
            pqueuePush1( computeQueue, t->cblknum, t->prionum );
        }
    }

    /* Make sure that all computeQueues are allocated */
    isched_barrier_wait( &(ctx->global_ctx->barrier) );

    while( arg->taskcnt > 0 )
    {
        cblknum = pqueuePop(computeQueue);

        if( cblknum == -1 ){
            if ( local_taskcnt ) {
                pastix_atomic_sub_32b( &(arg->taskcnt), local_taskcnt );
                local_taskcnt = 0;
            }
            cblknum = stealQueue( datacode, rank, &dest,
                                  ctx->global_ctx->world_size );
        }
        if( cblknum != -1 ){
            cblk = datacode->cblktab + cblknum;
            if ( cblk->cblktype & CBLK_IN_SCHUR ) {
                continue;
            }

            cblk->threadid = rank;
            /* Compute */
            cpucblk_zgetrfsp1d( datacode, cblk,
                                work, lwork );
            local_taskcnt++;
        }
    }
    memFree_null( work );

    /* Make sure that everyone is done before freeing */
    isched_barrier_wait( &(ctx->global_ctx->barrier) );
    pqueueExit( computeQueue );
    memFree_null( computeQueue );
}

void
dynamic_zgetrf( pastix_data_t  *pastix_data,
                sopalin_data_t *sopalin_data )
{
    volatile int32_t     taskcnt = sopalin_data->solvmtx->tasknbr;
    struct args_zgetrf_t args_zgetrf = {sopalin_data, taskcnt};
    /* Allocate the computeQueue */
    MALLOC_INTERN( sopalin_data->solvmtx->computeQueue,
                   pastix_data->isched->world_size, pastix_queue_t * );

    isched_parallel_call( pastix_data->isched, thread_zgetrf_dynamic, &args_zgetrf );
    memFree_null( sopalin_data->solvmtx->computeQueue );
}


static void (*zgetrf_table[5])(pastix_data_t *, sopalin_data_t *) = {
    sequential_zgetrf,
    static_zgetrf,
#if defined(PASTIX_WITH_PARSEC)
    parsec_zgetrf,
#else
    NULL,
#endif
#if defined(PASTIX_WITH_STARPU)
    starpu_zgetrf,
#else
    NULL,
#endif
    dynamic_zgetrf
};

void
sopalin_zgetrf( pastix_data_t  *pastix_data,
                sopalin_data_t *sopalin_data )
{
    int sched = pastix_data->iparm[IPARM_SCHEDULER];
    void (*zgetrf)(pastix_data_t *, sopalin_data_t *) = zgetrf_table[ sched ];

    if (zgetrf == NULL) {
        zgetrf = static_zgetrf;
    }
    zgetrf( pastix_data, sopalin_data );

#if defined(PASTIX_DEBUG_FACTO)
    coeftab_zdump( pastix_data, sopalin_data->solvmtx, "getrf.txt" );
#endif
}
