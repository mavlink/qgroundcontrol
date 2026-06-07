# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_create_nolink_target target dependee_target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "${target} does not exist when trying to build a nolink target.")
    endif()
    get_target_property(type "${target}" TYPE)
    if(type STREQUAL EXECUTABLE)
        message(FATAL_ERROR "${target} must be a library of some kind.")
    endif()
    if(type STREQUAL OBJECT_LIBRARY)
        message(FATAL_ERROR "${target} must not be an object library.")
    endif()

    # Strip off the namespace prefix, so from Vulkan::Vulkan to Vulkan, and then append _nolink.
    string(REGEX REPLACE "^.*::" "" non_prefixed_target ${target})
    set(nolink_target "${non_prefixed_target}_nolink")

    # Create the nolink interface target, assign the properties from the original target,
    # associate the nolink target with the same export which contains
    # the target that uses the _nolink target.
    # Also create a namespaced alias of the form ${target}::${target}_nolink which is used by
    # our modules.
    # Also create a Qt namespaced alias target, because when exporting via install(EXPORT)
    # Vulkan::Vulkan_nolink transforms into Qt6::Vulkan_nolink, and the latter needs to be an
    # accessible alias for standalone tests.
    if(NOT TARGET "${nolink_target}")
        add_library("${nolink_target}" INTERFACE)
        set(prefixed_nolink_target "${target}_nolink")

        # When configuring an example with qmake, if QtGui is built with Vulkan support but the
        # user's machine where Qt is installed doesn't have Vulkan, qmake doesn't fail saying
        # that vulkan is not installed. Instead it silently configures and just doesn't add
        # the include headers.
        # To mimic that in CMake, if the Vulkan CMake package is not found, we shouldn't fail
        # at generation time saying that the Vulkan target does not exist. Instead check with a
        # genex that the target exists to query the properties, otherwise just silently continue.
        # FIXME: If we figure out that such behavior should only be applied to Vulkan, and not the
        # other _nolink targets, we'll have to modify this to be configurable.
        set(target_exists_genex "$<TARGET_EXISTS:${target}>")
        set(props_to_set INTERFACE_INCLUDE_DIRECTORIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
                         INTERFACE_COMPILE_DEFINITIONS INTERFACE_COMPILE_OPTIONS
                         INTERFACE_COMPILE_FEATURES)
        foreach(prop ${props_to_set})
            set_target_properties(
                "${nolink_target}" PROPERTIES
                ${prop} $<${target_exists_genex}:$<TARGET_PROPERTY:${target},${prop}>>
                )
        endforeach()

        add_library(${prefixed_nolink_target} ALIAS ${nolink_target})
        add_library("${INSTALL_CMAKE_NAMESPACE}::${nolink_target}" ALIAS ${nolink_target})

        set(export_name "${INSTALL_CMAKE_NAMESPACE}${dependee_target}Targets")
        qt_install(TARGETS ${nolink_target} EXPORT ${export_name})
    endif()
endfunction()
