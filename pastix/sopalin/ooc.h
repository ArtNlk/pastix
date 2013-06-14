#ifndef OOC_H
#define OOC_H

/* Return values */
#define EXIT_FAILURE_CBLK_NOT_NULL      3
#define EXIT_FAILURE_SAVING_NULL_BUFFER 4
#define EXIT_FAILURE_OUT_OF_MEMORY      5
#define EXIT_FAILURE_FILE_OPENING       6
#define EXIT_FAILURE_FILE_TRUNCATED     7
#define EXIT_SUCCESS_HACK               8
#define EXIT_SUCCESS_ALL_LOADED         9
#define EXIT_FAILURE_CBLK_USED          10

/* OOC Step */
#define OOCSTEP_COEFINIT                1
#define OOCSTEP_SOPALIN                 2
#define OOCSTEP_DOWN                    3
#define OOCSTEP_DIAG                    4
#define OOCSTEP_UP                      5


#ifdef OOC

#define OOC_RECEIVING ooc_receiving(sopalin_data)
#define OOC_RECEIVED ooc_received(sopalin_data)
#define OOC_THREAD_NBR sopalin_data->sopar->iparm[IPARM_OOC_THREAD]

/* sets values for the global ooc structure 
 * sopalin_data : Sopalin_Data_t global structure
 * limit        : memory limit set by use
 */
void *ooc_thread(void * arg);

/* Init / Clean */
int ooc_init          (Sopalin_Data_t * sopalin_data, pastix_int_t limit);
int ooc_exit          (Sopalin_Data_t * sopalin_data);

/* Step */
int ooc_stop_thread   (Sopalin_Data_t * sopalin_data);
int ooc_freeze        (Sopalin_Data_t * sopalin_data);
int ooc_defreeze      (Sopalin_Data_t * sopalin_data);
int ooc_set_step      (Sopalin_Data_t * sopalin_data, int step);

/* Cblk */
int ooc_wait_for_cblk (Sopalin_Data_t * sopalin_data, pastix_int_t cblk, int me);
int ooc_hack_load     (Sopalin_Data_t * sopalin_data, pastix_int_t cblk, int me);
int ooc_save_coef     (Sopalin_Data_t * sopalin_data, pastix_int_t task, pastix_int_t cblk, int me);

void ooc_receiving    (Sopalin_Data_t * sopalin_data);
void ooc_received     (Sopalin_Data_t * sopalin_data);
void ooc_wait_task    (Sopalin_Data_t * sopalin_data, pastix_int_t task, int me);
#else /* OOC */

#define OOC_RECEIVING 
#define OOC_RECEIVED 
#define OOC_THREAD_NBR 0
#define ooc_thread     NULL

#define ooc_init(sopalin_data, limit)
#define ooc_exit(sopalin_data)

#define ooc_stop_thread(sopalin_data)
#define ooc_freeze(sopalin_data)
#define ooc_defreeze(sopalin_data)
#define ooc_set_step(sopalin_data, step)

#define ooc_wait_for_cblk(sopalin_data, cblk, me)
#define ooc_save_coef(sopalin_data, task, cblk, me)
#define ooc_hack_load(sopalin_data, cblknum, me)

#define ooc_wait_task(sopalin_data, task, me) 

#endif /* OOC */

#ifdef OOC_FTGT
/* Ftgt */
int ooc_wait_for_ftgt (Sopalin_Data_t * sopalin_data, pastix_int_t ftgtnum, int me);
int ooc_reset_ftgt    (Sopalin_Data_t * sopalin_data, pastix_int_t ftgtnum, int me);
int ooc_save_ftgt     (Sopalin_Data_t * sopalin_data, pastix_int_t tasknum, pastix_int_t ftgtnum, int me);

#else

#define ooc_wait_for_ftgt(sopalin_data, ftgtnum, me)
#define ooc_reset_ftgt(sopalin_data, ftgtnum, me)
#define ooc_save_ftgt(sopalin_data, tasknum, ftgtnum, me)
#endif

#endif /* OOC_H */
