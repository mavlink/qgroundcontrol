# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemFreetype::WrapSystemFreetype)
    set(WrapSystemFreetype_FOUND TRUE)
    return()
endif()
set(WrapSystemFreetype_REQUIRED_VARS __freetype_found)

# Hunter has the package named freetype, but exports the Freetype::Freetype target as upstream
# First try the CONFIG package, and afterwards the MODULE if not found
find_package(Freetype ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}
             CONFIG NAMES Freetype freetype QUIET)
if(NOT Freetype_FOUND)
    find_package(Freetype ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET MODULE)
endif()

if(Freetype_FOUND)
    # vcpkg defines a lower case target name, while upstream Find module defines a prefixed
    # upper case name.
    set(__freetype_potential_target_names Freetype::Freetype freetype)
    foreach(__freetype_potential_target_name ${__freetype_potential_target_names})
        if(TARGET "${__freetype_potential_target_name}")
            set(__freetype_target_name "${__freetype_potential_target_name}")
            set(__freetype_found TRUE)
            break()
        endif()
    endforeach()
endif()

if(FREETYPE_LIBRARIES)
    list(PREPEND WrapSystemFreetype_REQUIRED_VARS FREETYPE_LIBRARIES)
endif()
if(Freetype_VERSION)
    set(WrapSystemFreetype_VERSION "${Freetype_VERSION}")
elseif(FREETYPE_VERSION_STRING)
    set(WrapSystemFreetype_VERSION "${FREETYPE_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemFreetype
                                  REQUIRED_VARS ${WrapSystemFreetype_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemFreetype_VERSION)

if(WrapSystemFreetype_FOUND)
    add_library(WrapSystemFreetype::WrapSystemFreetype INTERFACE IMPORTED)
    target_link_libraries(WrapSystemFreetype::WrapSystemFreetype
                          INTERFACE "${__freetype_target_name}")
endif()
unset(__freetype_target_name)
unset(__freetype_found)
