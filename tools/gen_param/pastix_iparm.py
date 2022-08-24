"""
 @file pastix_iparm.py

 Declaration of the iparm parameters.

 @copyright 2021-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
                      Univ. Bordeaux. All rights reserved.

 @version 6.2.0
 @author Tony Delarue
 @date 2021-04-07

 This file allows us to generate:
      - Documentation files
      - IPARM/DPARM and their related enums declaration file
        ( $PASTIX_HOME/include/pastix/api.h )
      - parse_iparm, parse_dparm and parse_enums implementation and declaration.
        pastix_ENUM_getstr implementation and declaration.
        ( $PASTIX_HOME/common/parse_options.[h/c] )W

 If you want to modify one of these files, please modify this one.

 ***

 SYNTAX documentation. [] are used for optional keys.

 parm_name = {
   "default": Default value. If none, please write ("-")
   ["description"]: Full description for documentation.
   "brief": Short description for the refcard / comments.
   "access": "IN"/"OUT"
   ["min"]: Parm min value.
   ["max"]: Parm max value.
   ["enum"]: This PARM is related to an enum.
 }
"""

iparm = []

iparm_verbose = {
    "name" : "iparm_verbose",
    "default" : "PastixVerboseNo",
    "brief" : "Verbose mode (@see pastix_verbose_t)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "verbose",
}

iparm_io_strategy = {
    "name" : "iparm_io_strategy",
    "default" : "PastixIONo",
    "brief" : "IO strategy  (@see pastix_io_t)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "io",
}

iparm_none_group = {
    "subgroup" : [
        iparm_verbose,
        iparm_io_strategy,
    ],
    "name" : "none",
    "brief" : "None",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_none_group)

iparm_nnzeros = {
    "name" : "iparm_nnzeros",
    "default" : "-",
    "brief" : "Number of nonzero entries in the factorized matrix",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_nnzeros_block_local = {
    "name" : "iparm_nnzeros_block_local",
    "default" : "-",
    "brief" : "Number of nonzero entries in the local block factorized matrix",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_allocated_terms = {
    "name" : "iparm_allocated_terms",
    "default" : "-",
    "brief" : "Maximum memory allocated for matrix terms",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_produce_stats = {
    "name" : "iparm_produce_stats",
    "default" : "0",
    "brief" : "Compute some statistiques (such as precision error)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_stats_group = {
    "subgroup" : [
        iparm_nnzeros,
        iparm_nnzeros_block_local,
        iparm_allocated_terms,
        iparm_produce_stats,
    ],
    "name" : "stats",
    "brief" : "Stats",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_stats_group)

iparm_mc64 = {
    "name" : "iparm_mc64",
    "default" : "0",
    "brief" : "MC64 operation",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_scaling_group = {
    "subgroup" : [
        iparm_mc64,
    ],
    "name" : "scaling",
    "brief" : "Scaling",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_scaling_group)

iparm_ordering = {
    "name" : "iparm_ordering",
    "default" : "PastixOrderScotch",
    "brief" : "Choose ordering",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "ordering",
}

iparm_ordering_default = {
    "name" : "iparm_ordering_default",
    "default" : "1",
    "brief" : "Use default ordering parameters with Scotch or Metis",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_ordering_group = {
    "subgroup" : [
        iparm_ordering,
        iparm_ordering_default,
    ],
    "name" : "ordering",
    "brief" : "Ordering",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_ordering_group)

iparm_scotch_mt = {
    "name" : "iparm_scotch_mt",
    "default" : "1 (if available)",
    "brief" : "Ordering multi-threaded  (see Scotch Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_scotch_switch_level = {
    "name" : "iparm_scotch_switch_level",
    "default" : "120",
    "brief" : "Ordering switch level    (see Scotch Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_scotch_cmin = {
    "name" : "iparm_scotch_cmin",
    "default" : "0",
    "brief" : "Ordering cmin parameter  (see Scotch Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_scotch_cmax = {
    "name" : "iparm_scotch_cmax",
    "default" : "100000",
    "brief" : "Ordering cmax parameter  (see Scotch Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_scotch_frat = {
    "name" : "iparm_scotch_frat",
    "default" : "8",
    "brief" : "Ordering frat parameter  (see Scotch Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_subset_for_scotch_group = {
    "subgroup" : [
        iparm_scotch_mt,
        iparm_scotch_switch_level,
        iparm_scotch_cmin,
        iparm_scotch_cmax,
        iparm_scotch_frat,
    ],
    "name" : "subset_for_scotch",
    "brief" : "Subset for Scotch",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_subset_for_scotch_group)

iparm_metis_ctype = {
    "name" : "iparm_metis_ctype",
    "default" : "METIS_CTYPE_SHEM",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_rtype = {
    "name" : "iparm_metis_rtype",
    "default" : "METIS_RTYPE_SEP1SIDED",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_no2hop = {
    "name" : "iparm_metis_no2hop",
    "default" : "0",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_nseps = {
    "name" : "iparm_metis_nseps",
    "default" : "1",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_niter = {
    "name" : "iparm_metis_niter",
    "default" : "10",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_ufactor = {
    "name" : "iparm_metis_ufactor",
    "default" : "200",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_compress = {
    "name" : "iparm_metis_compress",
    "default" : "1",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_ccorder = {
    "name" : "iparm_metis_ccorder",
    "default" : "0",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_pfactor = {
    "name" : "iparm_metis_pfactor",
    "default" : "0",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_seed = {
    "name" : "iparm_metis_seed",
    "default" : "3452",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_metis_dbglvl = {
    "name" : "iparm_metis_dbglvl",
    "default" : "0",
    "brief" : "Metis parameters (see Metis Manual)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_subset_for_metis_group = {
    "subgroup" : [
        iparm_metis_ctype,
        iparm_metis_rtype,
        iparm_metis_no2hop,
        iparm_metis_nseps,
        iparm_metis_niter,
        iparm_metis_ufactor,
        iparm_metis_compress,
        iparm_metis_ccorder,
        iparm_metis_pfactor,
        iparm_metis_seed,
        iparm_metis_dbglvl,
    ],
    "name" : "subset_for_metis",
    "brief" : "Subset for Metis",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_subset_for_metis_group)

iparm_amalgamation_lvlblas = {
    "name" : "iparm_amalgamation_lvlblas",
    "default" : "5",
    "brief" : "Amalgamation level",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_amalgamation_lvlcblk = {
    "name" : "iparm_amalgamation_lvlcblk",
    "default" : "5",
    "brief" : "Amalgamation level",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_symbolic_factorization_group = {
    "subgroup" : [
        iparm_amalgamation_lvlblas,
        iparm_amalgamation_lvlcblk,
    ],
    "name" : "symbolic_factorization",
    "brief" : "Symbolic Factorization",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_symbolic_factorization_group)

iparm_reordering_split = {
    "name" : "iparm_reordering_split",
    "default" : "0",
    "brief" : "Reordering split level",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_reordering_stop = {
    "name" : "iparm_reordering_stop",
    "default" : "PASTIX_INT_MAX",
    "brief" : "Reordering stop criterion",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_splitting_strategy = {
    "name" : "iparm_splitting_strategy",
    "default" : "PastixSplitKway",
    "brief" : "Strategy used to split supernodes",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "split",
}

iparm_splitting_levels_projections = {
    "name" : "iparm_splitting_levels_projections",
    "default" : "0",
    "brief" : "Levels of projections",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_splitting_levels_kway = {
    "name" : "iparm_splitting_levels_kway",
    "default" : "PASTIX_INT_MAX",
    "brief" : "Levels of kway",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_splitting_projections_depth = {
    "name" : "iparm_splitting_projections_depth",
    "default" : "3",
    "brief" : "Number of level used for projections",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_splitting_projections_distance = {
    "name" : "iparm_splitting_projections_distance",
    "default" : "3",
    "brief" : "Distance used for projections",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_splitting_projections_width = {
    "name" : "iparm_splitting_projections_width",
    "default" : "1",
    "brief" : "Width used for projections",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_reordering_group = {
    "subgroup" : [
        iparm_reordering_split,
        iparm_reordering_stop,
        iparm_splitting_strategy,
        iparm_splitting_levels_projections,
        iparm_splitting_levels_kway,
        iparm_splitting_projections_depth,
        iparm_splitting_projections_distance,
        iparm_splitting_projections_width,
    ],
    "name" : "reordering",
    "brief" : "Reordering",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_reordering_group)

iparm_min_blocksize = {
    "name" : "iparm_min_blocksize",
    "default" : "160",
    "brief" : "Minimum block size",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_max_blocksize = {
    "name" : "iparm_max_blocksize",
    "default" : "320",
    "brief" : "Maximum block size",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_tasks2d_level = {
    "name" : "iparm_tasks2d_level",
    "default" : "-1",
    "brief" : "2D Distribution level (-1 for autolevel, 0 for 1D)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_tasks2d_width = {
    "name" : "iparm_tasks2d_width",
    "default" : "IPARM_MIN_BLOCKSIZE",
    "brief" : "Minimal width for 2D tasks with autolevel",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_allcand = {
    "name" : "iparm_allcand",
    "default" : "0",
    "brief" : "Allow all threads to be candidate in the proportional mapping",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_analyze_group = {
    "subgroup" : [
        iparm_min_blocksize,
        iparm_max_blocksize,
        iparm_tasks2d_level,
        iparm_tasks2d_width,
        iparm_allcand,
    ],
    "name" : "analyze",
    "brief" : "Analyze",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_analyze_group)

iparm_incomplete = {
    "name" : "iparm_incomplete",
    "default" : "0",
    "brief" : "Incomplete factorization",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_level_of_fill = {
    "name" : "iparm_level_of_fill",
    "default" : "0",
    "brief" : "Level of fill for incomplete factorization",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_incomplete_group = {
    "subgroup" : [
        iparm_incomplete,
        iparm_level_of_fill,
    ],
    "name" : "incomplete",
    "brief" : "Incomplete",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_incomplete_group)

iparm_factorization = {
    "name" : "iparm_factorization",
    "default" : "PastixFactLU",
    "brief" : "Factorization mode",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "factotype",
}

iparm_static_pivoting = {
    "name" : "iparm_static_pivoting",
    "default" : "-",
    "brief" : "Static pivoting",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_free_cscuser = {
    "name" : "iparm_free_cscuser",
    "default" : "0",
    "brief" : "Free user CSC",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_schur_fact_mode = {
    "name" : "iparm_schur_fact_mode",
    "default" : "PastixFactModeLocal",
    "brief" : "Specify if the Schur is factorized (@see pastix_fact_mode_t)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "fact_mode",
}

iparm_factorization_group = {
    "subgroup" : [
        iparm_factorization,
        iparm_static_pivoting,
        iparm_free_cscuser,
        iparm_schur_fact_mode,
    ],
    "name" : "factorization",
    "brief" : "Factorization",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_factorization_group)

iparm_transpose_solve = {
    "name" : "iparm_transpose_solve",
    "default" : "PastixNoTrans",
    "brief" : "Solve A^t x = b (to avoid CSR/CSC conversion for instance)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "trans",
}

iparm_schur_solv_mode = {
    "name" : "iparm_schur_solv_mode",
    "default" : "PastixSolvModeLocal",
    "brief" : "Specify the solve parts to apply (@see pastix_solv_mode_t)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "solv_mode",
}

iparm_applyperm_ws = {
    "name" : "iparm_applyperm_ws",
    "default" : "1",
    "brief" : "Enable/disable extra workspace for a thread-safe swap",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_solve_group = {
    "subgroup" : [
        iparm_transpose_solve,
        iparm_schur_solv_mode,
        iparm_applyperm_ws,
    ],
    "name" : "solve",
    "brief" : "Solve",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_solve_group)

iparm_refinement = {
    "name" : "iparm_refinement",
    "default" : "PastixRefineGMRES",
    "brief" : "Refinement mode",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "refine",
}

iparm_nbiter = {
    "name" : "iparm_nbiter",
    "default" : "-",
    "brief" : "Number of iterations performed in refinement",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_itermax = {
    "name" : "iparm_itermax",
    "default" : "250",
    "brief" : "Maximum iteration number for refinement",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_gmres_im = {
    "name" : "iparm_gmres_im",
    "default" : "25",
    "brief" : "GMRES restart parameter",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_refinement_group = {
    "subgroup" : [
        iparm_refinement,
        iparm_nbiter,
        iparm_itermax,
        iparm_gmres_im,
    ],
    "name" : "refinement",
    "brief" : "Refinement",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_refinement_group)

iparm_scheduler = {
    "name" : "iparm_scheduler",
    "default" : "PastixSchedDynamic",
    "brief" : "Scheduler mode",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "scheduler",
}

iparm_thread_nbr = {
    "name" : "iparm_thread_nbr",
    "default" : "-1",
    "brief" : "Number of threads per process (-1 for auto detect)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_autosplit_comm = {
    "name" : "iparm_autosplit_comm",
    "default" : "0",
    "brief" : "Automaticaly split communicator to have one MPI task by node",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_context_group = {
    "subgroup" : [
        iparm_scheduler,
        iparm_thread_nbr,
        iparm_autosplit_comm,
    ],
    "name" : "context",
    "brief" : "Context",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_context_group)

iparm_gpu_nbr = {
    "name" : "iparm_gpu_nbr",
    "default" : "0",
    "brief" : "Number of GPU devices",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_gpu_memory_percentage = {
    "name" : "iparm_gpu_memory_percentage",
    "default" : "95",
    "brief" : "Maximum percentage of the GPU memory used by the solver",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_gpu_memory_block_size = {
    "name" : "iparm_gpu_memory_block_size",
    "default" : "32 * 1024",
    "brief" : "Size of GPU memory pages (for PaRSEC runtime)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_gpu_group = {
    "subgroup" : [
        iparm_gpu_nbr,
        iparm_gpu_memory_percentage,
        iparm_gpu_memory_block_size,
    ],
    "name" : "gpu",
    "brief" : "GPU",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_gpu_group)

iparm_compress_min_width = {
    "name" : "iparm_compress_min_width",
    "default" : "128",
    "brief" : "Minimum width to compress a supernode",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_compress_min_height = {
    "name" : "iparm_compress_min_height",
    "default" : "20",
    "brief" : "Minimum height to compress an off-diagonal block",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_compress_when = {
    "name" : "iparm_compress_when",
    "default" : "PastixCompressNever",
    "brief" : "When to compress a supernode",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "compress_when",
}

iparm_compress_method = {
    "name" : "iparm_compress_method",
    "default" : "PastixCompressMethodPQRCP",
    "brief" : "Compression method (See pastix_compress_method_t)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "compress_method",
}

iparm_compress_ortho = {
    "name" : "iparm_compress_ortho",
    "default" : "PastixCompressOrthoCGS",
    "brief" : "Orthogonalization method",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "compress_ortho",
}

iparm_compress_reltol = {
    "name" : "iparm_compress_reltol",
    "default" : "0",
    "brief" : "Enable/Disable relative tolerance",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_compress_preselect = {
    "name" : "iparm_compress_preselect",
    "default" : "1",
    "brief" : "Enable/Disable compression of preselected blocks",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_compress_iluk = {
    "name" : "iparm_compress_iluk",
    "default" : "-2",
    "brief" : "Set the ILU(k) level of preselection (-2 for auto-level)",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_compression_group = {
    "subgroup" : [
        iparm_compress_min_width,
        iparm_compress_min_height,
        iparm_compress_when,
        iparm_compress_method,
        iparm_compress_ortho,
        iparm_compress_reltol,
        iparm_compress_preselect,
        iparm_compress_iluk,
    ],
    "name" : "compression",
    "brief" : "Compression",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_compression_group)

iparm_mpi_thread_level = {
    "name" : "iparm_mpi_thread_level",
    "default" : "PastixMpiNone",
    "brief" : "MPI thread level support",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "mpithreadmode",
}

iparm_mpi_modes_group = {
    "subgroup" : [
        iparm_mpi_thread_level,
    ],
    "name" : "mpi_modes",
    "brief" : "MPI modes",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_mpi_modes_group)

iparm_modify_parameter = {
    "name" : "iparm_modify_parameter",
    "default" : "1",
    "brief" : "Indicate if parameters have been set by user",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_start_task = {
    "name" : "iparm_start_task",
    "default" : "PastixTaskOrdering",
    "brief" : "Indicate the first step to execute",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "task",
}

iparm_end_task = {
    "name" : "iparm_end_task",
    "default" : "PastixTaskClean",
    "brief" : "Indicate the last step to execute",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "task",
}

iparm_float = {
    "name" : "iparm_float",
    "default" : "PastixDouble",
    "brief" : "Indicate the arithmetics",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
    "enum" : "coeftype",
}

iparm_mtx_type = {
    "name" : "iparm_mtx_type",
    "default" : "-1",
    "brief" : "Indicate matrix format",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_dof_nbr = {
    "name" : "iparm_dof_nbr",
    "default" : "1",
    "brief" : "Degree of freedom per node",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
''',
}

iparm_subset_for_old_interface_group = {
    "subgroup" : [
        iparm_modify_parameter,
        iparm_start_task,
        iparm_end_task,
        iparm_float,
        iparm_mtx_type,
        iparm_dof_nbr,
    ],
    "name" : "subset_for_old_interface",
    "brief" : "Subset for old interface",
    "description" :r'''
Long description in the doxygen format
''',  # Optional
}
iparm.append(iparm_subset_for_old_interface_group)
