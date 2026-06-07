# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

# Get the absolute path of the passed-in project dir, relative to the current working directory
# of the calling script, rather than relative to this source directory.
# The calling script sets PWD. If not set, just use the passed-in value as-is.
if(DEFINED PWD)
    get_filename_component(absolute_project_path "${QT_STANDALONE_TEST_PATH}" ABSOLUTE
                           BASE_DIR "${PWD}")
else()
    set(absolute_project_path "${QT_STANDALONE_TEST_PATH}")
endif()

# If path does not include the drive letter, we try to add it.
get_filename_component(absolute_project_path "." REALPATH BASE_DIR "${absolute_project_path}")

if(NOT IS_DIRECTORY "${absolute_project_path}")
    get_filename_component(filename "${absolute_project_path}" NAME)
    get_filename_component(directory "${absolute_project_path}" DIRECTORY)

    if(filename STREQUAL "CMakeLists.txt")
        set(absolute_project_path "${directory}")
    endif()
endif()

# Get the project name base on test directory name
get_filename_component(project_name "${absolute_project_path}" NAME)

project(${project_name} VERSION 6.0.0 LANGUAGES C CXX)

find_package(Qt6 REQUIRED COMPONENTS BuildInternals Core)

include(${CMAKE_CURRENT_LIST_DIR}/Main.cmake NO_POLICY_SCOPE)

# Add the test project path as a subdirectory project.
add_subdirectory("${absolute_project_path}" "build_dir")
