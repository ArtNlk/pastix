/*
 * File: out.h
 *
 * Define output format string for PaStiX.
 *
 * Authors:
 *   Mathieu Faverge - faverge@labri.fr
 *   Xavier  LACOSTE - lacoste@labri.fr
 *   Pierre  RAMET   - ramet@labri.fr
 */
#ifndef _OUT_H_
#define _OUT_H_

#define OUT_HEADER                                              \
    "+-------------------------------------------------+\n"     \
    "+     PaStiX : Parallel Sparse matriX package     +\n"     \
    "+-------------------------------------------------+\n"     \
    "  Version:                                   %d.%d.%d\n"   \
    "  Schedulers:\n"                                           \
    "    sequential:                           %8s\n"           \
    "    thread static:                        %8s\n"           \
    "    thread dynamic:                       %8s\n"           \
    "    parsec:                               %8s\n"           \
    "    starpu:                               %8s\n"           \
    "  Number of MPI processes:                %8d\n"           \
    "  Number of threads per process:          %8d\n"           \
    "  MPI communication support:              %8s\n"

#define OUT_HEADER_LR                                           \
    "+-------------------------------------------------+\n"     \
    "  Low rank parameters:                             \n"     \
    "    Tolerance                             %8.0e    \n"     \
    "    Compress size                         %8ld     \n"     \
    "    Compress width                        %8ld     \n"     \
    "    Strategy                      %16s             \n"     \
    "    Compress method                       %8s      \n"

#define OUT_STEP_ORDER                                          \
    "+-------------------------------------------------+\n"     \
    "  Ordering step :\n"
#define OUT_SUBSTEP_GRAPH                       \
    "    Prepare graph structure:\n"
#define OUT_ORDER_SYMGRAPH                      \
    "      Symmetrizing graph\n"
#define OUT_ORDER_NODIAG                        \
    "      Removing diagonal elements\n"
#define OUT_ORDER_SORT                          \
    "      Sort row indices in each column\n"
#define OUT_ORDER_INIT                          \
    "    Compute ordering\n"
#define OUT_ORDER_METHOD                        \
    "    Ordering method is: %s\n"
#define OUT_ORDER_TIME                                  \
    "    Time to compute ordering              %e s\n"

#define OUT_STEP_FAX                                            \
    "+-------------------------------------------------+\n"     \
    "  Symbolic Factorization :\n"
#define OUT_FAX_METHOD                          \
    "    Symbol factorization using: %s\n"


#define OUT_STEP_REORDER                                        \
    "+-------------------------------------------------+\n"     \
    "  Reordering step:\n"                                      \
    "    Split level                           %8ld\n"          \
    "    Stoping criteria                      %8ld\n"
#define OUT_REORDERING_TIME                             \
    "    Time for reordering                   %e s\n"

#define OUT_STEP_BLEND                                          \
    "+-------------------------------------------------+\n"     \
    "  Analyse step:\n"
#define OUT_BLEND_CONF                                  \
    "    Number of cluster                     %8ld\n"  \
    "    Number of processor per cluster       %8ld\n"  \
    "    Number of thread per MPI process      %8ld\n"

#define OUT_BLEND_CHKSMBMTX                     \
    "    Check the symbol matrix\n"
#define OUT_BLEND_ELIMTREE                      \
    "    Building elimination tree\n"
#define OUT_BLEND_ELIMTREE_TIME                         \
    "    Elimination tree built in             %e s\n"
#define OUT_BLEND_COSTMATRIX                    \
    "    Building cost matrix\n"
#define OUT_BLEND_COSTMATRIX_TIME                       \
    "    Cost matrix built in                  %e s\n"
#define OUT_BLEND_ELIMTREE_TOTAL_COST                   \
    "    Total estimated cost of the etree     %e s\n"
#define OUT_BLEND_PROPMAP                       \
    "    Perform proportional mapping\n"
#define OUT_BLEND_PROPMAP_TIME                          \
    "    Proportional mapping done in          %e s\n"
#define OUT_BLEND_SPLITSYMB                     \
    "    Split large symbolic blocks\n"
#define OUT_BLEND_SPLITSYMB_TIME                        \
    "    Symbol split done in                  %e s\n"
#define OUT_BLEND_BUILDSIMU                     \
    "    Build simulation structures\n"
#define OUT_BLEND_BUILDSIMU_TIME                        \
    "    Simulation structures built in        %e s\n"  \
    "    Number of tasks found                 %8ld\n"
#define OUT_BLEND_SIMU                                  \
    "    Start simulation (Data distribution)\n"
#define OUT_BLEND_SIMU_TIME                             \
    "    Simulation done in                    %e s\n"
#define OUT_BLEND_ELIMGRAPH                     \
    "    Building elimination graph\n"
#define OUT_BLEND_ELIMGRAPH_TIME                        \
    "    Elimination graph built in            %e s\n"
#define OUT_BLEND_SOLVER                        \
    "    Building solver structure\n"
#define OUT_BLEND_SOLVER_TIME                           \
    "    Solver built in                       %e s\n"
#define OUT_BLEND_TIME                                  \
    "    Time for analyze                      %e s\n"


#define OUT_STARPU_TP         " StarPU : Thread policy : %s\n"
#define OUT_STARPU_STP        " StarPU : No thread policy, setting thread policy to : %s\n"
#define OUT_MATRIX_SIZE       "  Matrix size                                   %ld x %ld\n"
#define OUT_NNZ               "  Number of nonzeros in A                       %ld\n"

#define OUT_GLOBAL_NNZL       "   Number of nonzeroes in L structure      %ld\n"
#define OUT_GLOBAL_FILLIN     "   Fill-in                                 %lf\n"
#define OUT_GLOBAL_THFLOPCNT  "   Number of theoretical flop            %.5g %cflops\n"
#define OUT_GLOBAL_RLFLOPCNT  "   Number of performed flop              %.5g %cflops\n"

#define OUT_STEP_NUMFACT_LU   " Numerical Factorization (LU) :\n"
#ifdef TYPE_COMPLEX
#  define OUT_STEP_NUMFACT_LLT  " Numerical Factorization (LLh) :\n"
#else
#  define OUT_STEP_NUMFACT_LLT  " Numerical Factorization (LLt) :\n"
#endif
#define OUT_STEP_NUMFACT_LDLT " Numerical Factorization (LDLt) :\n"
#define OUT_STEP_NUMFACT_LDLH " Numerical Factorization (LDLh) :\n"
#define OUT_STEP_SOLVE        " Solve :\n"
#define OUT_STEP_REFF         " Reffinement :\n"
#define TIME_TO_ANALYSE       "   Time to analyze                              %.3g s\n"
#define NNZERO_WITH_FILLIN_TH "   Number of nonzeros in factorized matrix      %ld\n"
#define NNZERO_WITH_FILLIN    "%d : Number of nonzeros (local block structure) %ld\n"
#define SOLVMTX_WITHOUT_CO    "%d : SolverMatrix size (without coefficients)   %.3g %s\n"
#define OUT_FILLIN_TH         "   Fill-in                                      %lg\n"
#define NUMBER_OP_LU          "   Number of operations (LU)                    %g\n"
#define NUMBER_OP_LLT         "   Number of operations (LLt)                   %g\n"
#define TIME_FACT_PRED        "   Prediction Time to factorize (%s) %.3g s\n"
#define OUT_COEFSIZE          "   Maximum coeftab size (cefficients)           %.3g %s\n"
#define OUT_REDIS_CSC         "   Redistributing user CSC into PaStiX distribution\n"
#define OUT_REDIS_RHS         "   Redistributing user RHS into PaStiX distribution\n"
#define OUT_REDIS_SOL         "   Redistributing solution into Users' distribution\n"
#define OUT2_SOP_BINITG       "   --- Sopalin : Allocation de la structure globale ---\n"
#define OUT2_SOP_EINITG       "   --- Fin Sopalin Init                             ---\n"
#define OUT2_SOP_TABG         "   --- Initialisation des tableaux globaux          ---\n"
#define OUT2_SOP_BINITL       "   --- Sopalin : Local structure allocation         ---\n"
#define OUT2_SOP_NOTBIND      "   --- Sopalin : Threads are NOT binded             ---\n"
#define OUT2_SOP_BIND         "   --- Sopalin : Threads are binded                 ---\n"
#define OUT2_FUN_STATS        "     - %3ld : Envois %5ld - Receptions %5ld          -\n"
#define OUT2_SOP_BSOP         "   --- Sopalin Begin                                ---\n"
#define OUT2_SOP_ESOP         "   --- Sopalin End                                  ---\n"
#define OUT4_UPDO_TIME_INIT   " [%d][%d] Solve initialization time : %lg s\n"
#define OUT4_UPDO_COMM_TIME   " [%d][%d] Solve communication time : %lg s\n"
#define OUT4_FACT_COMM_TIME   " [%d][%d] Factorization communication time : %lg s\n"
#define OUT2_SOP_DOWN         "   --- Down Step                                    ---\n"
#define OUT2_SOP_DIAG         "   --- Diag Step                                    ---\n"
#define OUT2_SOP_UP           "   --- Up Step                                      ---\n"
#define GEN_RHS_1             "   Generate RHS for X=1\n"
#define GEN_RHS_I             "   Generate RHS for X=i\n"
#define GEN_SOL_0             "   Generate X0=0\n"
#define OOC_MEM_LIM_PERCENT   "   OOC memory limit                             %d%% of needed (%.3g %s)\n"
#define OOC_MEM_LIM           "   OOC memory limit                             %.3g %s\n"
#define OOC_IN_STEP           "   [%2d] IN %s :\n"
#define OOC_WRITTEN           "   [%2d]   written                               %.3g %s, allocated : %.3g %s\n"
#define OOC_READ              "   [%2d]   read                                  %.3g %s\n"
#define OOC_ALLOCATED         "   [%2d]   Allocated                             %.3g %s\n"
#define OOC_MAX_ALLOCATED     "   [%2d]   Maximum allocated                     %.3g %s\n"
#define OUT_ITERRAFF_GMRES    "   GMRES :\n"
#define OUT_ITERRAFF_PIVOT    "   Simple refinement :\n"
#define OUT_ITERRAFF_BICGSTAB  "   BICGSTAB :\n"
#define OUT_ITERRAFF_GRAD     "   Conjuguate gradient :\n"
#define OUT_ITERRAFF_ITER     "    - iteration %d :\n"
#define OUT_ITERRAFF_TTS      "         time to solve                          %.3g s\n"
#define OUT_ITERRAFF_TTT      "         total iteration time                   %.3g s\n"
#define OUT_ITERRAFF_ERR      "         error                                  %.5g\n"
#define OUT_ITERRAFF_NORMA    "         ||A||                                  %.5g\n"
#define OUT_ITERRAFF_NORMR    "         ||r||                                  %.5g\n"
#define OUT_ITERRAFF_NORMB    "         ||b||                                  %.5g\n"
#define OUT_ITERRAFF_BDIVR    "         ||r||/||b||                            %.5g\n"
#define OUT_REDISCSCDTIME     "   Time to redistribute cscd                    %.3g s\n"
#define OUT_FILLCSCTIME       "   Time to fill internal csc                    %.3g s\n"
#define OUT_MAX_MEM_AF_SOP    "   Max memory used after factorization          %.3g %s\n"
#define OUT_MEM_USED_AF_SOP   "   Memory used after factorization              %.3g %s\n"
#define MAX_MEM_AF_CL         "   Max memory used after clean                  %.3g %s\n"
#define MEM_USED_AF_CL        "   Memory used after clean                      %.3g %s\n"
#define OUT_STATIC_PIVOTING   "   Static pivoting                              %ld\n"
#define OUT_INERTIA           "   Inertia                                      %ld\n"
#define OUT_INERTIA_PIVOT     "   Inertia (NB: with pivoting)                  %ld\n"
#define OUT_ESP_NBTASKS       "   Number of tasks added by esp                 %ld\n"
#define OUT_TIME_FACT         "   Time to factorize                            %.3g s  (%.3g %s)\n"
#define OUT_FLOPS_FACT        "   FLOPS during factorization                   %.5g %s\n"
#define OUT_TIME_SOLV         "   Time to solve                                %.3g s\n"
#define OUT_RAFF_ITER_NORM    "   Refinement                                   %ld iterations, norm=%.3g\n"
#define OUT_PREC1             "   ||b-Ax||/||b||                               %.3g\n"
#define OUT_PREC2             "   max_i(|b-Ax|_i/(|b| + |A||x|)_i              %.3g\n"
#define OUT_TIME_RAFF         "   Time for refinement                          %.3g s\n"
#define OUT_END               " +--------------------------------------------------------------------+\n"

/*
 * Printing function to redirect to the correct file
 */
#if defined(__GNUC__)
static inline void pastix_print( int mpirank, int thrdrank, const char *fmt, ...) __attribute__((format(printf,3,4)));
#endif

static inline void
pastix_print( int mpirank, int thrdrank, const char *fmt, ...)
{
    va_list ap;

    if( (mpirank == 0) && (thrdrank == 0) )
    {
        va_start(ap, fmt);
        vfprintf(stdout, fmt, ap );
        va_end(ap);
    }
}

#define MEMORY_WRITE(mem) ( ((mem) < 1<<10) ?                           \
                            ( (double)(mem) ) :                         \
                            ( ( (mem) < 1<<20 ) ?                       \
                              ( (double)(mem)/(double)(1<<10) ) :       \
                              ( ((mem) < 1<<30 ) ?                      \
                                ( (double)(mem)/(double)(1<<20) ) :     \
                                ( (double)(mem)/(double)(1<<30) ))))
#define MEMORY_UNIT_WRITE(mem) (((mem) < 1<<10) ?                       \
                                "o" :                                   \
                                ( ( (mem) < 1<<20 ) ?                   \
                                  "Ko" :                                \
                                  ( ( (mem) < 1<<30 ) ?                 \
                                    "Mo" :                              \
                                    "Go" )))

#define PRINT_FLOPS(flops) ( ((flops) < 1<<10) ?                        \
                             ( (double)(flops) ) :                      \
                             ( ( (flops) < 1<<20 ) ?                    \
                               ( (double)(flops)/(double)(1<<10) ) :    \
                               ( ((flops) < 1<<30 ) ?                   \
                                 ( (double)(flops)/(double)(1<<20) ) :  \
                                 ( (double)(flops)/(double)(1<<30) ))))
#define PRINT_FLOPS_UNIT(flops) ( ((flops) < 1<<10) ?                   \
                                  ( "Flop/s" ) :                        \
                                  ( ( (flops) < 1<<20 ) ?               \
                                    ( "KFlop/s" ) :                     \
                                    ( ((flops) < 1<<30 ) ?              \
                                      ( "MFlop/s" ) :                   \
                                      ( "GFlop/s" ))))

static inline double
printflopsv( double flops )
{
    static double ratio = (double)(1<<10);
    int unit = 0;

    while ( (flops > ratio) && (unit < 9) ) {
        flops /= ratio;
        unit++;
    }
    return flops;
}

static inline char
printflopsu( double flops )
{
    static char units[9] = { ' ', 'k', 'm', 'g', 't', 'p', 'e', 'z', 'y' };
    static double ratio = (double)(1<<10);
    int unit = 0;

    while ( (flops > ratio) && (unit < 9) ) {
        flops /= ratio;
        unit++;
    }
    return units[unit];
}

#endif /* _OUT_H_ */
