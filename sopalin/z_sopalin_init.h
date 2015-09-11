/**
 *
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 1.0.0
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @date 2011-11-11
 * @precisions normal z -> c d s
 *
 **/
#ifndef Z_SOPALIN_INIT_H
#define Z_SOPALIN_INIT_H

#include "z_sopalin3d.h"
/* Type of thread asking for allocation (must be a power of 2) */
#define INIT_COMPUTE   1
#define INIT_SEND      2
#define INIT_RECV      4
#define INIT_OOC       8

/************************************************/
/*     Structure pour le backup des donn�es     */
/************************************************/



/* Allocate and initialize/Free globale data for z_solver */
void z_sopalin_init     (z_Sopalin_Data_t *sopalin_data, z_SolverMatrix *m, z_SopalinParam *sopaparam, int fact);
void z_sopalin_clean    (z_Sopalin_Data_t *sopalin_data, int step);

/* Allocate and initialize/Free thread data for z_solver */
void z_sopalin_init_smp (z_Sopalin_Data_t *sopalin_data, pastix_int_t me, int fact, int thrdtype);
void z_sopalin_clean_smp(z_Sopalin_Data_t *sopalin_data, pastix_int_t me);

/* Restore/backup des donn�es modifi�es pendant l'encha�nement facto/solve */
void z_sopalin_backup (z_SolverMatrix *datacode, z_Backup *b);
void z_sopalin_restore(z_SolverMatrix *datacode, z_Backup *b);

/* Restore/backup des donn�es modifi�es pendant le solve */
void z_solve_backup (z_SolverMatrix *datacode, BackupSolve_t *b);
void z_solve_restore(z_SolverMatrix *datacode, BackupSolve_t *b);

#if (defined PASTIX_DYNSCHED && !(defined PASTIX_DYNSCHED_WITH_TREE))
#  define z_tabtravel_init   PASTIX_PREFIX_F(z_tabtravel_init)
#  define z_tabtravel_deinit PASTIX_PREFIX_F(z_tabtravel_deinit)
static inline
int z_tabtravel_init(z_Sopalin_Data_t * sopalin_data,
                   z_Thread_Data_t  * thread_data,
                   int              me);
static inline
int z_tabtravel_deinit(z_Thread_Data_t * thread_data);
#endif /* (PASTIX_DYNSCHED && !(defined PASTIX_DYNSCHED_WITH_TREE)) */

#endif /* Z_SOPALIN_INIT_H */
