# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemHarfbuzz::WrapSystemHarfbuzz)
    set(WrapSystemHarfbuzz_FOUND TRUE)
    return()
endif()
set(WrapSystemHarfbuzz_REQUIRED_VARS __harfbuzz_found)

find_package(harfbuzz ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

# Gentoo has some buggy version of a harfbuzz Config file. Check if include paths are valid.
set(__harfbuzz_target_name "harfbuzz::harfbuzz")
if(harfbuzz_FOUND AND TARGET "${__harfbuzz_target_name}")
    get_property(__harfbuzz_include_paths TARGET "${__harfbuzz_target_name}"
                                          PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
    foreach(__harfbuzz_include_dir ${__harfbuzz_include_paths})
        if(NOT EXISTS "${__harfbuzz_include_dir}")
            # Must be the broken Gentoo harfbuzzConfig.cmake file. Try to use pkg-config instead.
            set(__harfbuzz_broken_config_file TRUE)
            break()
        endif()
    endforeach()

    set(__harfbuzz_found TRUE)
    if(harfbuzz_VERSION)
        set(WrapSystemHarfbuzz_VERSION "${harfbuzz_VERSION}")
    endif()
else()
    get_cmake_property(__packages_not_found PACKAGES_NOT_FOUND)
    if(__packages_not_found)
        list(REMOVE_ITEM __packages_not_found harfbuzz)
        set_property(GLOBAL PROPERTY PACKAGES_NOT_FOUND "${__packages_not_found}")
    endif()
    unset(__packages_not_found)
endif()

if(__harfbuzz_broken_config_file OR NOT __harfbuzz_found)
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_HARFBUZZ IMPORTED_TARGET "harfbuzz")
    if(PC_HARFBUZZ_FOUND)
        set(__harfbuzz_target_name "PkgConfig::PC_HARFBUZZ")
        set(__harfbuzz_find_include_dirs_hints
            HINTS ${PC_HARFBUZZ_INCLUDEDIR})
        set(__harfbuzz_find_library_hints
            HINTS ${PC_HARFBUZZ_LIBDIR})
        if(PC_HARFBUZZ_VERSION)
            set(WrapSystemHarfbuzz_VERSION "${PC_HARFBUZZ_VERSION}")
        endif()
    else()
        set(__harfbuzz_target_name "Harfbuzz::Harfbuzz")
    endif()

    find_path(HARFBUZZ_INCLUDE_DIRS
        NAMES harfbuzz/hb.h
        ${__harfbuzz_find_include_dirs_hints})
    find_library(HARFBUZZ_LIBRARIES
        NAMES harfbuzz
        ${__harfbuzz_find_library_hints})

    if(HARFBUZZ_INCLUDE_DIRS AND HARFBUZZ_LIBRARIES)
        set(__harfbuzz_found TRUE)
        if(NOT PC_HARFBUZZ_FOUND)
            add_library(${__harfbuzz_target_name} UNKNOWN IMPORTED)
            list(TRANSFORM HARFBUZZ_INCLUDE_DIRS APPEND "/harfbuzz")
            set_target_properties(${__harfbuzz_target_name} PROPERTIES
                IMPORTED_LOCATION "${HARFBUZZ_LIBRARIES}"
                INTERFACE_INCLUDE_DIRECTORIES "${HARFBUZZ_INCLUDE_DIRS}"
            )
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemHarfbuzz
                                  REQUIRED_VARS ${WrapSystemHarfbuzz_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemHarfbuzz_VERSION)
if(WrapSystemHarfbuzz_FOUND)
    add_library(WrapSystemHarfbuzz::WrapSystemHarfbuzz INTERFACE IMPORTED)
    target_link_libraries(WrapSystemHarfbuzz::WrapSystemHarfbuzz
                          INTERFACE "${__harfbuzz_target_name}")
endif()
unset(__harfbuzz_target_name)
unset(__harfbuzz_find_include_dirs_hints)
unset(__harfbuzz_find_library_hints)
unset(__harfbuzz_found)
unset(__harfbuzz_include_dir)
unset(__harfbuzz_broken_config_file)
