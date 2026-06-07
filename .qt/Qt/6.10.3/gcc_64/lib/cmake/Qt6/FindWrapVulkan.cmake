# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapVulkan::WrapVulkan)
    set(WrapVulkan_FOUND ON)
    return()
endif()

set(WrapVulkan_FOUND OFF)

find_package(Vulkan ${WrapVulkan_FIND_VERSION} QUIET)

if(Vulkan_FOUND)
    set(WrapVulkan_FOUND ON)

    add_library(WrapVulkan::WrapVulkan INTERFACE IMPORTED)
    target_link_libraries(WrapVulkan::WrapVulkan INTERFACE Vulkan::Vulkan)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapVulkan DEFAULT_MSG Vulkan_LIBRARY Vulkan_INCLUDE_DIR)
