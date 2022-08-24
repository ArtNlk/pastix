"""
 @file pastix_dparm.py

 Declaration of the dparm parameters.

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

dparm = []

dparm_fill_in = {
    "name" : "dparm_fill_in",
    "default" : "-",
    "brief" : "Maximum memory (-DMEMORY_USAGE)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_fill_in)

dparm_epsilon_refinement = {
    "name" : "dparm_epsilon_refinement",
    "default" : "-1.",
    "brief" : "Epsilon for refinement",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_epsilon_refinement)

dparm_relative_error = {
    "name" : "dparm_relative_error",
    "default" : "-",
    "brief" : "Relative backward error",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_relative_error)

dparm_epsilon_magn_ctrl = {
    "name" : "dparm_epsilon_magn_ctrl",
    "default" : "0.",
    "brief" : "Epsilon for magnitude control",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_epsilon_magn_ctrl)

dparm_order_time = {
    "name" : "dparm_order_time",
    "default" : "-",
    "brief" : "Time for subtask order (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_order_time)

dparm_symbfact_time = {
    "name" : "dparm_symbfact_time",
    "default" : "-",
    "brief" : "Time for subtask symbfact (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_symbfact_time)

dparm_reorder_time = {
    "name" : "dparm_reorder_time",
    "default" : "-",
    "brief" : "Time for subtask reordering (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_reorder_time)

dparm_blend_time = {
    "name" : "dparm_blend_time",
    "default" : "-",
    "brief" : "Time for subtask blend (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_blend_time)

dparm_analyze_time = {
    "name" : "dparm_analyze_time",
    "default" : "-",
    "brief" : "Time for task analyse (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_analyze_time)

dparm_pred_fact_time = {
    "name" : "dparm_pred_fact_time",
    "default" : "-",
    "brief" : "Predicted factorization time",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_pred_fact_time)

dparm_fact_time = {
    "name" : "dparm_fact_time",
    "default" : "-",
    "brief" : "Time for task Numerical Factorization (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_fact_time)

dparm_fact_flops = {
    "name" : "dparm_fact_flops",
    "default" : "-",
    "brief" : "Factorization GFlops/s",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_fact_flops)

dparm_fact_thflops = {
    "name" : "dparm_fact_thflops",
    "default" : "-",
    "brief" : "Factorization theoretical Flops",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_fact_thflops)

dparm_fact_rlflops = {
    "name" : "dparm_fact_rlflops",
    "default" : "-",
    "brief" : "Factorization performed Flops",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_fact_rlflops)

dparm_solv_time = {
    "name" : "dparm_solv_time",
    "default" : "-",
    "brief" : "Time for task Solve (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_solv_time)

dparm_solv_flops = {
    "name" : "dparm_solv_flops",
    "default" : "-",
    "brief" : "Solve GFlops/s",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_solv_flops)

dparm_solv_thflops = {
    "name" : "dparm_solv_thflops",
    "default" : "-",
    "brief" : "Solve theoretical Flops",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_solv_thflops)

dparm_solv_rlflops = {
    "name" : "dparm_solv_rlflops",
    "default" : "-",
    "brief" : "Solve performed Flops",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_solv_rlflops)

dparm_refine_time = {
    "name" : "dparm_refine_time",
    "default" : "-",
    "brief" : "Time for task refinement (wallclock)",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_refine_time)

dparm_a_norm = {
    "name" : "dparm_a_norm",
    "default" : "-",
    "brief" : "(||A||_f) norm",
    "access" : "OUT",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_a_norm)

dparm_compress_tolerance = {
    "name" : "dparm_compress_tolerance",
    "default" : "0.01",
    "brief" : "Tolerance for low-rank kernels",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_compress_tolerance)

dparm_compress_min_ratio = {
    "name" : "dparm_compress_min_ratio",
    "default" : "1.0",
    "brief" : "Min ratio for rank w.r.t. strict rank",
    "access" : "IN",
    "description" : r'''
A long description in the doxygen format
'''
}
dparm.append(dparm_compress_min_ratio)
