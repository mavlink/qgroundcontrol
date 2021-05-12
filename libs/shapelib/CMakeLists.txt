# Top-level CMakeLists.txt for the CMake-based build and test system
# of the shapelib software.

# Copyright (C) 2012-2013 Alan W. Irwin

# See README.CMake

# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# This file is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this file; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

# It is a fatal error if no working C compiler is available to build
# the shapelib library and utilities
project(shapelib C)

message(STATUS "CMake version = ${CMAKE_VERSION}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")

# Version 2.8.5 or above of cmake is currently required for all platforms.
cmake_minimum_required(VERSION 2.8.5 FATAL_ERROR)

# libraries are all shared by default.
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

cmake_policy(SET CMP0026 OLD) # Disallow use of the LOCATION property for build targets

if(NOT MSVC)
  add_compile_options(
  	-Wno-missing-braces
  	-Wno-sign-compare
  	-Wno-return-type
  )
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use rpath?
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # No rpath on Darwin. Setting it will only cause trouble.
else(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  option(USE_RPATH "Use -rpath when linking libraries, executables" ON)
endif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")

# In windows all created dlls are gathered in the dll directory
# if you add this directory to your PATH all shared libraries are available
if(BUILD_SHARED_LIBS AND WIN32 AND NOT CYGWIN)
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/dll)
endif(BUILD_SHARED_LIBS AND WIN32 AND NOT CYGWIN)

set(PACKAGE shp)

# Set up install locations.
set(
  CMAKE_INSTALL_BINDIR
  ${CMAKE_INSTALL_PREFIX}/bin
  CACHE PATH "install location for user executables"
  )

set(
  CMAKE_INSTALL_LIBDIR
  ${CMAKE_INSTALL_PREFIX}/lib
  CACHE PATH "install location for object code libraries"
  )

set(
  CMAKE_INSTALL_INCLUDEDIR
  ${CMAKE_INSTALL_PREFIX}/include
  CACHE PATH "install location for C header files"
  )

set(
  CMAKE_INSTALL_SHP_DATADIR
  ${CMAKE_INSTALL_PREFIX}/share/${PACKAGE}
  CACHE PATH "install location for read-only architecture-independent shp data"
  )

# Export build information to help other projects link installed
# shapelib software.  Only one of these signatures is required
# for the export_shp name.
install(EXPORT export_shp DESTINATION ${CMAKE_INSTALL_SHP_DATADIR})

# Initial boilerplate done, now build library and executables.

set(lib_SRC
  shpopen.c
  dbfopen.c
  safileio.c
  shptree.c
  sbnsearch.c
  )
option(SHP_DROP_UNABLE_TO_OPEN_MSG "Drop \"unable to open\" error messages" ON)
if(SHP_DROP_UNABLE_TO_OPEN_MSG)
  #define the SHP_DROP_UNABLE_TO_OPEN_MSG C macro for this source file.
  set_source_files_properties(shpopen.c
    PROPERTIES
    COMPILE_DEFINITIONS SHP_DROP_UNABLE_TO_OPEN_MSG
    )
endif(SHP_DROP_UNABLE_TO_OPEN_MSG)

add_library(shp ${lib_SRC})

if(WIN32 AND NOT CYGWIN)
  set_target_properties(shp
    PROPERTIES
    COMPILE_DEFINITIONS SHAPELIB_DLLEXPORT
    )
endif(WIN32 AND NOT CYGWIN)

if(UNIX)
    find_library(M_LIB m)
    if(M_LIB)
      TARGET_LINK_LIBRARIES(shp -lm)
    endif()
endif(UNIX)

set(shp_SOVERSION 1)
set(shp_VERSION 1.4.1)
set_target_properties(shp
  PROPERTIES 
  SOVERSION ${shp_SOVERSION}
  VERSION ${shp_VERSION}
  INSTALL_NAME_DIR "${CMAKE_INSTALL_LIBDIR}"
  )

if(USE_RPATH)
  set_target_properties(shp
    PROPERTIES 
    INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}"
    )
endif(USE_RPATH)

install(TARGETS shp
  EXPORT export_shp
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

# executables to be built and installed.
set(executables
  shpcreate
  shpadd
  shpdump
  shprewind 
  dbfcreate
  dbfadd
  dbfdump
  shptreedump
  )

find_program(BASH_EXECUTABLE bash)
find_program(SED_EXECUTABLE sed)
if(BASH_EXECUTABLE AND SED_EXECUTABLE)
  #set(BUILD_TEST ON)
else(BASH_EXECUTABLE AND SED_EXECUTABLE)
  message(STATUS "WARNING: sed or bash not available so disabling testing")
endif(BASH_EXECUTABLE AND SED_EXECUTABLE)

# For the first series of tests, the user needs to have downloaded
# from http://dl.maptools.org/dl/shapelib/shape_eg_data.zip, unpacked
# that file, and specified the location of that directory with
# the cmake option, -DEG_DATA:PATH=whatever
if(BUILD_TEST)
  if(EG_DATA)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/script.sed "s?/u/www/projects/shapelib/eg_data?${EG_DATA}?\n")
  else(EG_DATA)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/script.sed "")
    message(STATUS "WARNING: EG_DATA:PATH not set to point to downloaded eg_data directory so the eg_data part of testing will be ignored.")
  endif(EG_DATA)
endif(BUILD_TEST)

foreach(executable ${executables})
  add_executable(${executable} EXCLUDE_FROM_ALL ${executable}.c)
  target_link_libraries(${executable} shp)
  if(USE_RPATH)
    set_target_properties(${executable}
      PROPERTIES
      INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}"
      )
  endif(USE_RPATH)
  if(BUILD_TEST)
    get_target_property(${executable}_LOC ${executable} LOCATION)
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/script.sed "s?\\./${executable}?${${executable}_LOC}?\n")
  endif(BUILD_TEST)
endforeach(executable ${executables})

# Install header
install(FILES shapefil.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BUILD_TEST)
  # Set up tests:

  enable_testing()

  # Other executables to be built to facilitate tests.
  foreach(executable shptest shputils)
    add_executable(${executable} EXCLUDE_FROM_ALL ${executable}.c)
    target_link_libraries(${executable} shp)
    get_target_property(${executable}_LOC ${executable} LOCATION)
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/script.sed "s?\\./${executable}?${${executable}_LOC}?\n")
  endforeach(executable shptest shputils)

  # Write this as a shell script since execute_process cannot handle
  # anything like redirection.
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/script.sh "${SED_EXECUTABLE} -f script.sed < $1 >| $2")
  execute_process(
    COMMAND
    ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/script.sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test1.sh ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test1.sh
    )

  execute_process(
    COMMAND
    ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/script.sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test2.sh ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test2.sh
    )

  execute_process(
    COMMAND
    ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/script.sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/test3.sh ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test3.sh
    )

  if(EG_DATA)
    # These tests correspond to everything in test1.sh
    add_test(
      NAME test1
      COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test1.sh
      )
  endif(EG_DATA)
  # These tests correspond to everything in test2.sh
  add_test(
    NAME test2
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test2.sh
    )

  # These tests correspond to everything in test3.sh
  add_test(
    NAME test3
    COMMAND ${BASH_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/sed_scripted_test3.sh
    )
endif(BUILD_TEST)
