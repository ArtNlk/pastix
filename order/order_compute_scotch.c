/**
 *
 * @file order_compute_scotch.c
 *
 * PaStiX order driver to perform ordering with Scotch library.
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Xavier Lacoste
 * @author Pierre Ramet
 * @author Mathieu Faverge
 * @author Tony Delarue
 * @author Vincent Bridonneau
 * @date 2021-06-16
 *
 **/
#include "common.h"
#include "graph/graph.h"
#include "pastix/order.h"
#if defined(PASTIX_ORDERING_PTSCOTCH)
#include <ptscotch.h>
#elif defined(PASTIX_ORDERING_SCOTCH)
#include <scotch.h>
#endif /* defined(PASTIX_ORDERING_PTSCOTCH) */
#include "order_scotch_strats.h"

#define STRAT_STR_MAX 1024

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Build the vertex weight array out of the dof array.
 *
 *******************************************************************************
 *
 * @param[in] graph
 *          Pointer to the graph structure.
 *
 *******************************************************************************
 *
 * @retval The vertex weight array if graph->dof != 1, NULL otherwise.
 *
 *******************************************************************************/
static inline pastix_int_t *
ocs_build_weights( const pastix_graph_t *graph )
{
    pastix_int_t  i, n;
    pastix_int_t *weights, *wptr;

    if ( graph->dof == 1 ) {
        return NULL;
    }

    n = graph->n;
    MALLOC_INTERN( weights, n, pastix_int_t );

    wptr = weights;
    /* Constant dof */
    if ( graph->dof > 1 ) {
        for (i = 0; i < n; i++, wptr++ ) {
            *wptr = graph->dof;
        }
    }
    /* Variadic dof */
    else {
        pastix_int_t *dofptr = graph->dofs;

        for (i = 0; i < n; i++, wptr++, dofptr++) {
            *wptr = dofptr[1] - dofptr[0];
        }
    }

    return weights;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Check the generated Scotch graph.
 *
 *******************************************************************************
 *
 * @param[in] graph
 *          Pointer to the SCOTCH_Graph structure.
 *
 * @param[in] procnum
 *          Procnum of the process. Output purpose.
 *
 *******************************************************************************/
static inline void
ocs_graphcheck( const SCOTCH_Graph *graph,
                pastix_int_t        procnum )
{
#if defined(PASTIX_DEBUG_ORDERING)
    Clock timer;
    clockStart(timer);
    if( SCOTCH_graphCheck(graph) ) {
        errorPrint("pastix: graphCheck");
        EXIT(MOD_SOPALIN,INTERNAL_ERR);
    }
    clockStop(timer);
    pastix_print( procnum, 0, "SCOTCH_graphCheck done in %lf second\n", clockVal(timer) );
#else
    (void)graph;
    (void)procnum;
#endif /* defined(PASTIX_DEBUG_ORDERING) */
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Build the Scotch graph.
 *
 *******************************************************************************
 *
 * @param[inout] scotchgraph
 *          The Scotch graph structure that will be build.
 *
 * @param[inout] graph
 *          The graph prepared by graphPrepare function on which we want to
 *          perform the ordering.
 *
 *******************************************************************************/
static inline void
ocs_scotchgraph_init( SCOTCH_Graph   *scotchgraph,
                      pastix_graph_t *graph )
{
    pastix_int_t *colptr;
    pastix_int_t *rows;
    pastix_int_t  n;
    pastix_int_t  baseval;
    pastix_int_t  nnz;
    pastix_int_t *weights;

#if 0
    /* Distributed */
    if (iparm[IPARM_GRAPHDIST] != 0)
    {
        cscd2csc_int( graph->n,
                      graph->colptr,
                      graph->rowptr,
                      NULL, NULL, NULL, NULL,
                      &n, &colptr, &rows,
                      NULL, NULL, NULL, NULL,
                      graph->loc2glob,
                      pastix_data->pastix_comm,
                      0, /* DoF to 0 as we have no values */
                      1);
    }
    else
#endif
    /* Centralized */
    {
        n       = graph->n;
        colptr  = graph->colptr;
        rows    = graph->rowptr;
        baseval = colptr[0];
        nnz     = colptr[n] - baseval;
        weights = NULL;
    }

    SCOTCH_graphInit( scotchgraph );

    /*
     * Generate the vertex load array if dof != 1
     */
    weights = ocs_build_weights( graph );

    if ( SCOTCH_graphBuild( scotchgraph,    /* Graph to build     */
                            baseval,        /* baseval            */
                            n,              /* Number of vertices */
                            colptr,         /* Vertex array       */
                            NULL,
                            weights,        /* Array of vertex weights (DOFs) */
                            NULL,
                            nnz,            /* Number of arcs     */
                            rows,           /* Edge array         */
                            NULL) )
    {
        errorPrint("pastix : graphBuildGraph");
        EXIT(MOD_SOPALIN,PASTIX_ERR_INTERNAL);
    }

    SCOTCH_graphBase( scotchgraph, 0 );

    /* Check the generated Scotch graph structure */
    ocs_graphcheck( scotchgraph, 0 );

    return;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Cleanup the Scotch graph structure and free the associated data.
 *
 *******************************************************************************
 *
 * @param[inout] scotchgraph
 *          The Scotch graph structure that will be build.
 *
 *******************************************************************************/
static inline void
ocs_scotchgraph_exit( SCOTCH_Graph *scotchgraph,
                      pastix_int_t  baseval )
{
    pastix_int_t *colptr  = NULL;
    pastix_int_t *rows    = NULL;
    pastix_int_t *weights = NULL;

    /*
     * SCOTCH_graphBase may have had side effects on our graph.
     * Call this routine again with our saved baseval.
     */
    SCOTCH_graphBase( scotchgraph, baseval );

    SCOTCH_graphData (
        scotchgraph,
        NULL,     /* Base value               */
        NULL,     /* Number of vertices       */
        &colptr,  /* Vertex array [vertnbr+1] */
        NULL,     /* Vertex array [vertnbr]   */
        &weights, /* Vertex load array        */
        NULL,     /* Vertex label array       */
        NULL,     /* Number of edges (arcs)   */
        &rows,    /* Edge array [edgenbr]     */
        NULL      /* Edge load array          */  );

#if 0
    if ( iparm[IPARM_GRAPHDIST] == 1 ) {
        memFree_null( colptr );
        memFree_null( rows );
    }
#endif

    /* Free the vertex load array */
    if ( weights != NULL ) {
        memFree_null( weights );
    }

    SCOTCH_graphExit( scotchgraph );

    return;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Generate the ordering strategy string based on the input parameters.
 *
 *******************************************************************************
 *
 * @param[inout] strat
 *          The preallocated ordering strategy string to initialize.
 *
 * @param[in] iparm
 *          Pointer to the iparm array.
 *
 * @param[in] procnum
 *          Procnum of the process. Output purpose.
 *
 *******************************************************************************/
static inline void
ocs_build_strategy( char               *strat,
                    const pastix_int_t *iparm,
                    pastix_int_t        procnum )
{
    int rc;

    /* Default ordering */
    if (iparm[IPARM_ORDERING_DEFAULT] == 1) {
        if (iparm[IPARM_INCOMPLETE] == 0) {
            if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
                pastix_print( procnum, 0,
                              "      Scotch direct strategy\n" );
            }
            snprintf( strat, STRAT_STR_MAX, SCOTCH_STRAT_DIRECT );
        }
        else {
            if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
                pastix_print( procnum, 0,
                              "      Scotch incomplete strategy\n" );
            }
            snprintf(strat, STRAT_STR_MAX, SCOTCH_STRAT_INCOMP );
        }
    }
    /* Personal ordering */
    else {
        rc = snprintf( strat, STRAT_STR_MAX, SCOTCH_STRAT_PERSO,
                       (long)  iparm[IPARM_SCOTCH_SWITCH_LEVEL],
                       (long)  iparm[IPARM_SCOTCH_CMIN],
                       (long)  iparm[IPARM_SCOTCH_CMAX],
                       ((float)iparm[IPARM_SCOTCH_FRAT])/100.,
                       (long)  iparm[IPARM_SCOTCH_SWITCH_LEVEL],
                       (long)  iparm[IPARM_SCOTCH_CMIN],
                       (long)  iparm[IPARM_SCOTCH_CMAX],
                       ((float)iparm[IPARM_SCOTCH_FRAT])/100. );
        if ( rc > STRAT_STR_MAX ) {
            pastix_print_error( "order_compute_scotch: Strategy string too long\n" );
            exit(-1);
        }

        if (iparm[IPARM_VERBOSE] > PastixVerboseNo) {
            pastix_print( procnum, 0,
                          "Scotch personal strategy |%s|\n", strat );
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Reallocate the ordering structure.
 *
 * If we decide to drop the Scothc partition to recompute it later, then
 * partition information is freed, otherwise its memory space is compressed.
 *
 *******************************************************************************
 *
 * @param[inout] ordemesh
 *          Pointer to the ordemesh structure to reallocate.
 *
 *******************************************************************************/
static inline void
ocs_reallocate_ordemesh( pastix_order_t *ordemesh )
{
#if defined(FORGET_PARTITION)
    ordemesh->cblknbr = 0;
    if (ordemesh->rangtab != NULL) {
        memFree_null(ordemesh->rangtab);
    }
    if (ordemesh->treetab != NULL) {
        memFree_null(ordemesh->treetab);
    }
#else
    /**
     * Adapt size of rangtab and treetab to the new cblknbr
     * WARNING: If no nodes in the graph, nothing has been initialized.
     */
    ordemesh->rangtab =
        (pastix_int_t *) memRealloc( ordemesh->rangtab,
                                    (ordemesh->cblknbr + 1)*sizeof(pastix_int_t) );
    ordemesh->treetab =
        (pastix_int_t *) memRealloc( ordemesh->treetab,
                                    (ordemesh->cblknbr)*sizeof(pastix_int_t) );
    if (ordemesh->cblknbr == 0) {
        ordemesh->rangtab[0] = ordemesh->baseval;
    }
#endif
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Compute the graph ordering
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          Pointer to the pastix_data instance.
 *
 * @param[inout] scotchgraph
 *          The Scotch graph structure that will be ordered.
 *
 ********************************************************************************
 *
 * @retval The return value of SCOTCH_graphOrderList.
 *
 *******************************************************************************/
static inline int
ocs_compute_graph_ordering( pastix_data_t  *pastix_data,
                            SCOTCH_Graph   *scotchgraph )
{
    pastix_order_t *ordemesh = pastix_data->ordemesh;
    SCOTCH_Strat stratdat;
    int  ret;
    char strat[STRAT_STR_MAX];

    /* Create Strategy string for Scotch */
    SCOTCH_stratInit( &stratdat );
    ocs_build_strategy( strat, pastix_data->iparm, pastix_data->procnum );

    /* Make sure the call to flex/yacc is serialized thanks to a global lock */
    {
        static volatile pastix_atomic_lock_t strat_lock = PASTIX_ATOMIC_UNLOCKED;
        pastix_atomic_lock( &strat_lock );
        ret = SCOTCH_stratGraphOrder( &stratdat, strat );
        pastix_atomic_unlock( &strat_lock );
    }

    if (ret == 0) {
        /* Compute graph ordering memory */
        ret = SCOTCH_graphOrderList( scotchgraph,
                                     (SCOTCH_Num)   ordemesh->vertnbr,
                                     (SCOTCH_Num *) NULL,
                                     &stratdat,
                                     (SCOTCH_Num *) ordemesh->permtab,
                                     (SCOTCH_Num *) ordemesh->peritab,
                                     (SCOTCH_Num *)&ordemesh->cblknbr,
                                     (SCOTCH_Num *) ordemesh->rangtab,
                                     (SCOTCH_Num *) ordemesh->treetab );
    }
    SCOTCH_stratExit( &stratdat );

    return ret;
}

#if defined(PASTIX_ORDERING_SCOTCH_MT)
struct args_ocs_mt {
    pastix_data_t  *pastix_data;
    SCOTCH_Context *scotch_ctx;
    SCOTCH_Graph   *scotch_grf;
    int             ret;
};

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Compute the ordering step in shared memory.
 *
 *******************************************************************************
 *
 * @param[in] ctx
 *          The pastix shared memory context
 *
 * @param[inout] args
 *          Shared args for the computation
 *
 *******************************************************************************/
static inline void
ocs_compute_graph_ordering_mt( isched_thread_t *ctx, void *args )
{
    struct args_ocs_mt *arg         = (struct args_ocs_mt *)args;
    pastix_data_t      *pastix_data = arg->pastix_data;
    SCOTCH_Context     *sctx        = arg->scotch_ctx;
    int                 rank        = ctx->rank;

    if( rank == 0 ) {
        SCOTCH_contextInit( sctx );        /* Initialize context           */
        SCOTCH_contextRandomClone( sctx ); /* Set private random generator */

        /* Enable this define to fix the SCOTCH random generator */
#if defined(PASTIX_ORDERING_FIX_SEED)
        SCOTCH_contextRandomSeed( sctx, (SCOTCH_Num)(pastix_data->id) );
#endif

        /*
         * Initiates the capture, into the given context,
         * of an existing pool of threads.
         */
        SCOTCH_contextThreadImport1( sctx, pastix_data->isched->world_size );
    }

    /* Synchronize all threads */
    isched_barrier_wait( &(ctx->global_ctx->barrier) );

    /* Finalyse the capture */
    SCOTCH_contextThreadImport2( sctx, rank );

    if( rank == 0 ) {
        SCOTCH_Graph *scotchgraph0 = arg->scotch_grf;
        SCOTCH_Graph  scotchgraph1;
        int           ret;

        /* Initialize MT graph, and bind it to the global one in the Scotch context */
        SCOTCH_graphInit( &scotchgraph1 );
        SCOTCH_contextBindGraph( sctx, scotchgraph0, &scotchgraph1 );

        /* Compute the ordering */
        ret = ocs_compute_graph_ordering( pastix_data, &scotchgraph1 );

        /* Destroy the context graph */
        SCOTCH_graphExit( &scotchgraph1 );

        /* Destroy the context and release the slave threads */
        SCOTCH_contextExit( sctx );

        arg->ret = ret;
    }
}
#endif

/**
 *******************************************************************************
 *
 * @ingroup pastix_order
 *
 * @brief Compute the ordering of the graph given as parameter
 * with Scotch library.
 *
 * This routine is affected by the following parameters:
 *   IPARM_VERBOSE, IPARM_ORDERING_DEFAULT, IPARM_SCOTCH_SWITCH_LEVEL,
 *   IPARM_SCOTCH_CMIN, IPARM_SCOTCH_CMAX, IPARM_SCOTCH_FRAT
 * If PASTIX_ORDERING_SCOTCH_MT is enabled, we will compute the ordering
 * in shared memory
 *
 *******************************************************************************
 *
 * @param[inout] pastix_data
 *          The pastix_data structure that describes the solver instance.
 *          On exit, the field ordemesh is initialize with the result of the
 *          ordering realized by Scotch.
 *
 * @param[inout] graph
 *          The graph prepared by graphPrepare function on which we want to
 *          perform the ordering. On exit, the graph might be rebased.
 *
 *******************************************************************************
 *
 * @retval PASTIX_SUCCESS on successful exit,
 * @retval PASTIX_ERR_BADPARAMETER if one parameter is incorrect,
 * @retval PASTIX_ERR_OUTOFMEMORY if one allocation failed,
 * @retval PASTIX_ERR_INTEGER_TYPE if Scotch integer type is not the
 *         same size as PaStiX ones,
 * @retval PASTIX_ERR_INTERNAL if an error occurs internally to Scotch.
 *
 *******************************************************************************/
int
pastixOrderComputeScotch( pastix_data_t  *pastix_data,
                          pastix_graph_t *graph )
{
    SCOTCH_Graph    scotchgraph;
    int             ret      = PASTIX_SUCCESS;
    pastix_order_t *ordemesh = pastix_data->ordemesh;
    pastix_int_t    baseval  = graph->baseval;

    /* Check integer compatibility */
    if (sizeof(pastix_int_t) != sizeof(SCOTCH_Num)) {
        errorPrint("Inconsistent integer type\n");
        return PASTIX_ERR_INTEGER_TYPE;
    }

    /* Enable this define to fix the SCOTCH random generator */
#if defined(PASTIX_ORDERING_FIX_SEED) && defined(PASTIX_ORDERING_SCOTCH_MT)
    SCOTCH_randomSeed( (SCOTCH_Num)(pastix_data->id) );
#endif

    /* Build The Scotch Graph */
    ocs_scotchgraph_init( &scotchgraph, graph );

    /* Allocate the ordering structure */
    pastixOrderAlloc( ordemesh, graph->n, graph->n );

    /*
     * Compute the ordering
     */
#if defined(PASTIX_ORDERING_SCOTCH_MT)
    if ( pastix_data->iparm[IPARM_SCOTCH_MT] )
    {
        SCOTCH_Context sctx;

        struct args_ocs_mt args = {
            .pastix_data = pastix_data,
            .scotch_ctx  = &sctx,
            .scotch_grf  = &scotchgraph,
            .ret         = ret,
        };
        isched_parallel_call( pastix_data->isched, ocs_compute_graph_ordering_mt, &args );
        ret = args.ret;
    }
    else
#endif
    {
        ret = ocs_compute_graph_ordering( pastix_data, &scotchgraph );
    }

    /* Free the Scotch graph structure */
    ocs_scotchgraph_exit( &scotchgraph, baseval );

    /* If something has failed in Scotch */
    if ( ret != 0 ) {
        pastixOrderExit(ordemesh);
        return PASTIX_ERR_INTERNAL;
    }
    ocs_reallocate_ordemesh( ordemesh );

    return ret;
}
