###
#
# @copyright (c) 2009-2014 The University of Tennessee and The University
#                          of Tennessee Research Foundation.
#                          All rights reserved.
# @copyright (c) 2012-2014 Inria. All rights reserved.
# @copyright (c) 2012-2014 Bordeaux INP, CNRS (LaBRI UMR 5800), Inria, Univ. Bordeaux. All rights reserved.
#
###
#
# - Find MUMPS include dirs and libraries
# Use this module by invoking find_package with the form:
#  find_package(MUMPS
#               [REQUIRED]             # Fail with error if mumps is not found
#               [COMPONENTS <libs>...] # required dependencies
#              )
#  COMPONENTS can be one of the following:
#   MPI: to activate detection of the parallel MPI version (default behaviour)
#   SEQ: to activate detection of sequential version (exclude MPI version)
#   SCOTCH: to activate detection of MUMPS linked with SCOTCH
#   METIS: to activate detection of MUMPS linked with METIS
# This module finds headers and mumps library.
# Results are reported in variables:
#  MUMPS_FOUND           - True if headers and requested libraries were found
#  MUMPS_INCLUDE_DIRS    - mumps include directories
#  MUMPS_LIBRARY_DIRS    - Link directories for mumps libraries
# The user can give specific paths where to find the libraries adding cmake
# options at configure (ex: cmake path/to/project -DMUMPS_DIR=path/to/mumps):
#  MUMPS_DIR             - Where to find the base directory of mumps
#  MUMPS_INCDIR          - Where to find the header files
#  MUMPS_LIBDIR          - Where to find the library files

#=============================================================================
# Copyright 2012-2013 Inria
# Copyright 2012-2013 Emmanuel Agullo
# Copyright 2012-2013 Mathieu Faverge
# Copyright 2012      Cedric Castagnede
# Copyright 2013      Florent Pruvost
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file MORSE-Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of Morse, substitute the full
#  License text for the above reference.)


if (NOT MUMPS_FOUND)
    set(MUMPS_DIR "" CACHE PATH "Root directory of MUMPS library")
    if (NOT MUMPS_FIND_QUIETLY)
        message(STATUS "A cache variable, namely MUMPS_DIR, has been set to specify the install directory of MUMPS")
    endif()
endif()

# Try to find MUMPS dependencies if specified as COMPONENTS during the call
set(MUMPS_LOOK_FOR_SEQ OFF)
set(MUMPS_LOOK_FOR_MPI ON)
if( MUMPS_FIND_COMPONENTS )
    foreach( component ${MUMPS_FIND_COMPONENTS} )
        if (${component} STREQUAL "SEQ")
            # means we look for the sequential version of MUMPS (without MPI)
            set(MUMPS_LOOK_FOR_SEQ ON)
            set(MUMPS_LOOK_FOR_MPI OFF)
        endif()
        if (${component} STREQUAL "MPI")
            # means we look for the sequential version of MUMPS (without MPI)
            set(MUMPS_LOOK_FOR_MPI ON)
            set(MUMPS_LOOK_FOR_SEQ OFF)
        endif()
        if (NOT ${component} STREQUAL "SEQ")
            if(MUMPS_FIND_REQUIRED_${component})
                find_package(${component} REQUIRED)
            else()
                find_package(${component})
            endif()
        endif()
        if(${component}_FOUND)
            set(MUMPS_${component}_FOUND TRUE)
            # should we have these variables available in gui modes?
            if (MPI_FOUND)
                mark_as_advanced(MPI_LIBRARY)
                mark_as_advanced(MPI_EXTRA_LIBRARY)
            endif()
        else()
            set(MUMPS_${component}_FOUND FALSE)
        endif()
    endforeach()
endif()

if (NOT MUMPS_FIND_QUIETLY)
    if (MUMPS_LOOK_FOR_SEQ)
        message(STATUS "Looking for MUMPS - sequential version (without MPI)")
    else()
        message(STATUS "Looking for MUMPS - MPI version -"
        " if you want to force detection of a sequential "
        "version use find_package(MUMPS [REQUIRED] COMPONENTS SEQ [...])")
    endif()
endif()

if (NOT MUMPS_FIND_QUIETLY)
    message(STATUS "Looking for MUMPS - PkgConfig not used")
endif()

# Dependencies detection
# ----------------------


# Required dependencies
# ---------------------

if (NOT MUMPS_FIND_QUIETLY)
    message(STATUS "Looking for MUMPS - Try to detect pthread")
endif()
if (MUMPS_FIND_REQUIRED)
    find_package(Threads REQUIRED)
else()
    find_package(Threads)
endif()
set(MUMPS_EXTRA_LIBRARIES "")
if( THREADS_FOUND )
    list(APPEND MUMPS_EXTRA_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif ()

# MUMPS depends on BLAS
#----------------------
if (NOT MUMPS_FIND_QUIETLY)
    message(STATUS "Looking for MUMPS - Try to detect BLAS")
endif()
if (MUMPS_FIND_REQUIRED)
    find_package(BLASEXT REQUIRED)
else()
    find_package(BLASEXT)
endif()

# Optional dependencies
# ---------------------

# MUMPS may depend on MPI
#------------------------
if (NOT MPI_FOUND AND NOT MUMPS_LOOK_FOR_SEQ)
    if (NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for MUMPS - Try to detect MPI")
    endif()
    # allows to use an external mpi compilation by setting compilers with
    # -DMPI_C_COMPILER=path/to/mpicc -DMPI_Fortran_COMPILER=path/to/mpif90
    # at cmake configure
    if(NOT MPI_C_COMPILER)
        set(MPI_C_COMPILER mpicc)
    endif()
    if (MUMPS_FIND_REQUIRED AND MUMPS_LOOK_FOR_MPI)
        find_package(MPI REQUIRED)
    else()
        find_package(MPI)
    endif()
    if (MPI_FOUND)
        mark_as_advanced(MPI_LIBRARY)
        mark_as_advanced(MPI_EXTRA_LIBRARY)
    endif()
endif (NOT MPI_FOUND AND NOT MUMPS_LOOK_FOR_SEQ)

# MUMPS may depend on ScaLAPACK (if MPI version)
#-----------------------------------------------
if (NOT SCALAPACK_FOUND AND NOT MUMPS_LOOK_FOR_SEQ)
    if (NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for MUMPS - Try to detect SCALAPACK")
    endif()
    # SCALAPACK is a required dependency if MPI is used
    if (MUMPS_FIND_REQUIRED AND MUMPS_LOOK_FOR_MPI)
        find_package(SCALAPACK REQUIRED)
    else()
        find_package(SCALAPACK)
    endif()
endif (NOT SCALAPACK_FOUND AND NOT MUMPS_LOOK_FOR_SEQ)

# MUMPS may depends on SCOTCH
#----------------------------
if (NOT SCOTCH_FOUND)
    if (NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for MUMPS - Try to detect SCOTCH")
    endif()
    if (MUMPS_FIND_REQUIRED AND MUMPS_FIND_REQUIRED_SCOTCH)
        find_package(SCOTCH REQUIRED)
    else()
        find_package(SCOTCH)
    endif()
endif()

# MUMPS may depends on METIS
#---------------------------
if (NOT METIS_FOUND)
    if (NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for MUMPS - Try to detect METIS")
    endif()
    if (MUMPS_FIND_REQUIRED AND MUMPS_FIND_REQUIRED_METIS)
        find_package(METIS REQUIRED)
    else()
        find_package(METIS)
    endif()
endif()


# Looking for MUMPS
# -----------------

# Looking for include
# -------------------

# Try to find the mumps header in the given path
# ----------------------------------------------
# call cmake macro to find the header path
if(MUMPS_DIR)
    set(MUMPS_smumps_c.h_DIRS "MUMPS_smumps_c.h_DIRS-NOTFOUND")
    find_path(MUMPS_smumps_c.h_DIRS
      NAMES smumps_c.h
      HINTS ${MUMPS_DIR}
      PATH_SUFFIXES "include")
    set(MUMPS_dmumps_c.h_DIRS "MUMPS_dmumps_c.h_DIRS-NOTFOUND")
    find_path(MUMPS_dmumps_c.h_DIRS
      NAMES dmumps_c.h
      HINTS ${MUMPS_DIR}
      PATH_SUFFIXES "include")
    set(MUMPS_cmumps_c.h_DIRS "MUMPS_cmumps_c.h_DIRS-NOTFOUND")
    find_path(MUMPS_cmumps_c.h_DIRS
      NAMES cmumps_c.h
      HINTS ${MUMPS_DIR}
      PATH_SUFFIXES "include")
    set(MUMPS_zmumps_c.h_DIRS "MUMPS_zmumps_c.h_DIRS-NOTFOUND")
    find_path(MUMPS_zmumps_c.h_DIRS
      NAMES zmumps_c.h
      HINTS ${MUMPS_DIR}
      PATH_SUFFIXES "include")
else()
    if (MUMPS_FIND_REQUIRED)
        message(FATAL_ERROR "Looking for mumps -- MUMPS_DIR is not set, to find"
        " MUMPS please set MUMPS_DIR, the path to MUMPS installation where"
        " sub-directories include/ and lib/ are located")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- MUMPS_DIR is not set, to find"
            " MUMPS please set MUMPS_DIR, the path to MUMPS installation where"
            " sub-directories include/ and lib/ are located")
        endif()
    endif()
endif()

# If found, add path to cmake variable
# ------------------------------------
# detect which precisions are available
if (MUMPS_smumps_c.h_DIRS)
    mark_as_advanced(MUMPS_smumps_c.h_DIRS)
    set(MUMPS_PREC_S ON)
    set(MUMPS_INCLUDE_DIRS "${MUMPS_smumps_c.h_DIRS}")
else ()
    set(MUMPS_PREC_S OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- smumps_c.h not found")
    endif()
endif()
if (MUMPS_dmumps_c.h_DIRS)
    mark_as_advanced(MUMPS_dmumps_c.h_DIRS)
    set(MUMPS_PREC_D ON)
    set(MUMPS_INCLUDE_DIRS "${MUMPS_dmumps_c.h_DIRS}")
else ()
    set(MUMPS_PREC_D OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- dmumps_c.h not found")
    endif()
endif()
if (MUMPS_cmumps_c.h_DIRS)
    mark_as_advanced(MUMPS_cmumps_c.h_DIRS)
    set(MUMPS_PREC_C ON)
    set(MUMPS_INCLUDE_DIRS "${MUMPS_cmumps_c.h_DIRS}")
else ()
    set(MUMPS_PREC_C OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- cmumps_c.h not found")
    endif()
endif()
if (MUMPS_zmumps_c.h_DIRS)
    mark_as_advanced(MUMPS_zmumps_c.h_DIRS)
    set(MUMPS_PREC_Z ON)
    set(MUMPS_INCLUDE_DIRS "${MUMPS_zmumps_c.h_DIRS}")
else ()
    set(MUMPS_PREC_Z OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- zmumps_c.h not found")
    endif()
endif()

# Looking for lib
# ---------------

# Try to find the mumps lib in the given paths
# --------------------------------------------

# call cmake macro to find the lib path
if(MUMPS_DIR)
    set(MUMPS_smumps_LIBRARY "MUMPS_smumps_LIBRARY-NOTFOUND")
    find_library(MUMPS_smumps_LIBRARY
                 NAMES smumps
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
    set(MUMPS_dmumps_LIBRARY "MUMPS_dmumps_LIBRARY-NOTFOUND")
    find_library(MUMPS_dmumps_LIBRARY
                 NAMES dmumps
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
    set(MUMPS_cmumps_LIBRARY "MUMPS_cmumps_LIBRARY-NOTFOUND")
    find_library(MUMPS_cmumps_LIBRARY
                 NAMES cmumps
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
    set(MUMPS_zmumps_LIBRARY "MUMPS_zmumps_LIBRARY-NOTFOUND")
    find_library(MUMPS_zmumps_LIBRARY
                 NAMES zmumps
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
    set(MUMPS_mumps_common_LIBRARY "MUMPS_mumps_common_LIBRARY-NOTFOUND")
    find_library(MUMPS_mumps_common_LIBRARY
                 NAMES mumps_common
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
    set(MUMPS_mpiseq_LIBRARY "MUMPS_mpiseq_LIBRARY-NOTFOUND")
    find_library(MUMPS_mpiseq_LIBRARY
                 NAMES mpiseq
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES libseq)
    set(MUMPS_pord_LIBRARY "MUMPS_pord_LIBRARY-NOTFOUND")
    find_library(MUMPS_pord_LIBRARY
                 NAMES pord
                 HINTS ${MUMPS_DIR}
                 PATH_SUFFIXES lib)
else()
    if (MUMPS_FIND_REQUIRED)
        message(FATAL_ERROR "Looking for mumps -- MUMPS_DIR is not set, to find"
        " MUMPS please set MUMPS_DIR, the path to MUMPS installation where"
        " sub-directories include/ and lib/ are located")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- MUMPS_DIR is not set, to find"
            " MUMPS please set MUMPS_DIR, the path to MUMPS installation where"
            " sub-directories include/ and lib/ are located")
        endif()
    endif()
endif()

# If found, add path to cmake variable
# ------------------------------------
set(MUMPS_LIBRARIES "")
set(MUMPS_LIBRARY_DIRS "")
# detect which precisions are available
if (MUMPS_smumps_LIBRARY)
    mark_as_advanced(MUMPS_smumps_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_smumps_LIBRARY}")
    get_filename_component(smumps_lib_path ${MUMPS_smumps_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${smumps_lib_path}")
else ()
    set(MUMPS_PREC_S OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- libsmumps.a not found")
    endif()
endif()
if (MUMPS_dmumps_LIBRARY)
    mark_as_advanced(MUMPS_dmumps_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_dmumps_LIBRARY}")
    get_filename_component(dmumps_lib_path ${MUMPS_dmumps_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${dmumps_lib_path}")
else ()
    set(MUMPS_PREC_D OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- libdmumps.a not found")
    endif()
endif()
if (MUMPS_cmumps_LIBRARY)
    mark_as_advanced(MUMPS_cmumps_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_cmumps_LIBRARY}")
    get_filename_component(cmumps_lib_path ${MUMPS_cmumps_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${cmumps_lib_path}")
else ()
    set(MUMPS_PREC_C OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- libcmumps.a not found")
    endif()
endif()
if (MUMPS_zmumps_LIBRARY)
    mark_as_advanced(MUMPS_zmumps_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_zmumps_LIBRARY}")
    get_filename_component(zmumps_lib_path ${MUMPS_zmumps_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${zmumps_lib_path}")
else ()
    set(MUMPS_PREC_Z OFF)
    if(NOT MUMPS_FIND_QUIETLY)
        message(STATUS "Looking for mumps -- libzmumps.a not found")
    endif()
endif()
list(REMOVE_DUPLICATES MUMPS_LIBRARY_DIRS)
# check that one precision arithmetic at least has been discovered
if (NOT MUMPS_PREC_S AND NOT MUMPS_PREC_D AND NOT MUMPS_PREC_C AND NOT MUMPS_PREC_S)
    if (MUMPS_FIND_REQUIRED)
        message(FATAL_ERROR "Looking for mumps -- "
        "no lib[sdcz]mumps.a have been found in ${MUMPS_DIR}/lib when required")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- no lib[sdcz]mumps.a have been found")
        endif()
    endif()
endif()
# other MUMPS libraries
if (MUMPS_mumps_common_LIBRARY)
    mark_as_advanced(MUMPS_mumps_common_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_mumps_common_LIBRARY}")
    get_filename_component(mumps_common_lib_path ${MUMPS_mumps_common_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${mumps_common_lib_path}")
else ()
    if (MUMPS_FIND_REQUIRED)
        message(FATAL_ERROR "Looking for mumps -- "
        "libmumps_common.a not found in ${MUMPS_DIR}/lib when required")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- libmumps_common.a not found")
        endif()
    endif()
endif()
if (MUMPS_mpiseq_LIBRARY)
    mark_as_advanced(MUMPS_mpiseq_LIBRARY)
    if (MUMPS_LOOK_FOR_SEQ)
        list(APPEND MUMPS_LIBRARIES "${MUMPS_mpiseq_LIBRARY}")
        get_filename_component(mpiseq_lib_path ${MUMPS_mpiseq_LIBRARY} PATH)
        list(APPEND MUMPS_LIBRARY_DIRS "${mpiseq_lib_path}")
    endif()
else ()
    if (MUMPS_FIND_REQUIRED AND MUMPS_LOOK_FOR_SEQ)
        message(FATAL_ERROR "Looking for mumps -- "
        "libmpiseq.a not found in ${MUMPS_DIR}/libseq when required")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- libmpiseq.a not found")
        endif()
    endif()
endif()
if (MUMPS_pord_LIBRARY)
    mark_as_advanced(MUMPS_pord_LIBRARY)
    list(APPEND MUMPS_LIBRARIES "${MUMPS_pord_LIBRARY}")
    get_filename_component(pord_lib_path ${MUMPS_pord_LIBRARY} PATH)
    list(APPEND MUMPS_LIBRARY_DIRS "${pord_lib_path}")
else ()
    if (MUMPS_FIND_REQUIRED)
        message(FATAL_ERROR "Looking for mumps -- "
        "libpord.a not found in ${MUMPS_DIR}/lib when required")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for mumps -- libpord.a not found")
        endif()
    endif()
endif()


if(MUMPS_LIBRARIES)
    # check a function to validate the find
    if (MUMPS_INCLUDE_DIRS)
        set(CMAKE_REQUIRED_INCLUDES "${MUMPS_INCLUDE_DIRS}")
    endif()
    set(CMAKE_REQUIRED_FLAGS)
    foreach(libdir ${MUMPS_LIBRARY_DIRS})
        if (libdir)
            set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
        endif()
    endforeach()
    set(CMAKE_REQUIRED_LIBRARIES "${MUMPS_LIBRARIES}")
    if (BLAS_FOUND)
        if (BLAS_INCLUDE_DIRS)
            list(APPEND CMAKE_REQUIRED_INCLUDES "${BLAS_INCLUDE_DIRS}")
        endif()
        foreach(libdir ${BLAS_LIBRARY_DIRS})
            if (libdir)
                set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
            endif()
        endforeach()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${BLAS_LIBRARIES}")
    endif()
    if (SCALAPACK_FOUND AND MUMPS_LOOK_FOR_MPI OR MUMPS_FIND_REQUIRED_MPI)
        if (SCALAPACK_INCLUDE_DIRS)
            list(APPEND CMAKE_REQUIRED_INCLUDES "${SCALAPACK_INCLUDE_DIRS}")
        endif()
        foreach(libdir ${SCALAPACK_LIBRARY_DIRS})
            if (libdir)
                set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
            endif()
        endforeach()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${SCALAPACK_LIBRARIES}")
    endif()
    if (MPI_FOUND AND MUMPS_LOOK_FOR_MPI OR MUMPS_FIND_REQUIRED_MPI)
        if (MPI_C_INCLUDE_PATH)
            list(APPEND CMAKE_REQUIRED_INCLUDES "${MPI_C_INCLUDE_PATH}")
        endif()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${MPI_C_LIBRARIES}")
    endif()
    if (MUMPS_FIND_REQUIRED_SCOTCH)
        if (SCOTCH_INCLUDE_DIRS)
            list(APPEND CMAKE_REQUIRED_INCLUDES "${SCOTCH_INCLUDE_DIRS}")
        endif()
        foreach(libdir ${SCOTCH_LIBRARY_DIRS})
            if (libdir)
                set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
            endif()
        endforeach()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${SCOTCH_LIBRARIES}")
    endif()
    if (MUMPS_FIND_REQUIRED_METIS)
        if (METIS_INCLUDE_DIRS)
            list(APPEND CMAKE_REQUIRED_INCLUDES "${METIS_INCLUDE_DIRS}")
        endif()
        foreach(libdir ${METIS_LIBRARY_DIRS})
            if (libdir)
                set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
            endif()
        endforeach()
        list(APPEND CMAKE_REQUIRED_LIBRARIES "${METIS_LIBRARIES}")
    endif()
    if (CMAKE_Fortran_COMPILER MATCHES ".+gfortran.*")
        list(APPEND CMAKE_REQUIRED_LIBRARIES "-lgfortran")
    elseif (CMAKE_Fortran_COMPILER MATCHES ".+ifort.*")
        list(APPEND CMAKE_REQUIRED_LIBRARIES "-lifcore")
    endif()
    list(APPEND CMAKE_REQUIRED_LIBRARIES ${MUMPS_EXTRA_LIBRARIES})

    include(CheckFortranFunctionExists)
    unset(MUMPS_PREC_S_WORKS CACHE)
    check_fortran_function_exists(smumps MUMPS_PREC_S_WORKS)
    mark_as_advanced(MUMPS_PREC_S_WORKS)
    unset(MUMPS_PREC_D_WORKS CACHE)
    check_fortran_function_exists(dmumps MUMPS_PREC_D_WORKS)
    mark_as_advanced(MUMPS_PREC_D_WORKS)
    unset(MUMPS_PREC_C_WORKS CACHE)
    check_fortran_function_exists(cmumps MUMPS_PREC_C_WORKS)
    mark_as_advanced(MUMPS_PREC_C_WORKS)
    unset(MUMPS_PREC_Z_WORKS CACHE)
    check_fortran_function_exists(zmumps MUMPS_PREC_Z_WORKS)
    mark_as_advanced(MUMPS_PREC_Z_WORKS)

    set(MUMPS_WORKS FALSE)
    if(MUMPS_PREC_S_WORKS OR MUMPS_PREC_D_WORKS OR MUMPS_PREC_C_WORKS OR MUMPS_PREC_Z_WORKS)
        set(MUMPS_WORKS TRUE)
    endif()

    if (NOT MUMPS_WORKS)
        if (SCOTCH_FOUND AND NOT MUMPS_FIND_REQUIRED_SCOTCH)
            if (SCOTCH_INCLUDE_DIRS)
                list(APPEND CMAKE_REQUIRED_INCLUDES "${SCOTCH_INCLUDE_DIRS}")
            endif()
            foreach(libdir ${SCOTCH_LIBRARY_DIRS})
                if (libdir)
                    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
                endif()
            endforeach()
            list(APPEND CMAKE_REQUIRED_LIBRARIES "${SCOTCH_LIBRARIES}")
        endif()
        check_fortran_function_exists(smumps MUMPS_PREC_S_WORKS)
        check_fortran_function_exists(dmumps MUMPS_PREC_D_WORKS)
        check_fortran_function_exists(cmumps MUMPS_PREC_C_WORKS)
        check_fortran_function_exists(zmumps MUMPS_PREC_Z_WORKS)
    endif()

    set(MUMPS_WORKS FALSE)
    if(MUMPS_PREC_S_WORKS OR MUMPS_PREC_D_WORKS OR MUMPS_PREC_C_WORKS OR MUMPS_PREC_Z_WORKS)
        set(MUMPS_WORKS TRUE)
    endif()

    if (NOT MUMPS_WORKS)
        if (METIS_FOUND AND NOT MUMPS_FIND_REQUIRED_METIS)
            if (METIS_INCLUDE_DIRS)
                list(APPEND CMAKE_REQUIRED_INCLUDES "${METIS_INCLUDE_DIRS}")
            endif()
            foreach(libdir ${METIS_LIBRARY_DIRS})
                if (libdir)
                    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L${libdir}")
                endif()
            endforeach()
            list(APPEND CMAKE_REQUIRED_LIBRARIES "${METIS_LIBRARIES}")
        endif()
        check_fortran_function_exists(smumps MUMPS_PREC_S_WORKS)
        check_fortran_function_exists(dmumps MUMPS_PREC_D_WORKS)
        check_fortran_function_exists(cmumps MUMPS_PREC_C_WORKS)
        check_fortran_function_exists(zmumps MUMPS_PREC_Z_WORKS)
    endif()

    set(MUMPS_WORKS FALSE)
    if(MUMPS_PREC_S_WORKS OR MUMPS_PREC_D_WORKS OR MUMPS_PREC_C_WORKS OR MUMPS_PREC_Z_WORKS)
        set(MUMPS_WORKS TRUE)
    endif()

    if(MUMPS_WORKS)
        string(REPLACE " -L" ";" CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        set(MUMPS_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
        set(MUMPS_LIBRARY_DIRS "${CMAKE_REQUIRED_FLAGS}")
        set(MUMPS_INCLUDE_DIRS "${CMAKE_REQUIRED_INCLUDES}")
    else()
        if(NOT MUMPS_FIND_QUIETLY)
            message(STATUS "Looking for MUMPS : test of [sdcz]mumps() fails")
            message(STATUS "MUMPS_LIBRARIES: ${CMAKE_REQUIRED_LIBRARIES}")
            message(STATUS "MUMPS_LIBRARY_DIRS: ${CMAKE_REQUIRED_FLAGS}")
            message(STATUS "MUMPS_INCLUDE_DIRS: ${CMAKE_REQUIRED_INCLUDES}")
            message(STATUS "Check in CMakeFiles/CMakeError.log to figure out why it fails")
            message(STATUS "Looking for mumps : set MUMPS_LIBRARIES to NOTFOUND")
        endif()
        set(MUMPS_LIBRARIES "MUMPS_LIBRARIES-NOTFOUND")
    endif()
    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_FLAGS)
    set(CMAKE_REQUIRED_LIBRARIES)
endif(MUMPS_LIBRARIES)


# check that MUMPS has been found
# ---------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MUMPS DEFAULT_MSG
                                  MUMPS_LIBRARIES)