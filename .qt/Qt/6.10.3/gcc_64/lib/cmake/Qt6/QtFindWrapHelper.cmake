# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Creates an imported wrapper target that links against either a Qt bundled package
# or a system package.
#
# Used for consuming 3rd party libraries in Qt.
#
# Example: Creates WrapFreetype::WrapFreetype linking against either
#          Qt6::BundledFreetype or WrapSystemFreetype::WrapSystemFreetype.
#
# The implementation has to use a unique prefix in each variable, otherwise when WrapFreetype
# find_package()s WrapPNG, the nested call would override the parent call variables, due to macros
# using the same scope.
macro(qt_find_package_system_or_bundled _unique_prefix)
    set(_flags "")
    set(_options
        FRIENDLY_PACKAGE_NAME
        WRAP_PACKAGE_TARGET
        WRAP_PACKAGE_FOUND_VAR_NAME
        BUNDLED_PACKAGE_NAME
        BUNDLED_PACKAGE_TARGET
        SYSTEM_PACKAGE_NAME
        SYSTEM_PACKAGE_TARGET
        )
    set(_multioptions "")

    cmake_parse_arguments("_qfwrap_${_unique_prefix}"
        "${_flags}" "${_options}" "${_multioptions}" ${ARGN})

    # We can't create the same interface imported target multiple times, CMake will complain if we
    # do that. This can happen if the find_package call is done in multiple different
    # subdirectories.
    if(TARGET "${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_TARGET}")
        set(${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_FOUND_VAR_NAME} ON)
        return()
    endif()

    set(${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_FOUND_VAR_NAME} OFF)

    include("FindWrap${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET}ConfigExtra" OPTIONAL)

    if(NOT DEFINED "QT_USE_BUNDLED_${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET}")
        message(FATAL_ERROR
            "Can't find cache variable "
            "QT_USE_BUNDLED_${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET} "
            "to decide whether to use bundled or system library.")
    endif()

    if("${QT_USE_BUNDLED_${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET}}")
        set(${_unique_prefix}_qt_package_name_to_use
            "Qt6${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_NAME}")
        set(${_unique_prefix}_qt_package_target_to_use
            "Qt6::${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET}")
        set(${_unique_prefix}_qt_package_success_message
            "Using Qt bundled ${_qfwrap_${_unique_prefix}_FRIENDLY_PACKAGE_NAME}.")
        set(${_unique_prefix}_qt_package_type "bundled")
    else()
        set(${_unique_prefix}_qt_package_name_to_use
            "${_qfwrap_${_unique_prefix}_SYSTEM_PACKAGE_NAME}")
        set(${_unique_prefix}_qt_package_target_to_use
            "${_qfwrap_${_unique_prefix}_SYSTEM_PACKAGE_TARGET}")
        set(${_unique_prefix}_qt_package_success_message
            "Using system ${_qfwrap_${_unique_prefix}_FRIENDLY_PACKAGE_NAME}.")
        set(${_unique_prefix}_qt_package_type "system")
    endif()

    if(NOT TARGET "${${_unique_prefix}_qt_package_target_to_use}")
        if(QT_USE_BUNDLED_${_qfwrap_${_unique_prefix}_BUNDLED_PACKAGE_TARGET})
            set(${_unique_prefix}_qt_use_no_default_path_for_qt_packages "NO_DEFAULT_PATH")
            if(QT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES)
                set(${_unique_prefix}_qt_use_no_default_path_for_qt_packages "")
            endif()

            # For bundled targets, we want to limit search paths to the relevant Qt search paths.
            find_package("${${_unique_prefix}_qt_package_name_to_use}"
                PATHS
                    ${QT_BUILD_CMAKE_PREFIX_PATH}
                    "${CMAKE_CURRENT_LIST_DIR}/.."
                    ${_qt_cmake_dir}
                    ${_qt_additional_packages_prefix_paths}
                ${${_unique_prefix}_qt_use_no_default_path_for_qt_packages}
            )
        else()
            # For the non-bundled case we will look for FindWrapSystemFoo.cmake module files,
            # which means we should not specify any PATHS.
            find_package("${${_unique_prefix}_qt_package_name_to_use}")
        endif()
    endif()

    if(TARGET "${${_unique_prefix}_qt_package_target_to_use}")
        set(${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_FOUND_VAR_NAME} ON)
        message(STATUS "${${_unique_prefix}_qt_package_success_message}")
        add_library("${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_TARGET}" INTERFACE IMPORTED)
        target_link_libraries("${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_TARGET}"
                              INTERFACE
                              ${${_unique_prefix}_qt_package_target_to_use})
        set_target_properties("${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_TARGET}" PROPERTIES
                              INTERFACE_QT_3RD_PARTY_PACKAGE_TYPE
                              "${${_unique_prefix}_qt_package_type}")


        # This might not be set, because qt_find_package() is called from a configure.cmake file via
        # qt_feature_evaluate_features, which means the _VERSION var is confined to the function
        # scope.
        if(${${_unique_prefix}_qt_package_name_to_use}_VERSION)
            set(_qfwrap_${_unique_prefix}_package_version
                "${${${_unique_prefix}_qt_package_name_to_use}_VERSION}"
            )
        else()
            # We set this in qt_find_package, so try to retrieve it.
            get_target_property(_qfwrap_${_unique_prefix}_package_version_from_prop
                "${${_unique_prefix}_qt_package_target_to_use}" _qt_package_found_version)
            if(_qfwrap_${_unique_prefix}_package_version_from_prop)
                set(_qfwrap_${_unique_prefix}_package_version
                    "${_qfwrap_${_unique_prefix}_package_version_from_prop}"
                )
            else()
                set(_qfwrap_${_unique_prefix}_package_version "")
            endif()
        endif()

        if(_qfwrap_${_unique_prefix}_package_version)
            set(Wrap${_qfwrap_${_unique_prefix}_FRIENDLY_PACKAGE_NAME}_VERSION
                "${_qfwrap_${_unique_prefix}_package_version}"
            )
            set(_qfwrap_${_unique_prefix}_package_version_option
                VERSION_VAR "Wrap${_qfwrap_${_unique_prefix}_FRIENDLY_PACKAGE_NAME}_VERSION"
            )
        endif()

        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(
            Wrap${_qfwrap_${_unique_prefix}_FRIENDLY_PACKAGE_NAME}
            REQUIRED_VARS ${_qfwrap_${_unique_prefix}_WRAP_PACKAGE_FOUND_VAR_NAME}
            ${_qfwrap_${_unique_prefix}_package_version_option}
        )
    elseif(${_unique_prefix}_qt_package_type STREQUAL "bundled")
        message(FATAL_ERROR "Can't find ${${_unique_prefix}_qt_package_target_to_use}.")
    endif()
endmacro()
