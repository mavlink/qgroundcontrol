# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(proj LANGUAGES CXX)
add_library(lib1 SHARED lib1.cpp)

add_custom_target(print_lib_path ALL
    COMMAND ${CMAKE_COMMAND} -E echo "###$<TARGET_FILE:lib1>###"
    VERBATIM
)
add_dependencies(print_lib_path lib1)
