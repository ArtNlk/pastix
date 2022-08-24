/**
 *
 * @file parse_options.c
 *
 * This file is generated automatically. If you want to modify it, modify
 * ${PASTIX_HOME}/tools/gen_param/pastix_[iparm/dparm/enums].py and run
 * ${PASTIX_HOME}/tools/gen_param/gen_parm_files.py ${PASTIX_HOME}.
 *
 * @copyright 2004-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
 *                      Univ. Bordeaux. All rights reserved.
 *
 * @version 6.2.1
 * @author Mathieu Faverge
 * @author Pierre Ramet
 * @author Xavier Lacoste
 * @author Esragul Korkmaz
 * @author Gregoire Pichon
 * @author Tony Delarue
 * @date 2021-07-02
 *
 */
#include "common.h"
#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif  /* defined(HAVE_GETOPT_H) */
#include <string.h>

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 *
 * @brief Parse iparm keywords to return its associated index in the iparm array
 *
 * This function converts the string only for input parameters, output
 * parameters are not handled.
 *
 *******************************************************************************
 *
 * @param[in] iparm
 *          The iparm string to convert to enum.
 *
 *******************************************************************************
 *
 * @retval The index of the iparm in the array.
 * @retval -1 if the string is not an iparm parameter.
 *
 *******************************************************************************/
pastix_iparm_t
parse_iparm( const char *iparm )
{
    if(0 == strcasecmp("iparm_verbose",                        iparm)) { return IPARM_VERBOSE; }
    if(0 == strcasecmp("iparm_io_strategy",                    iparm)) { return IPARM_IO_STRATEGY; }

    if(0 == strcasecmp("iparm_produce_stats",                  iparm)) { return IPARM_PRODUCE_STATS; }

    if(0 == strcasecmp("iparm_mc64",                           iparm)) { return IPARM_MC64; }

    if(0 == strcasecmp("iparm_ordering",                       iparm)) { return IPARM_ORDERING; }
    if(0 == strcasecmp("iparm_ordering_default",               iparm)) { return IPARM_ORDERING_DEFAULT; }

    if(0 == strcasecmp("iparm_scotch_mt",                      iparm)) { return IPARM_SCOTCH_MT; }
    if(0 == strcasecmp("iparm_scotch_switch_level",            iparm)) { return IPARM_SCOTCH_SWITCH_LEVEL; }
    if(0 == strcasecmp("iparm_scotch_cmin",                    iparm)) { return IPARM_SCOTCH_CMIN; }
    if(0 == strcasecmp("iparm_scotch_cmax",                    iparm)) { return IPARM_SCOTCH_CMAX; }
    if(0 == strcasecmp("iparm_scotch_frat",                    iparm)) { return IPARM_SCOTCH_FRAT; }

    if(0 == strcasecmp("iparm_metis_ctype",                    iparm)) { return IPARM_METIS_CTYPE; }
    if(0 == strcasecmp("iparm_metis_rtype",                    iparm)) { return IPARM_METIS_RTYPE; }
    if(0 == strcasecmp("iparm_metis_no2hop",                   iparm)) { return IPARM_METIS_NO2HOP; }
    if(0 == strcasecmp("iparm_metis_nseps",                    iparm)) { return IPARM_METIS_NSEPS; }
    if(0 == strcasecmp("iparm_metis_niter",                    iparm)) { return IPARM_METIS_NITER; }
    if(0 == strcasecmp("iparm_metis_ufactor",                  iparm)) { return IPARM_METIS_UFACTOR; }
    if(0 == strcasecmp("iparm_metis_compress",                 iparm)) { return IPARM_METIS_COMPRESS; }
    if(0 == strcasecmp("iparm_metis_ccorder",                  iparm)) { return IPARM_METIS_CCORDER; }
    if(0 == strcasecmp("iparm_metis_pfactor",                  iparm)) { return IPARM_METIS_PFACTOR; }
    if(0 == strcasecmp("iparm_metis_seed",                     iparm)) { return IPARM_METIS_SEED; }
    if(0 == strcasecmp("iparm_metis_dbglvl",                   iparm)) { return IPARM_METIS_DBGLVL; }

    if(0 == strcasecmp("iparm_amalgamation_lvlblas",           iparm)) { return IPARM_AMALGAMATION_LVLBLAS; }
    if(0 == strcasecmp("iparm_amalgamation_lvlcblk",           iparm)) { return IPARM_AMALGAMATION_LVLCBLK; }

    if(0 == strcasecmp("iparm_reordering_split",               iparm)) { return IPARM_REORDERING_SPLIT; }
    if(0 == strcasecmp("iparm_reordering_stop",                iparm)) { return IPARM_REORDERING_STOP; }
    if(0 == strcasecmp("iparm_splitting_strategy",             iparm)) { return IPARM_SPLITTING_STRATEGY; }
    if(0 == strcasecmp("iparm_splitting_levels_projections",   iparm)) { return IPARM_SPLITTING_LEVELS_PROJECTIONS; }
    if(0 == strcasecmp("iparm_splitting_levels_kway",          iparm)) { return IPARM_SPLITTING_LEVELS_KWAY; }
    if(0 == strcasecmp("iparm_splitting_projections_depth",    iparm)) { return IPARM_SPLITTING_PROJECTIONS_DEPTH; }
    if(0 == strcasecmp("iparm_splitting_projections_distance", iparm)) { return IPARM_SPLITTING_PROJECTIONS_DISTANCE; }
    if(0 == strcasecmp("iparm_splitting_projections_width",    iparm)) { return IPARM_SPLITTING_PROJECTIONS_WIDTH; }

    if(0 == strcasecmp("iparm_min_blocksize",                  iparm)) { return IPARM_MIN_BLOCKSIZE; }
    if(0 == strcasecmp("iparm_max_blocksize",                  iparm)) { return IPARM_MAX_BLOCKSIZE; }
    if(0 == strcasecmp("iparm_tasks2d_level",                  iparm)) { return IPARM_TASKS2D_LEVEL; }
    if(0 == strcasecmp("iparm_tasks2d_width",                  iparm)) { return IPARM_TASKS2D_WIDTH; }
    if(0 == strcasecmp("iparm_allcand",                        iparm)) { return IPARM_ALLCAND; }

    if(0 == strcasecmp("iparm_incomplete",                     iparm)) { return IPARM_INCOMPLETE; }
    if(0 == strcasecmp("iparm_level_of_fill",                  iparm)) { return IPARM_LEVEL_OF_FILL; }

    if(0 == strcasecmp("iparm_factorization",                  iparm)) { return IPARM_FACTORIZATION; }
    if(0 == strcasecmp("iparm_free_cscuser",                   iparm)) { return IPARM_FREE_CSCUSER; }
    if(0 == strcasecmp("iparm_schur_fact_mode",                iparm)) { return IPARM_SCHUR_FACT_MODE; }

    if(0 == strcasecmp("iparm_transpose_solve",                iparm)) { return IPARM_TRANSPOSE_SOLVE; }
    if(0 == strcasecmp("iparm_schur_solv_mode",                iparm)) { return IPARM_SCHUR_SOLV_MODE; }
    if(0 == strcasecmp("iparm_applyperm_ws",                   iparm)) { return IPARM_APPLYPERM_WS; }

    if(0 == strcasecmp("iparm_refinement",                     iparm)) { return IPARM_REFINEMENT; }
    if(0 == strcasecmp("iparm_itermax",                        iparm)) { return IPARM_ITERMAX; }
    if(0 == strcasecmp("iparm_gmres_im",                       iparm)) { return IPARM_GMRES_IM; }

    if(0 == strcasecmp("iparm_scheduler",                      iparm)) { return IPARM_SCHEDULER; }
    if(0 == strcasecmp("iparm_thread_nbr",                     iparm)) { return IPARM_THREAD_NBR; }
    if(0 == strcasecmp("iparm_autosplit_comm",                 iparm)) { return IPARM_AUTOSPLIT_COMM; }

    if(0 == strcasecmp("iparm_gpu_nbr",                        iparm)) { return IPARM_GPU_NBR; }
    if(0 == strcasecmp("iparm_gpu_memory_percentage",          iparm)) { return IPARM_GPU_MEMORY_PERCENTAGE; }
    if(0 == strcasecmp("iparm_gpu_memory_block_size",          iparm)) { return IPARM_GPU_MEMORY_BLOCK_SIZE; }

    if(0 == strcasecmp("iparm_compress_min_width",             iparm)) { return IPARM_COMPRESS_MIN_WIDTH; }
    if(0 == strcasecmp("iparm_compress_min_height",            iparm)) { return IPARM_COMPRESS_MIN_HEIGHT; }
    if(0 == strcasecmp("iparm_compress_when",                  iparm)) { return IPARM_COMPRESS_WHEN; }
    if(0 == strcasecmp("iparm_compress_method",                iparm)) { return IPARM_COMPRESS_METHOD; }
    if(0 == strcasecmp("iparm_compress_ortho",                 iparm)) { return IPARM_COMPRESS_ORTHO; }
    if(0 == strcasecmp("iparm_compress_reltol",                iparm)) { return IPARM_COMPRESS_RELTOL; }
    if(0 == strcasecmp("iparm_compress_preselect",             iparm)) { return IPARM_COMPRESS_PRESELECT; }
    if(0 == strcasecmp("iparm_compress_iluk",                  iparm)) { return IPARM_COMPRESS_ILUK; }

    if(0 == strcasecmp("iparm_modify_parameter",               iparm)) { return IPARM_MODIFY_PARAMETER; }
    if(0 == strcasecmp("iparm_start_task",                     iparm)) { return IPARM_START_TASK; }
    if(0 == strcasecmp("iparm_end_task",                       iparm)) { return IPARM_END_TASK; }
    if(0 == strcasecmp("iparm_float",                          iparm)) { return IPARM_FLOAT; }
    if(0 == strcasecmp("iparm_mtx_type",                       iparm)) { return IPARM_MTX_TYPE; }
    if(0 == strcasecmp("iparm_dof_nbr",                        iparm)) { return IPARM_DOF_NBR; }

    return -1;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 *
 * @brief Parse dparm keywords to return its associated index in the dparm array
 *
 * This function converts the string only for input parameters, output
 * parameters are not handled.
 *
 *******************************************************************************
 *
 * @param[in] dparm
 *          The dparm string to convert to enum.
 *
 *******************************************************************************
 *
 * @retval The index of the dparm in the array.
 * @retval -1 if the string is not a dparm parameter.
 *
 *******************************************************************************/
pastix_dparm_t
parse_dparm( const char *dparm )
{
    if(0 == strcasecmp("dparm_epsilon_refinement", dparm)) { return DPARM_EPSILON_REFINEMENT; }
    if(0 == strcasecmp("dparm_epsilon_magn_ctrl",  dparm)) { return DPARM_EPSILON_MAGN_CTRL; }
    if(0 == strcasecmp("dparm_compress_tolerance", dparm)) { return DPARM_COMPRESS_TOLERANCE; }
    if(0 == strcasecmp("dparm_compress_min_ratio", dparm)) { return DPARM_COMPRESS_MIN_RATIO; }

    return -1;
}

/**
 *******************************************************************************
 *
 * @ingroup pastix_common
 *
 * @brief Parse enum values for iparm array, and return the enum value
 * associated to it.
 *
 *******************************************************************************
 *
 * @param[in] string
 *          The enum name to convert to its value
 *
 *******************************************************************************
 *
 * @retval The value if the enum associated to the string
 * @retval -1 if the string is not an enum in the pastix API.
 *
 *******************************************************************************/
int
parse_enums( const char *string )
{
    if(0 == strcasecmp("pastixverbosenot", string)) { return PastixVerboseNot; }
    if(0 == strcasecmp("pastixverboseno",  string)) { return PastixVerboseNo; }
    if(0 == strcasecmp("pastixverboseyes", string)) { return PastixVerboseYes; }

    if(0 == strcasecmp("pastixiono",        string)) { return PastixIONo; }
    if(0 == strcasecmp("pastixioload",      string)) { return PastixIOLoad; }
    if(0 == strcasecmp("pastixiosave",      string)) { return PastixIOSave; }
    if(0 == strcasecmp("pastixioloadgraph", string)) { return PastixIOLoadGraph; }
    if(0 == strcasecmp("pastixiosavegraph", string)) { return PastixIOSaveGraph; }
    if(0 == strcasecmp("pastixioloadcsc",   string)) { return PastixIOLoadCSC; }
    if(0 == strcasecmp("pastixiosavecsc",   string)) { return PastixIOSaveCSC; }

    if(0 == strcasecmp("pastixorderscotch",   string)) { return PastixOrderScotch; }
    if(0 == strcasecmp("pastixordermetis",    string)) { return PastixOrderMetis; }
    if(0 == strcasecmp("pastixorderpersonal", string)) { return PastixOrderPersonal; }
    if(0 == strcasecmp("pastixorderptscotch", string)) { return PastixOrderPtScotch; }
    if(0 == strcasecmp("pastixorderparmetis", string)) { return PastixOrderParMetis; }

    if(0 == strcasecmp("pastixsplitnot",             string)) { return PastixSplitNot; }
    if(0 == strcasecmp("pastixsplitkway",            string)) { return PastixSplitKway; }
    if(0 == strcasecmp("pastixsplitkwayprojections", string)) { return PastixSplitKwayProjections; }

    if(0 == strcasecmp("pastixfactpotrf", string)) { return PastixFactPOTRF; }
    if(0 == strcasecmp("pastixfactsytrf", string)) { return PastixFactSYTRF; }
    if(0 == strcasecmp("pastixfactgetrf", string)) { return PastixFactGETRF; }
    if(0 == strcasecmp("pastixfactpxtrf", string)) { return PastixFactPXTRF; }
    if(0 == strcasecmp("pastixfacthetrf", string)) { return PastixFactHETRF; }
    if(0 == strcasecmp("pastixfactllh",   string)) { return PastixFactLLH; }
    if(0 == strcasecmp("pastixfactldlt",  string)) { return PastixFactLDLT; }
    if(0 == strcasecmp("pastixfactlu",    string)) { return PastixFactLU; }
    if(0 == strcasecmp("pastixfactllt",   string)) { return PastixFactLLT; }
    if(0 == strcasecmp("pastixfactldlh",  string)) { return PastixFactLDLH; }

    if(0 == strcasecmp("pastixfactmodelocal", string)) { return PastixFactModeLocal; }
    if(0 == strcasecmp("pastixfactmodeschur", string)) { return PastixFactModeSchur; }
    if(0 == strcasecmp("pastixfactmodeboth",  string)) { return PastixFactModeBoth; }

    if(0 == strcasecmp("pastixnotrans",   string)) { return PastixNoTrans; }
    if(0 == strcasecmp("pastixtrans",     string)) { return PastixTrans; }
    if(0 == strcasecmp("pastixconjtrans", string)) { return PastixConjTrans; }

    if(0 == strcasecmp("pastixsolvmodelocal",     string)) { return PastixSolvModeLocal; }
    if(0 == strcasecmp("pastixsolvmodeinterface", string)) { return PastixSolvModeInterface; }
    if(0 == strcasecmp("pastixsolvmodeschur",     string)) { return PastixSolvModeSchur; }

    if(0 == strcasecmp("pastixrefinegmres",    string)) { return PastixRefineGMRES; }
    if(0 == strcasecmp("pastixrefinecg",       string)) { return PastixRefineCG; }
    if(0 == strcasecmp("pastixrefinesr",       string)) { return PastixRefineSR; }
    if(0 == strcasecmp("pastixrefinebicgstab", string)) { return PastixRefineBiCGSTAB; }

    if(0 == strcasecmp("pastixschedsequential", string)) { return PastixSchedSequential; }
    if(0 == strcasecmp("pastixschedstatic",     string)) { return PastixSchedStatic; }
    if(0 == strcasecmp("pastixschedparsec",     string)) { return PastixSchedParsec; }
    if(0 == strcasecmp("pastixschedstarpu",     string)) { return PastixSchedStarPU; }
    if(0 == strcasecmp("pastixscheddynamic",    string)) { return PastixSchedDynamic; }

    if(0 == strcasecmp("pastixcompressnever",      string)) { return PastixCompressNever; }
    if(0 == strcasecmp("pastixcompresswhenbegin",  string)) { return PastixCompressWhenBegin; }
    if(0 == strcasecmp("pastixcompresswhenend",    string)) { return PastixCompressWhenEnd; }
    if(0 == strcasecmp("pastixcompresswhenduring", string)) { return PastixCompressWhenDuring; }

    if(0 == strcasecmp("pastixcompressmethodsvd",   string)) { return PastixCompressMethodSVD; }
    if(0 == strcasecmp("pastixcompressmethodpqrcp", string)) { return PastixCompressMethodPQRCP; }
    if(0 == strcasecmp("pastixcompressmethodrqrcp", string)) { return PastixCompressMethodRQRCP; }
    if(0 == strcasecmp("pastixcompressmethodtqrcp", string)) { return PastixCompressMethodTQRCP; }
    if(0 == strcasecmp("pastixcompressmethodrqrrt", string)) { return PastixCompressMethodRQRRT; }
    if(0 == strcasecmp("pastixcompressmethodnbr",   string)) { return PastixCompressMethodNbr; }

    if(0 == strcasecmp("pastixcompressorthocgs",       string)) { return PastixCompressOrthoCGS; }
    if(0 == strcasecmp("pastixcompressorthoqr",        string)) { return PastixCompressOrthoQR; }
    if(0 == strcasecmp("pastixcompressorthopartialqr", string)) { return PastixCompressOrthoPartialQR; }

    if(0 == strcasecmp("pastixtaskinit",     string)) { return PastixTaskInit; }
    if(0 == strcasecmp("pastixtaskordering", string)) { return PastixTaskOrdering; }
    if(0 == strcasecmp("pastixtasksymbfact", string)) { return PastixTaskSymbfact; }
    if(0 == strcasecmp("pastixtaskanalyze",  string)) { return PastixTaskAnalyze; }
    if(0 == strcasecmp("pastixtasknumfact",  string)) { return PastixTaskNumfact; }
    if(0 == strcasecmp("pastixtasksolve",    string)) { return PastixTaskSolve; }
    if(0 == strcasecmp("pastixtaskrefine",   string)) { return PastixTaskRefine; }
    if(0 == strcasecmp("pastixtaskclean",    string)) { return PastixTaskClean; }

    if(0 == strcasecmp("pastixpattern",   string)) { return PastixPattern; }
    if(0 == strcasecmp("pastixfloat",     string)) { return PastixFloat; }
    if(0 == strcasecmp("pastixdouble",    string)) { return PastixDouble; }
    if(0 == strcasecmp("pastixcomplex32", string)) { return PastixComplex32; }
    if(0 == strcasecmp("pastixcomplex64", string)) { return PastixComplex64; }

    /* If the value is directly given without string */
    {
        int value;
        if ( sscanf( string, "%d", &value ) != 1 ) {
            return -1;
        }
        else {
            return value;
        }
    }
}

const char*
pastix_task_getstr( pastix_task_t value )
{
    switch( value ) {
    case PastixTaskInit:
        return "PastixTaskInit";
    case PastixTaskOrdering:
        return "PastixTaskOrdering";
    case PastixTaskSymbfact:
        return "PastixTaskSymbfact";
    case PastixTaskAnalyze:
        return "PastixTaskAnalyze";
    case PastixTaskNumfact:
        return "PastixTaskNumfact";
    case PastixTaskSolve:
        return "PastixTaskSolve";
    case PastixTaskRefine:
        return "PastixTaskRefine";
    case PastixTaskClean:
        return "PastixTaskClean";
    default :
        return "Bad task given";
    }
}

const char*
pastix_verbose_getstr( pastix_verbose_t value )
{
    switch( value ) {
    case PastixVerboseNot:
        return "PastixVerboseNot";
    case PastixVerboseNo:
        return "PastixVerboseNo";
    case PastixVerboseYes:
        return "PastixVerboseYes";
    default :
        return "Bad verbose given";
    }
}

const char*
pastix_io_getstr( pastix_io_t value )
{
    switch( value ) {
    case PastixIONo:
        return "PastixIONo";
    case PastixIOLoad:
        return "PastixIOLoad";
    case PastixIOSave:
        return "PastixIOSave";
    case PastixIOLoadGraph:
        return "PastixIOLoadGraph";
    case PastixIOSaveGraph:
        return "PastixIOSaveGraph";
    case PastixIOLoadCSC:
        return "PastixIOLoadCSC";
    case PastixIOSaveCSC:
        return "PastixIOSaveCSC";
    default :
        return "Bad io given";
    }
}

const char*
pastix_fact_mode_getstr( pastix_fact_mode_t value )
{
    switch( value ) {
    case PastixFactModeLocal:
        return "PastixFactModeLocal";
    case PastixFactModeSchur:
        return "PastixFactModeSchur";
    case PastixFactModeBoth:
        return "PastixFactModeBoth";
    default :
        return "Bad fact_mode given";
    }
}

const char*
pastix_solv_mode_getstr( pastix_solv_mode_t value )
{
    switch( value ) {
    case PastixSolvModeLocal:
        return "PastixSolvModeLocal";
    case PastixSolvModeInterface:
        return "PastixSolvModeInterface";
    case PastixSolvModeSchur:
        return "PastixSolvModeSchur";
    default :
        return "Bad solv_mode given";
    }
}

const char*
pastix_refine_getstr( pastix_refine_t value )
{
    switch( value ) {
    case PastixRefineGMRES:
        return "PastixRefineGMRES";
    case PastixRefineCG:
        return "PastixRefineCG";
    case PastixRefineSR:
        return "PastixRefineSR";
    case PastixRefineBiCGSTAB:
        return "PastixRefineBiCGSTAB";
    default :
        return "Bad refine given";
    }
}

const char*
pastix_coeftype_getstr( pastix_coeftype_t value )
{
    switch( value ) {
    case PastixPattern:
        return "PastixPattern";
    case PastixFloat:
        return "PastixFloat";
    case PastixDouble:
        return "PastixDouble";
    case PastixComplex32:
        return "PastixComplex32";
    case PastixComplex64:
        return "PastixComplex64";
    default :
        return "Bad coeftype given";
    }
}

const char*
pastix_factotype_getstr( pastix_factotype_t value )
{
    switch( value ) {
    case PastixFactLLH:
        return "PastixFactLLH";
    case PastixFactLDLT:
        return "PastixFactLDLT";
    case PastixFactLU:
        return "PastixFactLU";
    case PastixFactLLT:
        return "PastixFactLLT";
    case PastixFactLDLH:
        return "PastixFactLDLH";
    default :
        return "Bad factotype given";
    }
}

const char*
pastix_scheduler_getstr( pastix_scheduler_t value )
{
    switch( value ) {
    case PastixSchedSequential:
        return "PastixSchedSequential";
    case PastixSchedStatic:
        return "PastixSchedStatic";
    case PastixSchedParsec:
        return "PastixSchedParsec";
    case PastixSchedStarPU:
        return "PastixSchedStarPU";
    case PastixSchedDynamic:
        return "PastixSchedDynamic";
    default :
        return "Bad scheduler given";
    }
}

const char*
pastix_ordering_getstr( pastix_ordering_t value )
{
    switch( value ) {
    case PastixOrderScotch:
        return "PastixOrderScotch";
    case PastixOrderMetis:
        return "PastixOrderMetis";
    case PastixOrderPersonal:
        return "PastixOrderPersonal";
    case PastixOrderPtScotch:
        return "PastixOrderPtScotch";
    case PastixOrderParMetis:
        return "PastixOrderParMetis";
    default :
        return "Bad ordering given";
    }
}

const char*
pastix_mpithreadmode_getstr( pastix_mpithreadmode_t value )
{
    switch( value ) {
    case PastixMpiNone:
        return "PastixMpiNone";
    case PastixMpiThreadSingle:
        return "PastixMpiThreadSingle";
    case PastixMpiThreadFunneled:
        return "PastixMpiThreadFunneled";
    case PastixMpiThreadSerialized:
        return "PastixMpiThreadSerialized";
    case PastixMpiThreadMultiple:
        return "PastixMpiThreadMultiple";
    default :
        return "Bad mpithreadmode given";
    }
}

const char*
pastix_error_getstr( pastix_error_t value )
{
    switch( value ) {
    case PASTIX_SUCCESS:
        return "PASTIX_SUCCESS";
    case PASTIX_ERR_UNKNOWN:
        return "PASTIX_ERR_UNKNOWN";
    case PASTIX_ERR_ALLOC:
        return "PASTIX_ERR_ALLOC";
    case PASTIX_ERR_NOTIMPLEMENTED:
        return "PASTIX_ERR_NOTIMPLEMENTED";
    case PASTIX_ERR_OUTOFMEMORY:
        return "PASTIX_ERR_OUTOFMEMORY";
    case PASTIX_ERR_THREAD:
        return "PASTIX_ERR_THREAD";
    case PASTIX_ERR_INTERNAL:
        return "PASTIX_ERR_INTERNAL";
    case PASTIX_ERR_BADPARAMETER:
        return "PASTIX_ERR_BADPARAMETER";
    case PASTIX_ERR_FILE:
        return "PASTIX_ERR_FILE";
    case PASTIX_ERR_INTEGER_TYPE:
        return "PASTIX_ERR_INTEGER_TYPE";
    case PASTIX_ERR_IO:
        return "PASTIX_ERR_IO";
    case PASTIX_ERR_MPI:
        return "PASTIX_ERR_MPI";
    default :
        return "Bad error given";
    }
}

const char*
pastix_compress_when_getstr( pastix_compress_when_t value )
{
    switch( value ) {
    case PastixCompressNever:
        return "PastixCompressNever";
    case PastixCompressWhenBegin:
        return "PastixCompressWhenBegin";
    case PastixCompressWhenEnd:
        return "PastixCompressWhenEnd";
    case PastixCompressWhenDuring:
        return "PastixCompressWhenDuring";
    default :
        return "Bad compress_when given";
    }
}

const char*
pastix_compress_method_getstr( pastix_compress_method_t value )
{
    switch( value ) {
    case PastixCompressMethodSVD:
        return "PastixCompressMethodSVD";
    case PastixCompressMethodPQRCP:
        return "PastixCompressMethodPQRCP";
    case PastixCompressMethodRQRCP:
        return "PastixCompressMethodRQRCP";
    case PastixCompressMethodTQRCP:
        return "PastixCompressMethodTQRCP";
    case PastixCompressMethodRQRRT:
        return "PastixCompressMethodRQRRT";
    case PastixCompressMethodNbr:
        return "PastixCompressMethodNbr";
    default :
        return "Bad compress_method given";
    }
}

const char*
pastix_compress_ortho_getstr( pastix_compress_ortho_t value )
{
    switch( value ) {
    case PastixCompressOrthoCGS:
        return "PastixCompressOrthoCGS";
    case PastixCompressOrthoQR:
        return "PastixCompressOrthoQR";
    case PastixCompressOrthoPartialQR:
        return "PastixCompressOrthoPartialQR";
    default :
        return "Bad compress_ortho given";
    }
}

const char*
pastix_split_getstr( pastix_split_t value )
{
    switch( value ) {
    case PastixSplitNot:
        return "PastixSplitNot";
    case PastixSplitKway:
        return "PastixSplitKway";
    case PastixSplitKwayProjections:
        return "PastixSplitKwayProjections";
    default :
        return "Bad split given";
    }
}

const char*
pastix_layout_getstr( pastix_layout_t value )
{
    switch( value ) {
    case PastixRowMajor:
        return "PastixRowMajor";
    case PastixColMajor:
        return "PastixColMajor";
    default :
        return "Bad layout given";
    }
}

const char*
pastix_trans_getstr( pastix_trans_t value )
{
    switch( value ) {
    case PastixNoTrans:
        return "PastixNoTrans";
    case PastixTrans:
        return "PastixTrans";
    case PastixConjTrans:
        return "PastixConjTrans";
    default :
        return "Bad trans given";
    }
}

const char*
pastix_mtxtype_getstr( pastix_mtxtype_t value )
{
    switch( value ) {
    case PastixGeneral:
        return "PastixGeneral";
    case PastixSymmetric:
        return "PastixSymmetric";
    case PastixHermitian:
        return "PastixHermitian";
    default :
        return "Bad mtxtype given";
    }
}

const char*
pastix_uplo_getstr( pastix_uplo_t value )
{
    switch( value ) {
    case PastixUpper:
        return "PastixUpper";
    case PastixLower:
        return "PastixLower";
    case PastixUpperLower:
        return "PastixUpperLower";
    default :
        return "Bad uplo given";
    }
}

const char*
pastix_coefside_getstr( pastix_coefside_t value )
{
    switch( value ) {
    case PastixLCoef:
        return "PastixLCoef";
    case PastixUCoef:
        return "PastixUCoef";
    case PastixLUCoef:
        return "PastixLUCoef";
    default :
        return "Bad coefside given";
    }
}

const char*
pastix_diag_getstr( pastix_diag_t value )
{
    switch( value ) {
    case PastixNonUnit:
        return "PastixNonUnit";
    case PastixUnit:
        return "PastixUnit";
    default :
        return "Bad diag given";
    }
}

const char*
pastix_side_getstr( pastix_side_t value )
{
    switch( value ) {
    case PastixLeft:
        return "PastixLeft";
    case PastixRight:
        return "PastixRight";
    default :
        return "Bad side given";
    }
}

const char*
pastix_normtype_getstr( pastix_normtype_t value )
{
    switch( value ) {
    case PastixOneNorm:
        return "PastixOneNorm";
    case PastixFrobeniusNorm:
        return "PastixFrobeniusNorm";
    case PastixInfNorm:
        return "PastixInfNorm";
    case PastixMaxNorm:
        return "PastixMaxNorm";
    default :
        return "Bad normtype given";
    }
}

const char*
pastix_dir_getstr( pastix_dir_t value )
{
    switch( value ) {
    case PastixDirForward:
        return "PastixDirForward";
    case PastixDirBackward:
        return "PastixDirBackward";
    default :
        return "Bad dir given";
    }
}

