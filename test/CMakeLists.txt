###
#
#  @copyright 2013-2022 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.2.0
#  @author Mathieu Faverge
#  @author Esragul Korkmaz
#  @author Florent Pruvost
#  @author Matias Hastaran
#  @author Pierre Ramet
#  @author Tony Delarue
#  @author Vincent Bridonneau
#  @date 2021-02-12
#
###
include(RulesPrecisions)

### Generate the headers in all precisions
set(HEADERS
  z_tests.h
)

precisions_rules_py(generated_headers
  "${HEADERS}"
  PRECISIONS "s;d;c;z")

set(tests_headers
  ${generated_headers}
  )

add_custom_target(tests_headers_tgt
  DEPENDS ${tests_headers} )

## Generate the test library files for all required precisions
set(LIB_SOURCES
  z_bcsc_tests.c
  z_bvec_tests.c
)

## reset variables
set(generated_libfiles "")

precisions_rules_py(generated_libfiles
   "${LIB_SOURCES}"
   PRECISIONS "p;s;d;c;z")

add_library( bcsc_test
  ${generated_libfiles}
  )

add_dependencies( bcsc_test
  tests_headers_tgt
  )

target_include_directories( bcsc_test PRIVATE
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

target_include_directories( bcsc_test PUBLIC
  $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/test>
  $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/test> )

target_link_libraries( bcsc_test PUBLIC
  pastix
  pastix_kernels
  spm
  MORSE::LAPACKE
  MORSE::CBLAS
  MORSE::M
  )

## Generate all test executables
set (TESTS
  bcsc_norm_tests.c
  bcsc_spmv_tests.c
  bcsc_spmv_time.c
  bvec_gemv_tests.c
  bvec_tests.c
  )

foreach (_file ${TESTS})
  get_filename_component(_name_we ${_file} NAME_WE)
  add_executable(${_name_we} ${_file})

  target_include_directories( ${_name_we} PRIVATE
    $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
    $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

  target_link_libraries(${_name_we} PRIVATE
    pastix bcsc_test )
endforeach()

## CTest execution
set( BCSC_TESTS )

if ( NOT PASTIX_WITH_MPI )
  list( APPEND BCSC_TESTS
    bcsc_norm_tests
    )
endif()

list( APPEND BCSC_TESTS
  bcsc_spmv_tests
  bcsc_spmv_time
  )

set( BVEC_TESTS
  bvec_gemv_tests
  bvec_tests
  )
set( RUNTYPE shm )

# Add runtype
if (PASTIX_WITH_MPI)
  list( APPEND RUNTYPE mpi )
endif()

foreach( version ${RUNTYPE} )
  unset( exe )
  if( version STREQUAL "shm" )
    set( exe "")
  endif()
  if( version STREQUAL "mpi" )
    set( exe mpiexec -np 4 )
  endif()

  # Bcsc tests
  foreach(example ${BCSC_TESTS} )
    set( _test_name bcsc_${version}_test_${example} )
    set( _test_cmd  ${exe} ./${example} )

    # Laplacian
    foreach(arithm ${PASTIX_PRECISIONS} )
      add_test(${_test_name}_lap_${arithm} ${_test_cmd} --lap ${arithm}:10:10:10)
    endforeach()

    # RSA
    add_test(${_test_name}_rsa ${_test_cmd} --rsa ${CMAKE_SOURCE_DIR}/test/matrix/small.rsa)

    # Matrix Market
    add_test(${_test_name}_mm ${_test_cmd} --mm ${CMAKE_SOURCE_DIR}/test/matrix/young4c.mtx)

    # Harwell Boeing
    add_test(${_test_name}_hb ${_test_cmd} --hb ${CMAKE_SOURCE_DIR}/test/matrix/orsirr.rua)

    # Matrix Market - Hermitian
    add_test(${_test_name}_mm2 ${_test_cmd} --mm ${CMAKE_SOURCE_DIR}/test/matrix/mhd1280b.mtx)
  endforeach()

  # Bvec tests
  foreach(example ${BVEC_TESTS} )
    set( _test_name bcsc_${version}_test_${example} )
    set( _test_cmd  ${exe} ./${example} )

    add_test(${_test_name} ${_test_cmd})
  endforeach()
endforeach()

###################################
#   Low-rank testings
###
if(PASTIX_LR_TESTINGS)

  if(TMG_FOUND)
    ## Generate the test library files for all required precisions
    set(LRLIB_SOURCES
      z_lowrank_tests.c
      )

    ## reset variables
    set(generated_lrlibfiles "")

    precisions_rules_py(generated_lrlibfiles
      "${LRLIB_SOURCES}"
      PRECISIONS "p;s;d;c;z")

    add_library( pastix_lrtests
      ${generated_lrlibfiles}
      tests_auxiliary.c
      )

    add_dependencies( pastix_lrtests
      tests_headers_tgt
      )

    target_include_directories( pastix_lrtests PRIVATE
      $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
      $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
      $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

    target_include_directories( pastix_lrtests PUBLIC
      $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/test>
      $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/test> )

    target_link_libraries( pastix_lrtests PUBLIC
      pastix
      pastix_kernels
      MORSE::LAPACKE
      MORSE::TMG
      MORSE::CBLAS
      MORSE::M
      )

    set(LR_TESTS_SOURCES
      z_ge2lr_performance.c
      z_ge2lr_stability.c
      z_ge2lr_tests.c
      z_lrmm_tests.c
      z_rradd_tests.c
      )

    ## reset variables
    set(generated_lrfiles)

    precisions_rules_py(generated_lrfiles
      "${LR_TESTS_SOURCES}"
      PRECISIONS "p;s;d;c;z")

    set( testings
      ${generated_lrfiles} )

    foreach (_file ${testings})
      get_filename_component(_name_we ${_file} NAME_WE)
      add_executable(${_name_we} ${_file})


      target_include_directories( ${_name_we} PRIVATE
        $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}>
        $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}>
        $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/common> )

      target_include_directories( ${_name_we} PUBLIC
        $<BUILD_INTERFACE:${PASTIX_SOURCE_DIR}/test>
        $<BUILD_INTERFACE:${PASTIX_BINARY_DIR}/test> )


      target_link_libraries( ${_name_we} PRIVATE
        pastix_lrtests
        pastix_kernels
        MORSE::LAPACKE
        MORSE::CBLAS
        MORSE::M
        )

      add_test( lr_shm_test_${_name_we} ./${_name_we} -n 300 -p 10 )
    endforeach()

  endif()
endif()
