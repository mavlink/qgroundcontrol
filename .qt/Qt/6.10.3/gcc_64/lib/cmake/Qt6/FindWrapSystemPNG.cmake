# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemPNG::WrapSystemPNG)
    set(WrapSystemPNG_FOUND TRUE)
    return()
endif()
set(WrapSystemPNG_REQUIRED_VARS __png_found)

find_package(PNG ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

set(__png_target_name "PNG::PNG")
if(PNG_FOUND AND TARGET "${__png_target_name}")
    set(__png_found TRUE)
    if(PNG_VERSION)
        set(WrapSystemPNG_VERSION "${PNG_VERSION}")
    endif()
endif()

if(PNG_LIBRARIES)
    list(PREPEND WrapSystemPNG_REQUIRED_VARS PNG_LIBRARIES)
endif()
if(PNG_VERSION)
    set(WrapSystemPNG_VERSION "${PNG_VERSION}")
elseif(PNG_VERSION_STRING)
    set(WrapSystemPNG_VERSION "${PNG_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemPNG
                                  REQUIRED_VARS ${WrapSystemPNG_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemPNG_VERSION)

if(WrapSystemPNG_FOUND)
    add_library(WrapSystemPNG::WrapSystemPNG INTERFACE IMPORTED)
    target_link_libraries(WrapSystemPNG::WrapSystemPNG
                          INTERFACE "${__png_target_name}")
endif()
unset(__png_target_name)
unset(__png_found)
