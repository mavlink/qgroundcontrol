# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# FindMySQL
# ---------
#
# Try to locate the mysql client library.
#
# By default, pkg-config is used, if available.
# If pkg-config is not available or if you want to disable it, set the ``MySQL_ROOT`` variable.
# The following variables can be set to control the behavior of this find module.
#
# ``MySQL_ROOT``
#     The root directory of the mysql client library's installation.
# ``MySQL_INCLUDE_DIR``
#     The directory containing the include files of the mysql client library.
#     If not set, the directory is detected within ``MySQL_ROOT`` using find_path.
# ``MySQL_LIBRARY_DIR``
#     The directory containing the binaries of the mysql client library.
#     This is used to detect ``MySQL_LIBRARY`` and passed as HINT to find_library.
# ``MySQL_LIBRARY``
#     The file path to the mysql client library.
# ``MySQL_LIBRARY_DEBUG``
#     The file path to the mysql client library for the DEBUG configuration.
#
# If the mysql client library is found, this will define the following variables:
#
# ``MySQL_FOUND``
#     True if the mysql library is available
# ``MySQL_INCLUDE_DIRS``
#     The mysql include directories
# ``MySQL_LIBRARIES``
#     The mysql libraries for linking
#
# If ``MySQL_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``MySQL::MySQL``
#     The mysql client library

if(NOT DEFINED MySQL_ROOT)
    if(DEFINED ENV{MySQL_ROOT})
        set(MySQL_ROOT "$ENV{MySQL_ROOT}")
    else()
        find_package(PkgConfig QUIET)
    endif()
endif()
if(PkgConfig_FOUND AND NOT DEFINED MySQL_ROOT)
    pkg_check_modules(PC_MySQL QUIET "mysqlclient")
    set(MySQL_include_dir_hints ${PC_MySQL_INCLUDEDIR})
    set(MySQL_library_hints ${PC_MySQL_LIBDIR})
    set(MySQL_library_hints_debug "")
else()
    set(MySQL_include_dir_hints "${MySQL_ROOT}" "${MySQL_ROOT}/include")
    if(NOT DEFINED MySQL_LIBRARY_DIR)
        set(MySQL_LIBRARY_DIR "${MySQL_ROOT}/lib")
    endif()
    set(MySQL_library_hints "${MySQL_LIBRARY_DIR}")
    set(MySQL_library_hints_debug "${MySQL_LIBRARY_DIR}/debug")
endif()

find_path(MySQL_INCLUDE_DIR
          NAMES mysql.h
          HINTS "${MySQL_include_dir_hints}"
          PATH_SUFFIXES mysql mariadb)

find_library(MySQL_LIBRARY
             NO_PACKAGE_ROOT_PATH
             NAMES libmysql mysql mysqlclient libmariadb mariadb
             HINTS ${MySQL_library_hints})

if(MySQL_library_hints_debug)
    find_library(MySQL_LIBRARY_DEBUG
                 NO_PACKAGE_ROOT_PATH
                 NAMES libmysql mysql mysqlclient libmariadb mariadb
                 HINTS ${MySQL_library_hints_debug})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MySQL_LIBRARY MySQL_INCLUDE_DIR)

if(MySQL_FOUND)
  set(MySQL_INCLUDE_DIRS "${MySQL_INCLUDE_DIR}")
  set(MySQL_LIBRARIES "${MySQL_LIBRARY}")
  if(NOT TARGET MySQL::MySQL)
    add_library(MySQL::MySQL UNKNOWN IMPORTED)
    set_target_properties(MySQL::MySQL PROPERTIES
                          IMPORTED_LOCATION "${MySQL_LIBRARIES}"
                          INTERFACE_INCLUDE_DIRECTORIES "${MySQL_INCLUDE_DIRS}")
    if(MySQL_LIBRARY_DEBUG)
      set_target_properties(MySQL::MySQL PROPERTIES
                            IMPORTED_LOCATION_DEBUG "${MySQL_LIBRARY_DEBUG}")
    endif()
  endif()
endif()

mark_as_advanced(MySQL_INCLUDE_DIR MySQL_LIBRARY)

include(FeatureSummary)
set_package_properties(MySQL PROPERTIES
  URL "https://www.mysql.com"
  DESCRIPTION "MySQL client library")

