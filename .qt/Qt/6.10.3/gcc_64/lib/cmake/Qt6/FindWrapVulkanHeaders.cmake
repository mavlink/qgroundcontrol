# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapVulkanHeaders::WrapVulkanHeaders)
    set(WrapVulkanHeaders_FOUND ON)
    return()
endif()

set(WrapVulkanHeaders_FOUND OFF)

find_package(Vulkan ${WrapVulkanHeaders_FIND_VERSION} QUIET)

# We are interested only in include headers. The libraries might be missing, so we can't check the
# _FOUND variable.
if(Vulkan_INCLUDE_DIR)
    set(WrapVulkanHeaders_FOUND ON)

    add_library(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE IMPORTED)
    target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
        ${Vulkan_INCLUDE_DIR})

    set_target_properties(WrapVulkanHeaders::WrapVulkanHeaders PROPERTIES
        _qt_is_nolink_target TRUE)

    set_target_properties(WrapVulkanHeaders::WrapVulkanHeaders PROPERTIES
        _qt_skip_include_dir_for_pri TRUE)

    # Also propagate MoltenVK include directory on Apple platforms if found.
    if(APPLE)
        # Check for the LunarG Vulkan SDK folder structure.
        set(__qt_molten_vk_include_path "${Vulkan_INCLUDE_DIR}/../../MoltenVK/include")
        get_filename_component(
            __qt_molten_vk_include_path
            "${__qt_molten_vk_include_path}" ABSOLUTE)
        if(EXISTS "${__qt_molten_vk_include_path}")
            target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
                ${__qt_molten_vk_include_path})
        endif()

        # Check for homebrew molten-vk folder structure
        set(__qt_molten_vk_homebrew_include_path "${Vulkan_INCLUDE_DIR}/../../include")
        get_filename_component(
            __qt_molten_vk_homebrew_include_path
            "${__qt_molten_vk_homebrew_include_path}" ABSOLUTE)
        if(EXISTS "${__qt_molten_vk_homebrew_include_path}")
            target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
                ${__qt_molten_vk_homebrew_include_path})
        endif()

        # Check for homebrew vulkan-headers folder structure
        # If instead of molten-vk folder, CMAKE_PREFIX_PATH points to Homebrew's
        # vulkan-headers installation, then we will not be able to find molten-vk
        # headers. If we assume that user has installed the molten-vk formula as
        # well, then we might have a chance to pick it up like this.
        if(Vulkan_INCLUDE_DIR MATCHES "/homebrew/Cellar/")
            set(__qt_standalone_molten_vk_homebrew_include_path
                    "${Vulkan_INCLUDE_DIR}/../../../../opt/molten-vk/include")
        else()
            set(__qt_standalone_molten_vk_homebrew_include_path
                    "${Vulkan_INCLUDE_DIR}/../../molten-vk/include")
        endif()
        get_filename_component(
            __qt_standalone_molten_vk_homebrew_include_path
            "${__qt_standalone_molten_vk_homebrew_include_path}" ABSOLUTE)
        if(EXISTS "${__qt_standalone_molten_vk_homebrew_include_path}")
            target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
                ${__qt_standalone_molten_vk_homebrew_include_path})
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapVulkanHeaders DEFAULT_MSG Vulkan_INCLUDE_DIR)
