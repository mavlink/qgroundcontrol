# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# These values should be kept in sync with those in qtbase/.cmake.conf
cmake_minimum_required(VERSION 3.16...3.21)

set(QT_BACKUP_CMAKE_INSTALL_PREFIX_BEFORE_EXTRA_INCLUDE "${CMAKE_INSTALL_PREFIX}")

# This depends on qt_internal_read_repo_dependencies existing.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake")
    include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsExtra.cmake)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/QtBuildInternalsHelpers.cmake)

macro(qt_set_up_build_internals_paths)
    # Set up the paths for the cmake modules located in the prefix dir. Prepend, so the paths are
    # least important compared to the source dir ones, but more important than command line
    # provided ones.
    set(QT_CMAKE_MODULE_PATH "${QT_BUILD_INTERNALS_PATH}/../${QT_CMAKE_EXPORT_NAMESPACE}")
    list(PREPEND CMAKE_MODULE_PATH "${QT_CMAKE_MODULE_PATH}")

    # Prepend the qtbase source cmake directory to CMAKE_MODULE_PATH,
    # so that if a change is done in cmake/QtBuild.cmake, it gets automatically picked up when
    # building qtdeclarative, rather than having to build qtbase first (which will copy
    # QtBuild.cmake to the build dir). This is similar to qmake non-prefix builds, where the
    # source qtbase/mkspecs directory is used.
    # TODO: Clean this up, together with qt_internal_try_compile_binary_for_strip to only use the
    # the qtbase sources when building qtbase. And perhaps also when doing a non-prefix
    # developer-build.
    if(EXISTS "${QT_SOURCE_TREE}/cmake")
        list(PREPEND CMAKE_MODULE_PATH "${QT_SOURCE_TREE}/cmake")
    endif()

    # If the repo has its own cmake modules, include those in the module path.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    endif()

    # Find the cmake files when doing a standalone tests build.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
        list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    endif()
endmacro()

qt_internal_setup_cmake_and_export_namespace()

# Set up the build internal paths unless explicitly requested not to.
if(NOT QT_BUILD_INTERNALS_SKIP_CMAKE_MODULE_PATH_ADDITION)
    # Depends on qt_internal_setup_cmake_and_export_namespace
    qt_set_up_build_internals_paths()
endif()

include(QtBuildHelpers)

qt_internal_include_all_helpers()
qt_internal_setup_build_internals()
