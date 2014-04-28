/**
 * @file starpu_zsubmit.c
 *
 * @author Xavier Lacoste
 * @precisions normal z -> s d c
 */

#include "starpu_defines.h"
#include "common.h"
#include "starpu_zsubmit.h"
#include "starpu_zkernels.h"
#include "sopalin_acces.h"
#include "perf.h"

static size_t starpu_zsyadd_size(struct starpu_task *task,
#ifdef STARPU_1_2
                                 struct starpu_perfmodel_arch *arch,
#else
                                 enum starpu_perfmodel_archtype arch,
#endif
                                 unsigned nimpl) {
    Sopalin_Data_t    * sopalin_data;
    SolverCblk        * cblk, *fcblk;
    pastix_int_t        stride;
    size_t              dima;
    starpu_codelet_unpack_args(task->cl_arg, &sopalin_data, &cblk, &fcblk);
    stride   = cblk->stride;
    dima     = cblk->lcolnum - cblk->fcolnum + 1;
    return OPS_GEAM(stride, dima);

}

static size_t starpu_zgeadd_size(struct starpu_task *task,
#ifdef STARPU_1_2
                                 struct starpu_perfmodel_arch *arch,
#else
                                 enum starpu_perfmodel_archtype arch,
#endif
                                 unsigned nimpl) {
    return 2*starpu_zsyadd_size(task, arch, nimpl);
}


static struct starpu_perfmodel starpu_zgeadd_model;

static struct starpu_perfmodel starpu_zsyadd_model;

struct starpu_codelet starpu_zgeadd_cl =
{
    .where = STARPU_CPU,
    .cpu_funcs[0] = starpu_zgetrfsp1d_geadd_cpu,
    //.cuda_funcs[0] = xxtrf_starpu_cuda,
    .model = &starpu_zgeadd_model,
    /* LU */
    .nbuffers = 5,
    .modes = {
        STARPU_R,
        STARPU_RW,
        STARPU_R,
        STARPU_RW,
        STARPU_R
    }
};

struct starpu_codelet starpu_zsyadd_cl =
{
    .where = STARPU_CPU,
    .cpu_funcs[0] = starpu_zsytrfsp1d_syadd_cpu,
    //.cuda_funcs[0] = xxtrf_starpu_cuda,
    .model = &starpu_zsyadd_model,
    /* LU */
    .nbuffers = 3,
    .modes = {
        STARPU_R,
        STARPU_RW,
        STARPU_R
    }
};

int
starpu_zgesubmit_incomming_fanin(Sopalin_Data_t * sopalin_data) {
    SolverMatrix * datacode = sopalin_data->datacode;
    starpu_loop_data_t  *starpu_loop_data  = sopalin_data->starpu_loop_data;
    pastix_int_t itertask, workerid = -1;
    starpu_data_handle_t *L_handle         = starpu_loop_data->L_handle;
    starpu_data_handle_t **Lfanin_handle   = starpu_loop_data->Lfanin_handle;
    starpu_data_handle_t *U_handle         = starpu_loop_data->U_handle;
    starpu_data_handle_t **Ufanin_handle   = starpu_loop_data->Ufanin_handle;
    pastix_int_t max_deps = 0;
    int ret;
    SolverCblk *fanin;
    SolverBlok *blok;
    pastix_int_t clustnum;

    starpu_zgeadd_model.type = STARPU_REGRESSION_BASED;
    starpu_zgeadd_model.symbol = "FANIN_ADD";
#ifdef STARPU_1_2
    starpu_perfmodel_init(&starpu_zgeadd_model);
    starpu_zgeadd_model.per_arch[STARPU_CPU_WORKER][0][0][0].size_base  = starpu_zgeadd_size;
    if(starpu_worker_get_count_by_type(STARPU_CUDA_WORKER) != 0)
        starpu_zgeadd_model.per_arch[STARPU_CUDA_WORKER][0][0][0].size_base = starpu_zgeadd_size;
#  else
    starpu_zgeadd_model.per_arch[STARPU_CPU_WORKER][0].size_base  = starpu_zgeadd_size;
    starpu_zgeadd_model.per_arch[STARPU_CUDA_WORKER][0].size_base = starpu_zgeadd_size;
#  endif

    for (clustnum = 0; clustnum < datacode->clustnbr; clustnum++) {
        for (fanin = datacode->fcblktab[clustnum];
             fanin < datacode->fcblktab[clustnum] + datacode->fcblknbr[clustnum];
             fanin++) {
            SolverCblk  *fcblk;
            pastix_int_t gfcblk = fanin->gcblknum;
            pastix_int_t lfcblk = SOLV_GCBLK2LOC(gfcblk);
            pastix_int_t faninnum = fcblk_getnum(datacode, fanin, clustnum);
            fcblk = datacode->cblktab+lfcblk;
            assert(cblk_islocal(datacode, fcblk) == API_YES);
            assert(lfcblk >= 0);
            ret =
                starpu_mpi_insert_task(sopalin_data->sopar->pastix_comm,
                                       &starpu_zgeadd_cl,
                                       STARPU_VALUE, &sopalin_data, sizeof(Sopalin_Data_t*),
                                       STARPU_VALUE, &fanin,        sizeof(SolverCblk*),
                                       STARPU_VALUE, &fcblk,        sizeof(SolverCblk*),
                                       STARPU_R,     Lfanin_handle[clustnum][faninnum],
                                       STARPU_COMMUTE|STARPU_RW,    L_handle[lfcblk],
                                       STARPU_R,     Ufanin_handle[clustnum][faninnum],
                                       STARPU_COMMUTE|STARPU_RW,    U_handle[lfcblk],
                                       STARPU_SCRATCH, starpu_loop_data->WORK_handle,
                                       STARPU_R, starpu_loop_data->blocktab_handles[SOLV_PROCNUM],
                                       STARPU_EXECUTE_ON_WORKER, workerid,
                                       STARPU_CALLBACK,     starpu_prof_callback,
                                       STARPU_CALLBACK_ARG, sopalin_data->hgemm_stats,
#ifdef STARPU_CONTEXT
                                       STARPU_SCHED_CTX, sched_ctxs[my_ctx],
#endif
                                       0);
        }
    }
    return PASTIX_SUCCESS;
}

int
starpu_zsysubmit_incomming_fanin(Sopalin_Data_t * sopalin_data) {
    SolverMatrix * datacode = sopalin_data->datacode;
    starpu_loop_data_t  *starpu_loop_data  = sopalin_data->starpu_loop_data;
    pastix_int_t itertask, workerid = -1;
    starpu_data_handle_t *L_handle         = starpu_loop_data->L_handle;
    starpu_data_handle_t **Lfanin_handle   = starpu_loop_data->Lfanin_handle;
    pastix_int_t max_deps = 0;
    pastix_int_t clustnum;
    int ret;
    SolverCblk *fanin;
    SolverBlok *blok;

    starpu_zsyadd_model.type = STARPU_REGRESSION_BASED;
    starpu_zsyadd_model.symbol = "FANIN_ADD";
#ifdef STARPU_1_2
    starpu_perfmodel_init(&starpu_zsyadd_model);
    starpu_zsyadd_model.per_arch[STARPU_CPU_WORKER][0][0][0].size_base  = starpu_zsyadd_size;
    if(starpu_worker_get_count_by_type(STARPU_CUDA_WORKER) != 0)
        starpu_zsyadd_model.per_arch[STARPU_CUDA_WORKER][0][0][0].size_base = starpu_zsyadd_size;
#  else
    starpu_zsyadd_model.per_arch[STARPU_CPU_WORKER][0].size_base  = starpu_zsyadd_size;
    starpu_zsyadd_model.per_arch[STARPU_CUDA_WORKER][0].size_base = starpu_zsyadd_size;
#  endif

    for (clustnum = 0; clustnum < datacode->clustnbr; clustnum++) {
        if (clustnum == datacode->clustnum) continue;
        for (fanin = datacode->fcblktab[clustnum];
             fanin < datacode->fcblktab[clustnum] + datacode->fcblknbr[clustnum];
             fanin++) {
            SolverCblk *fcblk;
            pastix_int_t gfcblk = fanin->gcblknum;
            pastix_int_t lfcblk = SOLV_GCBLK2LOC(gfcblk);
            pastix_int_t faninnum = fcblk_getnum(datacode, fanin, clustnum);
            fcblk = datacode->cblktab+lfcblk;

            assert(lfcblk >= 0);
            assert(cblk_islocal(datacode, fcblk));
            ret =
                starpu_mpi_insert_task(sopalin_data->sopar->pastix_comm,
                                       &starpu_zsyadd_cl,
                                       STARPU_VALUE, &sopalin_data, sizeof(Sopalin_Data_t*),
                                       STARPU_VALUE, &fanin,        sizeof(SolverCblk*),
                                       STARPU_VALUE, &fcblk,       sizeof(SolverCblk*),
                                       STARPU_R,     Lfanin_handle[clustnum][faninnum],
                                       STARPU_COMMUTE|STARPU_RW,    L_handle[lfcblk],
                                       STARPU_R, starpu_loop_data->blocktab_handles[SOLV_PROCNUM],
#ifdef STARPU_1_2
                                       STARPU_EXECUTE_ON_WORKER, workerid,
#endif
                                       STARPU_CALLBACK,     starpu_prof_callback,
                                       STARPU_CALLBACK_ARG, sopalin_data->hgemm_stats,
#ifdef STARPU_CONTEXT
                                       STARPU_SCHED_CTX, sched_ctxs[my_ctx],
#endif
                                       0);
            STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_insert_task");
        }
    }
    return PASTIX_SUCCESS;
}

int starpu_zgesubmit_outgoing_fanin( Sopalin_Data_t * sopalin_data,
                                     SolverCblk     * fcblk,
                                     SolverCblk     * hcblk ) {
    SolverMatrix          *datacode         = sopalin_data->datacode;
    starpu_loop_data_t    *starpu_loop_data = sopalin_data->starpu_loop_data;
    starpu_data_handle_t  *Lhalo_handle     = starpu_loop_data->Lhalo_handle;
    starpu_data_handle_t  *Lfanin_handle    = starpu_loop_data->Lfanin_handle[SOLV_PROCNUM];
    starpu_data_handle_t  *Uhalo_handle     = starpu_loop_data->Uhalo_handle;
    starpu_data_handle_t  *Ufanin_handle    = starpu_loop_data->Ufanin_handle[SOLV_PROCNUM];
    pastix_int_t fcblkidx   = fcblk_getnum(datacode, fcblk, SOLV_PROCNUM);
    pastix_int_t hcblkidx   = hcblk_getnum(datacode, hcblk);
    int ret, workerid = -1;
#ifdef STARPU_CONTEXT
    pastix_int_t sched_ctxs = starpu_loop_data->sched_ctxs;
    pastix_int_t threadid = TASK_THREADID(itertask);
    pastix_int_t thread_per_ctx = starpu_loop_data->thread_per_ctx;
    pastix_int_t my_ctx;
    my_ctx = 1+threadid/thread_per_ctx;
#endif
    assert(hcblk->procdiag  == starpu_data_get_rank(Lhalo_handle[hcblkidx]));
    assert(hcblk->procdiag < datacode->clustnbr);
    ret =
        starpu_mpi_insert_task(sopalin_data->sopar->pastix_comm,
                               &starpu_zsyadd_cl,
                               STARPU_VALUE, &sopalin_data, sizeof(Sopalin_Data_t*),
                               STARPU_VALUE, &fcblk,        sizeof(SolverCblk*),
                               STARPU_VALUE, &hcblk,        sizeof(SolverCblk*),
                               STARPU_R,                    Lfanin_handle[fcblkidx],
                               STARPU_COMMUTE|STARPU_RW,    Lhalo_handle[hcblkidx],
                               STARPU_R,                    Ufanin_handle[fcblkidx],
                               STARPU_COMMUTE|STARPU_RW,    Uhalo_handle[hcblkidx],
                               STARPU_R, starpu_loop_data->blocktab_handles[hcblk->procdiag],
#ifdef STARPU_1_1
                               STARPU_EXECUTE_ON_WORKER, workerid,
#endif
                               STARPU_CALLBACK,     starpu_prof_callback,
                               STARPU_CALLBACK_ARG, sopalin_data->hgemm_stats,
#ifdef STARPU_CONTEXT
                               STARPU_SCHED_CTX,    sched_ctxs[my_ctx],
#endif
                               0);
    STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_insert_task");
    starpu_data_unregister_submit(Lfanin_handle[fcblkidx]);
    starpu_data_unregister_submit(Ufanin_handle[fcblkidx]);
    return PASTIX_SUCCESS;
}

int starpu_zsysubmit_outgoing_fanin( Sopalin_Data_t * sopalin_data,
                                     SolverCblk     * fcblk,
                                     SolverCblk     * hcblk ) {
    SolverMatrix          *datacode         = sopalin_data->datacode;
    starpu_loop_data_t    *starpu_loop_data = sopalin_data->starpu_loop_data;
    starpu_data_handle_t  *Lhalo_handle     = starpu_loop_data->Lhalo_handle;
    starpu_data_handle_t  *Lfanin_handle    = starpu_loop_data->Lfanin_handle[SOLV_PROCNUM];
    pastix_int_t fcblkidx   = fcblk_getnum(datacode, fcblk, SOLV_PROCNUM);
    pastix_int_t hcblkidx   = hcblk_getnum(datacode, hcblk);
    int ret, workerid = -1;
#ifdef STARPU_CONTEXT
    pastix_int_t sched_ctxs = starpu_loop_data->sched_ctxs;
    pastix_int_t threadid = TASK_THREADID(itertask);
    pastix_int_t thread_per_ctx = starpu_loop_data->thread_per_ctx;
    pastix_int_t my_ctx;
    my_ctx = 1+threadid/thread_per_ctx;
#endif
    assert(hcblk->procdiag  == starpu_data_get_rank(Lhalo_handle[hcblkidx]));
    assert(hcblk->procdiag < datacode->clustnbr);
    assert(fcblk->procdiag  == hcblk->procdiag);
    assert(fcblk->procdiag < datacode->clustnbr);
    assert(datacode->clustnum == starpu_data_get_rank(Lfanin_handle[fcblkidx]));
#warning "REMOVE THIS LOCK WHEN STARPU FIXED"
    pthread_mutex_lock(sopalin_data->mutex_task);
    ret =
        starpu_mpi_insert_task(sopalin_data->sopar->pastix_comm,
                               &starpu_zsyadd_cl,
                               STARPU_VALUE, &sopalin_data, sizeof(Sopalin_Data_t*),
                               STARPU_VALUE, &fcblk,        sizeof(SolverCblk*),
                               STARPU_VALUE, &hcblk,        sizeof(SolverCblk*),
                               STARPU_R,                    Lfanin_handle[fcblkidx],
                               STARPU_COMMUTE|STARPU_RW,    Lhalo_handle[hcblkidx],
                               STARPU_R, starpu_loop_data->blocktab_handles[hcblk->procdiag],
#ifdef STARPU_1_1
                               STARPU_EXECUTE_ON_WORKER, workerid,
#endif
                               STARPU_CALLBACK,     starpu_prof_callback,
                               STARPU_CALLBACK_ARG, sopalin_data->hgemm_stats,
#ifdef STARPU_CONTEXT
                               STARPU_SCHED_CTX,    sched_ctxs[my_ctx],
#endif
                               0);
#warning "REMOVE THIS UNLOCK WHEN STARPU FIXED"
    pthread_mutex_unlock(sopalin_data->mutex_task);
    STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_insert_task");
    starpu_data_unregister_submit(Lfanin_handle[fcblkidx]);
    return PASTIX_SUCCESS;
}
