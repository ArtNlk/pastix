###
#
#  @copyright 2013-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.2.0
#  @author Mathieu Faverge
#  @author Esragul Korkmaz
#  @author Gregoire Pichon
#  @author Tony Delarue
#  @date 2021-03-16
#
###
include(RulesPrecisions)
include(AddSourceFiles)

# reset variables
set(generated_sources "")
set(generated_headers "")

### Add trace generation
if(PASTIX_WITH_EZTRACE)
  add_library(eztrace-convert-kernels SHARED
        eztrace_module/eztrace_convert_kernels.c
  )

  target_include_directories( eztrace-convert-kernels PRIVATE
    $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}> )

  target_include_directories( eztrace-convert-kernels PUBLIC
    $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include> )

  target_link_libraries( eztrace-convert-kernels PUBLIC MORSE::EZTRACE )
  target_link_libraries( eztrace-convert-kernels PRIVATE MORSE::LITL   )
endif(PASTIX_WITH_EZTRACE)

### Generate the headers in all precisions
set(HEADERS
  pastix_zcores.h
  pastix_zcuda.h
  pastix_zlrcores.h
  z_nan_check.h
)

precisions_rules_py(generated_headers
  "${HEADERS}"
  PRECISIONS "s;d;c;z")

set(kernels_headers
  ${generated_headers}
  queue.h
  )

add_custom_target(kernels_headers_tgt
  DEPENDS ${kernels_headers} )

### Generate the sources in all precisions
set(SOURCES
  # CPU extra kernel
  core_zgemdm.c
  core_zgetmo.c
  core_zgeadd.c
  core_zplrnt.c
  core_ztradd.c
  core_zgemmsp.c
  core_ztrsmsp.c
  core_zscalo.c
  # Kernels
  core_zsytrfsp.c
  core_zhetrfsp.c
  core_zpotrfsp.c
  core_zpxtrfsp.c
  core_zgetrfsp.c
  # Low-rank kernels
  core_zlrnrm.c
  core_zlrdbg.c
  core_zlr2xx.c
  core_zxx2lr.c
  core_zxx2fr.c
  core_zlrmm.c
  core_zpqrcp.c
  core_zrqrcp.c
  core_ztqrcp.c
  core_zrqrrt.c
  core_zlrothu.c
  core_zgelrops_svd.c
  core_zgelrops.c
  # cblk operations
  cpucblk_zinit.c
  cpucblk_zcompress.c
  cpucblk_zdiff.c
  cpucblk_zadd.c
  cpucblk_zschur.c
  # MPI operations
  cpucblk_zmpi_coeftab.c
  cpucblk_zmpi_rhs.c
  # Solve operation
  solve_ztrsmsp.c
)

precisions_rules_py(generated_sources
  "${SOURCES}"
  PRECISIONS "s;d;c;z")

set(kernels_sources
  ${generated_sources}
  kernels.c
  kernels_trace.c
  lowrank.c
  queue.c
  )

if(PASTIX_WITH_CUDA)
   set( CUDA_SOURCES
     # GPU kernel
     gpu_zgemmsp.c
    )
   set(CUDA_generated_files "")
   precisions_rules_py(CUDA_generated_files
        "${CUDA_SOURCES}"
        PRECISIONS "s;d;c;z")

   set(kernels_sources
       ${kernels_sources}
       ${CUDA_generated_files})

   add_subdirectory(gpus)

endif(PASTIX_WITH_CUDA)

add_library(pastix_kernels
  ${kernels_sources}
  )

add_dependencies(pastix_kernels
  kernels_headers_tgt
)

target_include_directories( pastix_kernels PRIVATE
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

target_include_directories( pastix_kernels PUBLIC
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include> )

target_link_libraries( pastix_kernels PUBLIC spm            )
target_link_libraries( pastix_kernels PUBLIC MORSE::LAPACKE )
target_link_libraries( pastix_kernels PUBLIC MORSE::CBLAS   )
target_link_libraries( pastix_kernels PUBLIC MORSE::M       )

if(PASTIX_WITH_EZTRACE)
  target_link_libraries( pastix_kernels PUBLIC MORSE::EZTRACE )
  target_link_libraries( pastix_kernels PRIVATE MORSE::LITL   )

endif(PASTIX_WITH_EZTRACE)

if(PASTIX_WITH_CUDA)
    # if(PASTIX_CUDA_FERMI)
    #   target_link_libraries( pastix_kernels
    #                          pastix_cucores_sm20 )
    # else()
    #   target_link_libraries( pastix_kernels
    #                          pastix_cucores_sm35 )
    # endif()
    target_link_libraries( pastix_kernels PRIVATE pastix_kernels_cuda )
endif()

# export target pastix_kernels
# ----------------------------
install(EXPORT pastix_kernelsTargets
  NAMESPACE PASTIX::
  DESTINATION lib/cmake/pastix
  )

# Installation
# ------------
install(TARGETS pastix_kernels
        EXPORT pastix_kernelsTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

### Add generated files to the global property
add_documented_files(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  ${generated_headers}
  ${generated_sources}
  )

### Add non generated files to the global property
add_documented_files(
  # Headers
  kernels.h
  queue.h
  # Source files
  kernels.c
  kernels_trace.c
  lowrank.c
  queue.c
  )
