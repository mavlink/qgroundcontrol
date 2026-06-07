# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The test represents the order-related issue that we have with the ld linker.
#
# CMake versions < 3.21.0 produce the following linker line:
# <binary_name> main.cpp -o static_link_order_test libstaticLib.a objlib.cpp.o
# Since 'static_link_order_test' doesn't have direct use of 'staticlib2.cpp.o' symbols
# the translation unit is not linked. When we link objlib.cpp.o it cannot resolve symbols from
# staticlib2.cpp.o.
#
# For now it's only applicable for ld-like linkers. 'lld' has no such issue.
cmake_minimum_required(VERSION 3.16)

project(static_link_order_test LANGUAGES CXX)

add_library(objLib OBJECT objlib.cpp)
add_library(staticLib STATIC staticlib1.cpp staticlib2.cpp)

target_link_libraries(staticLib
    INTERFACE objLib "$<TARGET_OBJECTS:objLib>"
)

add_executable(static_link_order_test main.cpp)
target_link_libraries(static_link_order_test PRIVATE staticLib)
