/**
 *
 * @file api.c
 *
 *  PaStiX API routines
 *  PaStiX is a software package provided by Inria Bordeaux - Sud-Ouest,
 *  LaBRI, University of Bordeaux 1 and IPB.
 *
 * @version 5.1.0
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @date 2013-06-24
 *
 **/
#include "common.h"
#if defined(HAVE_METIS)
#include <metis.h>
#endif
#include "order.h"
#include "solver.h"
#include "bcsc.h"

typedef struct isched_s isched_t;

isched_t *scheduler = NULL;
isched_t *ischedInit(int cores, int *coresbind);
int ischedFinalize(isched_t *isched);

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 * @ingroup pastix_internal
 *
 * pastix_init_param - Initialize the iparm and dparm arrays to their default
 * values. This is performed only if iparm[IPARM_MODIFY_PARAMETER] is set to
 * API_NO.
 *
 *******************************************************************************
 *
 * @param[in,out] iparm
 *          The integer array of parameters to initialize.
 *
 * @param[in,out] dparm
 *          The floating point array of parameters to initialize.
 *
 *******************************************************************************/
void
pastixInitParam( pastix_int_t *iparm,
                 double       *dparm )
{
    memset( iparm, 0, IPARM_SIZE * sizeof(pastix_int_t) );
    memset( dparm, 0, DPARM_SIZE * sizeof(double) );

    iparm[IPARM_MODIFY_PARAMETER]      = API_YES;             /* Indicate if parameters have been set by user         */
    iparm[IPARM_START_TASK]            = API_TASK_ORDERING;   /* Indicate the first step to execute (see PaStiX steps)*/
    iparm[IPARM_END_TASK]              = API_TASK_CLEAN;      /* Indicate the last step to execute (see PaStiX steps) */
    iparm[IPARM_VERBOSE]               = API_VERBOSE_NO;      /* Verbose mode (see Verbose modes)                     */
    iparm[IPARM_DOF_NBR]               = 1;                   /* Degree of freedom per node                           */
    iparm[IPARM_DOF_COST]              = 0;                   /* Degree of freedom for cost computation
                                                               (If different from IPARM_DOF_NBR) */
    iparm[IPARM_ITERMAX]               = 250;                 /* Maximum iteration number for refinement              */
    iparm[IPARM_MATRIX_VERIFICATION]   = API_YES;             /* Check the input matrix                               */
    iparm[IPARM_MC64]                  = 0;                   /* MC64 operation <z_pastix.h> IGNORE                     */
    iparm[IPARM_ONLY_RAFF]             = API_NO;              /* Refinement only                                      */
    iparm[IPARM_TRACEFMT]              = API_TRACE_PAJE;      /* Trace format (see Trace modes)                       */
    iparm[IPARM_GRAPHDIST]             = API_YES;             /* UNUSED  */
    iparm[IPARM_AMALGAMATION_LEVEL]    = 5;                   /* Amalgamation level                                   */

    /**
     * Ordering parameters
     */
    iparm[IPARM_ORDERING]              = API_ORDER_SCOTCH;    /* Choose ordering                                      */
    iparm[IPARM_ORDERING_DEFAULT]      = API_YES;             /* Use default ordering parameters with scotch or metis */

    /* Scotch */
    {
        iparm[IPARM_ORDERING_SWITCH_LEVEL] = 120;    /* Ordering switch level    (see Scotch User's Guide)   */
        iparm[IPARM_ORDERING_CMIN]         = 0;      /* Ordering cmin parameter  (see Scotch User's Guide)   */
        iparm[IPARM_ORDERING_CMAX]         = 100000; /* Ordering cmax parameter  (see Scotch User's Guide)   */
        iparm[IPARM_ORDERING_FRAT]         = 8;      /* Ordering frat parameter  (see Scotch User's Guide)   */
    }

    /* Metis */
    {
#if defined(HAVE_METIS)
        iparm[IPARM_METIS_CTYPE   ] = METIS_CTYPE_SHEM;
        iparm[IPARM_METIS_RTYPE   ] = METIS_RTYPE_SEP1SIDED;
#else
        iparm[IPARM_METIS_CTYPE   ] = -1;
        iparm[IPARM_METIS_RTYPE   ] = -1;
#endif
        iparm[IPARM_METIS_NO2HOP  ] = 0;
        iparm[IPARM_METIS_NSEPS   ] = 1;
        iparm[IPARM_METIS_NITER   ] = 10;
        iparm[IPARM_METIS_UFACTOR ] = 200;
        iparm[IPARM_METIS_COMPRESS] = 1;
        iparm[IPARM_METIS_CCORDER ] = 0;
        iparm[IPARM_METIS_PFACTOR ] = 0;
        iparm[IPARM_METIS_SEED    ] = 3452;
        iparm[IPARM_METIS_DBGLVL  ] = 0;
    }

    /**
     * End of ordering parameters
     ******************************************/

    iparm[IPARM_STATIC_PIVOTING]       = 0;                   /* number of control of diagonal magnitude              */
    iparm[IPARM_NNZEROS]               = 0;                   /* memory space for coefficients                        */
    iparm[IPARM_ALLOCATED_TERMS]       = 0;                   /* number of non zero in factorized sparse matrix       */
    iparm[IPARM_MIN_BLOCKSIZE]         = 60;                  /* min blocksize                                        */
    iparm[IPARM_MAX_BLOCKSIZE]         = 120;                 /* max blocksize                                        */
    iparm[IPARM_SCHUR]                 = API_NO;              /* Schur mode */
    iparm[IPARM_ISOLATE_ZEROS]         = API_NO;              /* Isolate null diagonal terms at the end of the matrix */
    iparm[IPARM_FACTORIZATION]         = API_FACT_LDLT;       /* LdLt     */
    iparm[IPARM_CPU_BY_NODE]           = 0;                   /* cpu/node */
    iparm[IPARM_BINDTHRD]              = API_BIND_AUTO;       /* Default binding method */
    iparm[IPARM_THREAD_NBR]            = 1;                   /* thread/mpi */
    iparm[IPARM_CUDA_NBR]              = 0;                   /* CUDA devices */
    iparm[IPARM_DISTRIBUTION_LEVEL]    = 0;                   /* 1d / 2d */
    iparm[IPARM_LEVEL_OF_FILL]         = 0;                   /* level of fill */
    iparm[IPARM_IO_STRATEGY]           = API_IO_NO;           /* I/O */
    iparm[IPARM_RHS_MAKING]            = API_RHS_B;           /* generate rhs */
    iparm[IPARM_REFINEMENT]            = API_RAF_GMRES;       /* gmres */
    iparm[IPARM_SYM]                   = API_SYM_YES;         /* Symmetric */
    iparm[IPARM_INCOMPLETE]            = API_NO;              /* direct */
    iparm[IPARM_ABS]                   = 1;                   /* ABS level to 1 */
    iparm[IPARM_ESP]                   = API_NO;              /* no esp */
#ifdef OOC
    iparm[IPARM_GMRES_IM]              = 1;                   /* gmres_im */
    iparm[IPARM_ITERMAX]               = 1;
#else
    iparm[IPARM_GMRES_IM]              = 25;                  /* gmres_im */
#endif
    iparm[IPARM_FREE_CSCUSER]          = API_CSC_PRESERVE;    /* Free user csc after coefinit */
    iparm[IPARM_FREE_CSCPASTIX]        = API_CSC_PRESERVE;    /* Free internal csc after coefinit */
    iparm[IPARM_OOC_LIMIT]             = 2000;                /* memory limit */
    iparm[IPARM_OOC_THREAD]            = 1;                   /* ooc thrdnbr */
    iparm[IPARM_OOC_ID]                = -1;                  /* Out of core run ID */
    iparm[IPARM_NB_SMP_NODE_USED]      = 0;                   /* Nb SMP node used (0 for 1 per MPI process) */
    iparm[IPARM_MURGE_REFINEMENT]      = API_YES;
    iparm[IPARM_TRANSPOSE_SOLVE]       = API_NO;


    /**
     * Communication modes
     */
    iparm[IPARM_THREAD_COMM_MODE] = 0;
#if defined(PASTIX_WITH_MPI)
    {
        int flag = 0;
        int provided = MPI_THREAD_SINGLE;
        MPI_Initialized(&flag);

        if (flag) {
            MPI_Query_thread(&provided);
            switch( provided ) {
            case MPI_THREAD_MULTIPLE:
                iparm[IPARM_THREAD_COMM_MODE] = API_THREAD_MULTIPLE;
                break;
            case MPI_THREAD_SERIALIZED:
            case MPI_THREAD_FUNNELED:
                iparm[IPARM_THREAD_COMM_MODE] = API_THREAD_FUNNELED;
                break;
                /**
                 * In the folowing cases, we consider that any MPI implementation
                 * should provide enough level of parallelism to turn in Funneled mode
                 */
            case MPI_THREAD_SINGLE:
            default:
                iparm[IPARM_THREAD_COMM_MODE] = API_THREAD_FUNNELED;
            }
        }
    }
#endif /* defined(PASTIX_WITH_MPI) */


    iparm[IPARM_NB_THREAD_COMM]     = 1;                   /* Nb thread quand iparm[IPARM_THREAD_COMM_MODE] == API_THCOMM_DEFINED */
    iparm[IPARM_FILL_MATRIX]        = API_NO;              /* fill matrix */
    iparm[IPARM_INERTIA]            = -1;
    iparm[IPARM_ESP_NBTASKS]        = -1;
    iparm[IPARM_ESP_THRESHOLD]      = 16384;               /* Taille de bloc minimale pour passer en esp (2**14) = 128 * 128 */
    iparm[IPARM_ERROR_NUMBER]       = PASTIX_SUCCESS;
    iparm[IPARM_RHSD_CHECK]         = API_YES;
    iparm[IPARM_STARPU]             = API_NO;
    iparm[IPARM_AUTOSPLIT_COMM]     = API_NO;
    iparm[IPARM_FLOAT]              = API_REALDOUBLE;
    iparm[IPARM_STARPU_CTX_DEPTH]   = 3;
    iparm[IPARM_STARPU_CTX_NBR]     = -1;
    iparm[IPARM_PRODUCE_STATS]      = API_NO;

    dparm[DPARM_EPSILON_REFINEMENT] = 1e-12;
    dparm[DPARM_RELATIVE_ERROR]     = -1.;
    dparm[DPARM_SCALED_RESIDUAL]    = -1.;
    dparm[DPARM_EPSILON_MAGN_CTRL]  = 1e-31;
    dparm[DPARM_FACT_FLOPS]         = 0.;
    dparm[DPARM_SOLV_FLOPS]         = 0.;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 *
 * pastixInit - Initialize the iparm and dparm arrays to their default
 * values. This is performed only if iparm[IPARM_MODIFY_PARAMETER] is set to
 * API_NO.
 *
 *******************************************************************************
 *
 * @param[in,out] iparm
 *          The integer array of parameters to initialize.
 *
 * @param[in,out] dparm
 *          The floating point array of parameters to initialize.
 *
 *******************************************************************************/
void
pastixInit( pastix_data_t **pastix_data,
            MPI_Comm        pastix_comm,
            pastix_int_t   *iparm,
            double         *dparm )
{
    pastix_data_t *pastix;

    /**
     * Check if MPI is initialized
     */
#if defined(PASTIX_WITH_MPI)
    {
        int provided = MPI_THREAD_SINGLE;
        int flag = 0;
        MPI_Initialized(&flag);
        if ( !flag ) {
            MPI_Init_thread( NULL, NULL, MPI_THREAD_MULTIPLE, &provided );
        }
    }
#endif

    /**
     * Allocate pastix_data structure when we enter PaStiX for the first time.
     */
    MALLOC_INTERN(pastix, 1, pastix_data_t);
    memset( pastix, 0, sizeof(pastix_data_t) );

    /**
     * Initialize iparm/dparm vectors and set them to default values if not set
     * by the user.
     */
    if ( iparm[IPARM_MODIFY_PARAMETER] == API_NO ) {
        pastixInitParam( iparm, dparm );
    }
    pastix->iparm = iparm;
    pastix->dparm = dparm;

    pastix->graph      = NULL;
    pastix->schur_n    = 0;
    pastix->schur_list = NULL;
    pastix->zeros_n    = 0;
    pastix->zeros_list = NULL;
    pastix->ordemesh   = NULL;
    pastix->symbmtx    = NULL;

    pastix->gN = -1;
    pastix->n2 = -1;
    pastix->solvmatr   = NULL;
    pastix->bcsc       = NULL;

    /**
     * Setup all communicators for autosplitmode and initialize number/rank of
     * processes.
     */
    pastix->pastix_comm = pastix_comm;
    MPI_Comm_size(pastix_comm, &(pastix->procnbr));
    MPI_Comm_rank(pastix_comm, &(pastix->procnum));

    if (iparm[IPARM_AUTOSPLIT_COMM] == API_YES)
    {
        int     i, len;
        char    procname[MPI_MAX_PROCESSOR_NAME];
        int     rc, key = pastix->procnum;
        int64_t color;
        (void)rc;

        /**
         * Get hostname to generate a hash that will be the color of each node
         * MPI_Get_processor_name is not used as it can returned different
         * strings for processes of a same physical node.
         */
        rc = gethostname(procname, MPI_MAX_PROCESSOR_NAME-1);
        assert(rc == 0);
        procname[MPI_MAX_PROCESSOR_NAME-1] = '\0';
        len = strlen( procname );

        /* Compute hash */
        color = 0;
        for (i = 0; i < len; i++) {
            color = color*256*sizeof(char) + procname[i];
        }

        /* Create intra-node communicator */
        MPI_Comm_split(pastix_comm, color, key, &(pastix->intra_node_comm));
        MPI_Comm_size(pastix->intra_node_comm, &(pastix->intra_node_procnbr));
        MPI_Comm_rank(pastix->intra_node_comm, &(pastix->intra_node_procnum));

        /* Create inter-node communicator */
        MPI_Comm_split(pastix_comm, pastix->intra_node_procnum, key, &(pastix->inter_node_comm));
        MPI_Comm_size(pastix->inter_node_comm, &(pastix->inter_node_procnbr));
        MPI_Comm_rank(pastix->inter_node_comm, &(pastix->inter_node_procnum));
    }
    else
    {
        pastix->intra_node_comm = MPI_COMM_SELF;
        pastix->intra_node_procnbr = 1;
        pastix->intra_node_procnum = 0;
        pastix->inter_node_comm = pastix_comm;
        pastix->inter_node_procnbr = pastix->procnbr;
        pastix->inter_node_procnum = pastix->procnum;
    }

    iparm[IPARM_THREAD_NBR] = pastix->intra_node_procnbr;

    if (pastix->procnum == 0)
    {
        pastix->pastix_id = getpid();
    }
    MPI_Bcast(&(pastix->pastix_id), 1, PASTIX_MPI_INT, 0, pastix_comm);

#ifdef WITH_SEM_BARRIER
    if (pastix->intra_node_procnbr > 1)
    {
        char sem_name[256];
        sprintf(sem_name, "/pastix_%d", pastix->pastix_id);
        OPEN_SEM(pastix->sem_barrier, sem_name, 0);
    }
#endif

    if (iparm != NULL)
    {
        if (iparm[IPARM_VERBOSE] > API_VERBOSE_NO)
        {
            fprintf(stdout, "AUTOSPLIT_COMM : global rank : %d,"
                    " inter node rank %d,"
                    " intra node rank %d, threads %d\n",
                    (int)(pastix->procnum),
                    (int)(pastix->inter_node_procnum),
                    (int)(pastix->intra_node_procnum),
                    (int)iparm[IPARM_THREAD_NBR]);
        }

        iparm[IPARM_PID] = pastix->pastix_id;
    }




    pastix->sopar.bindtab    = NULL;
    pastix->sopar.b          = NULL;
    pastix->sopar.transcsc   = NULL;
    pastix->sopar.stopthrd   = API_NO;
    pastix->bindtab          = NULL;
    pastix->cscInternFilled  = API_NO;

#ifdef PASTIX_DISTRIBUTED
    pastix->malrhsd_int      = API_NO;
    pastix->l2g_int          = NULL;
    pastix->mal_l2g_int      = API_NO;
    pastix->glob2loc         = NULL;
    pastix->PTS_permtab      = NULL;
    pastix->PTS_peritab      = NULL;
#endif
    pastix->schur_tab        = NULL;
    pastix->schur_tab_set    = API_NO;
    pastix->scaling  = API_NO;

    /* DIRTY Initialization for Scotch */
    srand(1);

    /* Environement variables */
    /* On Mac set VECLIB_MAXIMUM_THREADS if not setted */
    setenv("VECLIB_MAXIMUM_THREADS", "1", 0);

    // TODO
    //z_pastix_welcome_print(*pastix_data, colptr, n);

    /* Initialization step done, overwrite anything done before */
    pastix->steps = STEP_INIT;

    scheduler = ischedInit( -1, NULL );

    *pastix_data = pastix;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 *
 * pastixFinalize -
 *
 *******************************************************************************
 *
 * @param[in,out] iparm
 *          The integer array of parameters to initialize.
 *
 * @param[in,out] dparm
 *          The floating point array of parameters to initialize.
 *
 *******************************************************************************/
void
pastixFinalize( pastix_data_t **pastix_data,
                MPI_Comm        pastix_comm,
                pastix_int_t   *iparm,
                double         *dparm )
{
    pastix_data_t *pastix = *pastix_data;
    (void)pastix_comm; (void)iparm; (void)dparm;

    ischedFinalize( scheduler );

    if ( pastix->graph != NULL )
    {
        graphExit( pastix->graph );
        memFree_null( pastix->graph );
    }

    if ( pastix->ordemesh != NULL )
    {
        orderExit( pastix->ordemesh );
        memFree_null( pastix->ordemesh );
    }

    if ( pastix->symbmtx != NULL )
    {
        symbolExit( pastix->symbmtx );
        memFree_null( pastix->symbmtx );
    }

    if ( pastix->solvmatr != NULL )
    {
        SolverMatrix *solvmatr = pastix->solvmatr;
        pastix_int_t i;

        /* TODO: move following block to solverExit */
        {
            if (solvmatr->updovct.cblktab) {
                for (i=0; i<solvmatr->cblknbr; i++)
                {
                    if (solvmatr->updovct.cblktab[i].browcblktab)
                        memFree_null(solvmatr->updovct.cblktab[i].browcblktab);
                    if (solvmatr->updovct.cblktab[i].browproctab)
                        memFree_null(solvmatr->updovct.cblktab[i].browproctab);
                }
            }
            memFree_null(solvmatr->updovct.lblk2gcblk);
            memFree_null(solvmatr->updovct.listblok);
            memFree_null(solvmatr->updovct.listcblk);
            memFree_null(solvmatr->updovct.gcblk2list);
            memFree_null(solvmatr->updovct.loc2glob);
            memFree_null(solvmatr->updovct.cblktab);
            memFree_null(solvmatr->updovct.listptr);
        }

        solverExit( pastix->solvmatr );
        memFree_null( pastix->solvmatr );
    }

    if ( pastix->bcsc != NULL )
    {
        bcscExit( pastix->bcsc );
        memFree_null( pastix->bcsc );
    }
    memFree_null(*pastix_data);
}
