###
#
#  @copyright 2017      Bordeaux INP, CNRS (LaBRI UMR 5800), Inria,
#                       Univ. Bordeaux. All rights reserved.
#
#  @version 6.0.0
#  @author Mathieu Faverge
#  @date 2017-05-22
#
###
cmake_minimum_required (VERSION 3.1)

# Coherce CMake to install the generated .mod files
set(CMAKE_Fortran_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/mod_files)
install(DIRECTORY ${CMAKE_Fortran_MODULE_DIRECTORY}/ DESTINATION include)

add_library( pastixf
  src/pastix_enums.F90
  src/spmf.f90
  src/pastixf.f90 )

if ( PASTIX_INT64 )
  set_source_files_properties(
    src/pastix_enums.F90
    PROPERTIES COMPILE_DEFINITIONS "PASTIX_INT_KIND=8")
else()
  set_source_files_properties(
    src/pastix_enums.F90
    PROPERTIES COMPILE_DEFINITIONS "PASTIX_INT_KIND=4")
endif()

target_link_libraries( pastixf pastix )
install(TARGETS pastixf
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib )

#
# Add examples
#
set (EXAMPLES
  fsimple.f90
  flaplacian.f90
  )

foreach (_file ${EXAMPLES})
  get_filename_component(_name_we ${_file} NAME_WE)
  add_executable(${_name_we} examples/${_file})
  target_link_libraries(${_name_we} pastixf)

  install(TARGETS ${_name_we}       RUNTIME DESTINATION examples )
  install(FILES   examples/${_file}         DESTINATION examples )
endforeach()