###############################################################################
# Copyright (c) 2014, Lawrence Livermore National Security, LLC.
#
# Produced at the Lawrence Livermore National Laboratory
#
# LLNL-CODE-666778
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the disclaimer below.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the disclaimer (as noted below) in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the LLNS/LLNL nor the names of its contributors may
#   be used to endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
# LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
###############################################################################

################################
# Setup build options and their default values
################################
include(cmake/AxomOptions.cmake)

################################
# AXOM's Third party library setup
################################
include(cmake/thirdparty/SetupAxomThirdParty.cmake)

#
# We don't try to use this approach for CMake generators that support
# multiple configurations. See: CZ JIRA: ATK-45
#
if(NOT CMAKE_CONFIGURATION_TYPES)
    ######################################################
    # Add define we can use when debug builds are enabled
    ######################################################
    if( (CMAKE_BUILD_TYPE MATCHES Debug)
        OR (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo )
      )
        add_definitions(-DAXOM_DEBUG)
    endif()
endif()

################################
# Fortran Configuration
################################
if(ENABLE_FORTRAN)
    # Create macros for Fortran name mangling
    include(FortranCInterface)
    FortranCInterface_VERIFY()
    
    if (ENABLE_MPI)
        # Determine if we should use fortran mpif.h header or fortran mpi module
        find_path(mpif_path
            NAMES "mpif.h"
            PATHS ${MPI_Fortran_INCLUDE_PATH}
            NO_DEFAULT_PATH
            )
        
        if(mpif_path)
            set(MPI_Fortran_USE_MPIF ON CACHE PATH "")
            message(STATUS "Using MPI Fortran header: mpif.h")
        else()
            set(MPI_Fortran_USE_MPIF OFF CACHE PATH "")
            message(STATUS "Using MPI Fortran module: mpi.mod")
        endif()
    endif()
endif()


##############################################################################
# Setup some additional compiler options that can be useful in various targets
# These are stored in their own variables.
# Usage: To add one of these sets of flags to some source files:
#   get_source_file_property(_origflags <src_file> COMPILE_FLAGS)
#   set_source_files_properties(<list_of_src_files> 
#        PROPERTIES COMPILE_FLAGS "${_origFlags} ${<flags_variable}" )
##############################################################################

set(custom_compiler_flags_list) # Tracks custom compiler flags for logging

# Flag for disabling warnings about omp pragmas in the code
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_OMP_PRAGMA_WARNINGS
                  DEFAULT "-Wno-unknown-pragmas"
                  XL      "-qignprag=omp"
                  INTEL   "-diag-disable 3180"
                  )
list(APPEND custom_compiler_flags_list AXOM_DISABLE_OMP_PRAGMA_WARNINGS)

# Flag for disabling warnings about unused parameters.
# Useful when we include external code.
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_UNUSED_PARAMETER_WARNINGS
                  DEFAULT "-Wno-unused-parameter"
                  XL      "-qinfo=nopar"
                  )
list(APPEND custom_compiler_flags_list AXOM_DISABLE_UNUSED_PARAMETER_WARNINGS)

# Flag for disabling warnings about unused variables
# Useful when we include external code.
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_UNUSED_VARIABLE_WARNINGS
                  DEFAULT "-Wno-unused-variable"
                  XL      "-qinfo=nouse"
                  )
list(APPEND custom_compiler_flags_list AXOM_DISABLE_UNUSED_VARIABLE_WARNINGS)

# Flag for disabling warnings about variables that may be uninitialized.
# Useful when we are using compiler generated interface code (e.g. in shroud)
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_UNINITIALIZED_WARNINGS
                  DEFAULT "-Wno-uninitialized"
                  XL      "-qsuppress=1540-1102"
                  )
list(APPEND custom_compiler_flags_list AXOM_DISABLE_UNINITIALIZED_WARNINGS)

# Flag for disabling warnings about strict aliasing.
# Useful when we are using compiler generated interface code (e.g. in shroud)
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_ALIASING_WARNINGS
                  DEFAULT "-Wno-strict-aliasing"
                  XL      " "
                  )
list(APPEND custom_compiler_flags_list AXOM_DISABLE_ALIASING_WARNINGS)
                  
# Flag for disabling warnings about unused local typedefs.
# Note: Clang 3.5 and below are not aware of this warning, but later versions are
if(COMPILER_FAMILY_IS_CLANG AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 3.5))
  set(clang_unused_local_typedef "-Wno-unused-local-typedefs")
endif()

blt_append_custom_compiler_flag(FLAGS_VAR AXOM_DISABLE_UNUSED_LOCAL_TYPEDEF
                  DEFAULT " "
                  CLANG   "${clang_unused_local_typedef}"
                  GNU     "-Wno-unused-local-typedefs"
                  )                  
list(APPEND custom_compiler_flags_list AXOM_DISABLE_UNUSED_LOCAL_TYPEDEF)

# Linker flag for allowing multiple definitions of a symbol
blt_append_custom_compiler_flag(FLAGS_VAR AXOM_ALLOW_MULTIPLE_DEFINITIONS
                  DEFAULT " "
                  CLANG   "-Wl,--allow-multiple-definition"
                  GNU     "-Wl,--allow-multiple-definition"
                  )                  
list(APPEND custom_compiler_flags_list AXOM_ALLOW_MULTIPLE_DEFINITIONS)


   
# message(STATUS "Custom compiler flags:")
# foreach(flag ${custom_compiler_flags_list})
#    message(STATUS "\tvalue of ${flag} is '${${flag}}'")
# endforeach()
