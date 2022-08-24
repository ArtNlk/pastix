/**
 *
 * @file pastixdata.h
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Tony Delarue
 * @date 2021-07-02
 *
 **/
#ifndef _pastixdata_h_
#define _pastixdata_h_

#include "common/isched.h"
#include "symbol/symbol.h"
#include "kernels/queue.h"

/*
 * Steps of the pastix solver
 */
#define STEP_INIT      (1 << 0)
#define STEP_ORDERING  (1 << 1)
#define STEP_SYMBFACT  (1 << 2)
#define STEP_ANALYSE   (1 << 3)
#define STEP_CSC2BCSC  (1 << 4)
#define STEP_BCSC2CTAB (1 << 5)
#define STEP_NUMFACT   (1 << 6)
#define STEP_SOLVE     (1 << 7)
#define STEP_REFINE    (1 << 8)

/*
 * Scheduler family
 */
#define PASTIX_SCHED_FAMILY_RUNTIME ((1 << PastixSchedParsec) | (1 << PastixSchedStarPU))
#define PASTIX_SCHED_FAMILY_PTHREAD ((1 << PastixSchedSequential) | (1 << PastixSchedStatic) | (1 << PastixSchedDynamic))

#define isSchedRuntime( _runtime_ ) ( (1 << (_runtime_)) & PASTIX_SCHED_FAMILY_RUNTIME )
#define isSchedPthread( _runtime_ ) ( (1 << (_runtime_)) & PASTIX_SCHED_FAMILY_PTHREAD )
#define isSchedValid( _runtime_, _family_ )  ( (1 << (_runtime_)) & (_family_) )

struct pastix_bcsc_s;
typedef struct pastix_bcsc_s pastix_bcsc_t;

struct pastix_model_s;
typedef struct pastix_model_s pastix_model_t;

/**
 *
 * @ingroup pastix_users
 *
 * @brief Main PaStiX data structure
 *
 * This structure holds all informations related to the library and problem
 * instance. It stores information from one step to another.
 * @warning This structure should not be modified directly by the user.
 *
 */
struct pastix_data_s {
    pastix_int_t     id;                 /**< Unique identifier of the pastix instance                            */
    pastix_int_t    *iparm;              /**< Store integer parameters (input/output)                             */
    double          *dparm;              /**< Store floating parameters (input/output)                            */

    pastix_int_t       steps;            /**< Bitmask of the steps performed or not                               */
    pastix_scheduler_t sched;            /**< Indicates the scheduler used for the factorization step             */

    PASTIX_Comm      pastix_comm;        /**< PaStiX MPI communicator used for the ordering step                  */
    PASTIX_Comm      intra_node_comm;    /**< PaStiX intra node MPI communicator used for synchronizations        */
    PASTIX_Comm      inter_node_comm;    /**< PaStiX inter node MPI communicator used for the factorization       */
    int              procnbr;            /**< Total number of MPI processes                                       */
    int              procnum;            /**< Local MPI rank                                                      */
    int              intra_node_procnbr; /**< Number of MPI tasks in intra node communicator                      */
    int              intra_node_procnum; /**< Local MPI rank in intra node communicator                           */
    int              inter_node_procnbr; /**< Number of MPI tasks in inter node communicator                      */
    int              inter_node_procnum; /**< Local MPI rank in inter node communicator                           */

    isched_t        *isched;             /**< Internal scheduler structure that is always available               */
    void            *parsec;             /**< PaRSEC context if available                                         */
    void            *starpu;             /**< StarPU context if available                                         */

    const spmatrix_t *csc;               /**< Pointer to the user csc structure used as input                     */

    pastix_graph_t  *graph;              /**< Symmetrized graph of the problem used within ordering
                                              and symbolic factorization steps.                                   */
    pastix_int_t     schur_n;            /**< Number of entries for the Schur complement                          */
    pastix_int_t    *schur_list;         /**< List of entries for the schur complement                            */
    pastix_int_t     zeros_n;            /**< Number of diagonal entries considered as zeros                      */
    pastix_int_t    *zeros_list;         /**< List of diagonal entries considered as zeros                        */
    pastix_order_t  *ordemesh;           /**< Ordering structure                                                  */

    symbol_matrix_t *symbmtx;            /**< Symbol Matrix                                                       */

    pastix_bcsc_t   *bcsc;               /**< Csc after reordering grouped by cblk                                */
    SolverMatrix    *solvmatr;           /**< Solver informations associated to the matrix problem                */
    SolverMatrix    *solvloc;            /**< Solver informations associated to the matrix problem - Local        */
    SolverMatrix    *solvglob;           /**< Solver informations associated to the matrix problem - Global       */

    pastix_model_t  *cpu_models;         /**< CPU model coefficients for the kernels                              */
    pastix_model_t  *gpu_models;         /**< GPU model coefficients for the kernels                              */

    char            *dir_global;         /**< Unique directory name to store output files                         */
    char            *dir_local;          /**< Unique directory name to store output specific to a MPI process     */

    /* Backup for old pastix interface */
    void            *b;
    void            *x0;

    /**
     * Former fields that are no longer used for now
     */
    int             *bindtab;            /*+ Tabular giving for each thread a CPU to bind it too                 */
    void            *schur_tab;
    pastix_int_t     schur_tab_set;
    int              cscInternFilled;
    int              scaling;            /*+ Indicates if the matrix has been scaled                             */
    void  *scalerowtab;        /*+ Describes how the matrix has been scaled                            */
    void  *iscalerowtab;
    void  *scalecoltab;
    void  *iscalecoltab;
#ifdef WITH_SEM_BARRIER
    sem_t           *sem_barrier;        /*+ Semaphore used for AUTOSPLIT_COMM barrier                           */
#endif
    pastix_int_t     pastix_id;          /*+ Id of the pastix instance (PID of first MPI task)                   */
};

#endif /* _pastixdata_h_ */
