###
#
#  @copyright 2013-2023 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.3.0
#  @author Mathieu Faverge
#  @author Tony Delarue
#  @date 2023-01-13
#
###
include(RulesPrecisions)

# reset variables
set(cuda_generated_files "")
set(generated_headers "")

### Generate .c files from .jdf for all required precisions
set(SOURCES
  kepler/pastix_zgemm_vbatched.cu
  fermi/sparse_zgemm_fermi.cu
)

precisions_rules_py(cuda_generated_files
  "${SOURCES}"
  PRECISIONS "s;d;c;z")

# Change the paths if in binary dir
foreach( _cudafile ${cuda_generated_files} )
  get_source_file_property(_IsInBinaryDir ${_cudafile} IS_IN_BINARY_DIR )
  if( _IsInBinaryDir )
    string(REGEX REPLACE
      ${_cudafile}
      ${CMAKE_CURRENT_BINARY_DIR}/${_cudafile}
      cuda_generated_files "${cuda_generated_files}")
  endif( _IsInBinaryDir )
endforeach()

set_source_files_properties(
  ${cuda_generated_files}
  PROPERTIES DEPENDS kernels_headers
  )

# # For Fermi
# set(CUDA_NVCC_FLAGS_20 -arch sm_20 -DCUDA_SM_VERSION=20)
# # For Kepler
# set(CUDA_NVCC_FLAGS_30 -arch sm_30 -DCUDA_SM_VERSION=30)
# set(CUDA_NVCC_FLAGS_32 -arch sm_32 -DCUDA_SM_VERSION=32)
# set(CUDA_NVCC_FLAGS_35 -arch sm_35 -DCUDA_SM_VERSION=35)
# # For Maxwell
# set(CUDA_NVCC_FLAGS_50 -arch sm_50 -DCUDA_SM_VERSION=50)
# set(CUDA_NVCC_FLAGS_52 -arch sm_52 -DCUDA_SM_VERSION=52)
# set(CUDA_NVCC_FLAGS_53 -arch sm_53 -DCUDA_SM_VERSION=53)
#
# set(CUDA_SM_TARGETS 20 30 32 35 CACHE STRING "Specify GPU architectures to build binaries for, BIN(PTX) format is supported")
#
# set(CUDA_NVCC_FLAGS_BACKUP ${CUDA_NVCC_FLAGS})
# foreach( _smtarget ${CUDA_SM_TARGETS} )
#   set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS_${_smtarget}} ${CUDA_NVCC_FLAGS_BACKUP})

#   cuda_add_library(pastix_cucores_sm${_smtarget}
#     ${cuda_generated_files})
#   cuda_add_cublas_to_target( pastix_cucores_sm${_smtarget} )
#   install(TARGETS pastix_cucores_sm${_smtarget}
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib )
#   # if( NOT CMAKE_BUILD_SHARED_LIBS )
#   #   target_link_libraries( pastix_cucores pastix_cucores_sm${_smtarget} )
#   # endif()
# endforeach()
# set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS_BACKUP})

cuda_add_library(pastix_kernels_cuda
  ${cuda_generated_files})

cuda_add_cublas_to_target( pastix_kernels_cuda )
#target_link_libraries( pastix_kernels_cuda PRIVATE CUDA::CUBLAS )

set_property(TARGET pastix_kernels_cuda PROPERTY CXX_STANDARD 11)

target_include_directories( pastix_kernels_cuda PRIVATE
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

target_include_directories( pastix_kernels_cuda PUBLIC
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include> )

# TODO: Add PRIVATE dependency when updating the FindCUDA
target_link_libraries( pastix_kernels_cuda SPM::spm )

# export target pastix_kernels
# ----------------------------
install(EXPORT pastix_kernels_cudaTargets
  NAMESPACE PASTIX::
  DESTINATION lib/cmake/pastix
  )

# Installation
# ------------
install(TARGETS pastix_kernels_cuda
        EXPORT pastix_kernels_cudaTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

cuda_build_clean_target()
