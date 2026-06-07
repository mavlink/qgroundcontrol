# Copyright (C) 2022 The Qt Company Ltd.
# Copyright (C) 2023 Intel Corpotation.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapResolv::WrapResolv)
    set(WrapResolv_FOUND ON)
    return()
endif()

set(WrapResolv_FOUND OFF)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

if(QNX)
    find_library(LIBRESOLV socket)
else()
    find_library(LIBRESOLV resolv)
endif()

cmake_push_check_state()
if(LIBRESOLV)
    list(APPEND CMAKE_REQUIRED_LIBRARIES "${LIBRESOLV}")
endif()

cmake_policy(PUSH)
if(POLICY CMP0067)
    cmake_policy(SET CMP0067 NEW)
endif()
if(DEFINED CMAKE_CXX_STANDARD)
    set(_WrapResolv_CMAKE_CXX_STANDARD_back ${CMAKE_CXX_STANDARD})
endif()
set(CMAKE_CXX_STANDARD 11)

check_cxx_source_compiles("
#include <netinet/in.h>
#include <resolv.h>

int main(int, char **argv)
{
    res_state statep = {};
    int n = res_nmkquery(statep, 0, argv[1], 0, 0, NULL, 0, NULL, NULL, 0);
    n = res_nsend(statep, NULL, 0, NULL, 0);
    n = dn_expand(NULL, NULL, NULL, NULL, 0);
    return n;
}
" HAVE_LIBRESOLV_FUNCTIONS)

if(DEFINED _WrapResolv_CMAKE_CXX_STANDARD_back)
    set(CMAKE_CXX_STANDARD ${_WrapResolv_CMAKE_CXX_STANDARD_back})
    unset(_WrapResolv_CMAKE_CXX_STANDARD_back)
else()
    unset(CMAKE_CXX_STANDARD)
endif()

cmake_policy(POP)
cmake_pop_check_state()

if(HAVE_LIBRESOLV_FUNCTIONS)
    set(WrapResolv_FOUND ON)
    add_library(WrapResolv::WrapResolv INTERFACE IMPORTED)
    if(LIBRESOLV)
        target_link_libraries(WrapResolv::WrapResolv INTERFACE "${LIBRESOLV}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapResolv DEFAULT_MSG WrapResolv_FOUND)
