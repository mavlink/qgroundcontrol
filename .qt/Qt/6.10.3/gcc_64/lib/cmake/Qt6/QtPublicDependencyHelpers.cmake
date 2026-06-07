# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Note that target_dep_list does not accept a list of values, but a var name that contains the
# list of dependencies. See foreach block for reference.
macro(_qt_internal_find_third_party_dependencies target target_dep_list)
    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_is_optional)
        list(GET __qt_${target}_target_dep 2 __qt_${target}_version)
        list(GET __qt_${target}_target_dep 3 __qt_${target}_components)
        list(GET __qt_${target}_target_dep 4 __qt_${target}_optional_components)
        set(__qt_${target}_find_package_args "${__qt_${target}_pkg}")
        if(__qt_${target}_version)
            list(APPEND __qt_${target}_find_package_args "${__qt_${target}_version}")
        endif()
        if(__qt_${target}_components)
            string(REPLACE " " ";" __qt_${target}_components "${__qt_${target}_components}")
            list(APPEND __qt_${target}_find_package_args COMPONENTS ${__qt_${target}_components})
        endif()
        if(__qt_${target}_optional_components)
            string(REPLACE " " ";"
                __qt_${target}_optional_components "${__qt_${target}_optional_components}")
            list(APPEND __qt_${target}_find_package_args
                 OPTIONAL_COMPONENTS ${__qt_${target}_optional_components})
        endif()

        _qt_internal_save_find_package_context_for_debugging(${target})

        if(__qt_${target}_is_optional)
            if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
                list(APPEND __qt_${target}_find_package_args QUIET)
            endif()
            find_package(${__qt_${target}_find_package_args})
        else()
            find_dependency(${__qt_${target}_find_package_args})
            if(NOT ${__qt_${target}_pkg}_FOUND)
                list(APPEND __qt_${target}_missing_deps "${__qt_${target}_pkg}")
            endif()
        endif()

        _qt_internal_get_package_components_id(
            PACKAGE_NAME "${__qt_${target}_pkg}"
            COMPONENTS ${__qt_${target}_components}
            OPTIONAL_COMPONENTS ${__qt_${target}_optional_components}
            OUT_VAR_KEY __qt_${target}_package_components_id
        )
        if(${__qt_${target}_pkg}_FOUND
                AND __qt_${target}_third_party_package_${__qt_${target}_package_components_id}_provided_targets)
            set(__qt_${target}_sbom_args "")

            if(${__qt_${target}_pkg}_VERSION)
                list(APPEND __qt_${target}_sbom_args
                    PACKAGE_VERSION "${${__qt_${target}_pkg}_VERSION}"
                )
            endif()

            foreach(__qt_${target}_provided_target
                    IN LISTS
                    __qt_${target}_third_party_package_${__qt_${target}_package_components_id}_provided_targets)

                _qt_internal_promote_3rd_party_provided_target_and_3rd_party_deps_to_global(
                    "${__qt_${target}_provided_target}")

                _qt_internal_sbom_record_system_library_usage(
                    "${__qt_${target}_provided_target}"
                    SBOM_ENTITY_TYPE SYSTEM_LIBRARY
                    FRIENDLY_PACKAGE_NAME "${__qt_${target}_pkg}"
                    ${__qt_${target}_sbom_args}
                )
            endforeach()
        endif()
    endforeach()
endmacro()

# Note that target_dep_list does not accept a list of values, but a var name that contains the
# list of dependencies. See foreach block for reference.
macro(_qt_internal_find_tool_dependencies target target_dep_list)
    if(NOT "${${target_dep_list}}" STREQUAL "" AND NOT "${QT_HOST_PATH}" STREQUAL "")
         # Make sure that the tools find the host tools first
         set(BACKUP_${target}_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
         set(BACKUP_${target}_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
         list(PREPEND CMAKE_PREFIX_PATH "${QT_HOST_PATH_CMAKE_DIR}"
             "${_qt_additional_host_packages_prefix_paths}")
         list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}"
             "${_qt_additional_host_packages_root_paths}")
    endif()

    unset(__qt_${target}_find_package_args)
    if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
        list(APPEND __qt_${target}_find_package_args QUIET)
    endif()

    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_version)

        _qt_internal_save_find_package_context_for_debugging(${target})

        find_package(${__qt_${target}_pkg}
            ${__qt_${target}_version}
            ${__qt_${target}_find_package_args}
            PATHS
                "${CMAKE_CURRENT_LIST_DIR}/.."
                "${_qt_cmake_dir}"
                ${_qt_additional_packages_prefix_paths}
        )
        if (NOT ${__qt_${target}_pkg}_FOUND AND NOT QT_ALLOW_MISSING_TOOLS_PACKAGES)
            set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
            set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
"${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency \
${__qt_${target}_pkg} could not be found.")
            if(NOT "${QT_HOST_PATH}" STREQUAL "")
                 set(CMAKE_PREFIX_PATH ${BACKUP_${target}_CMAKE_PREFIX_PATH})
                 set(CMAKE_FIND_ROOT_PATH ${BACKUP_${target}_CMAKE_FIND_ROOT_PATH})
            endif()
            return()
        endif()
    endforeach()
    if(NOT "${${target_dep_list}}" STREQUAL "" AND NOT "${QT_HOST_PATH}" STREQUAL "")
         set(CMAKE_PREFIX_PATH ${BACKUP_${target}_CMAKE_PREFIX_PATH})
         set(CMAKE_FIND_ROOT_PATH ${BACKUP_${target}_CMAKE_FIND_ROOT_PATH})
    endif()
endmacro()

# Please note the target_dep_list accepts not the actual list values but the list names that
# contain preformed dependencies. See foreach block for reference.
# The same applies for find_dependency_path_list.
macro(_qt_internal_find_qt_dependencies target target_dep_list find_dependency_path_list)
    list(APPEND __qt_${target}_find_qt_dependencies_save_QT_NO_PRIVATE_MODULE_WARNING
        ${QT_NO_PRIVATE_MODULE_WARNING}
    )
    set(QT_NO_PRIVATE_MODULE_WARNING ON)

    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_version)

        if (NOT ${__qt_${target}_pkg}_FOUND)
            _qt_internal_save_find_package_context_for_debugging(${target})

            find_dependency(${__qt_${target}_pkg} ${__qt_${target}_version}
                PATHS
                    ${QT_BUILD_CMAKE_PREFIX_PATH}
                    ${${find_dependency_path_list}}
                    ${_qt_additional_packages_prefix_paths}
                ${__qt_use_no_default_path_for_qt_packages}
            )
            if(NOT ${__qt_${target}_pkg}_FOUND)
                list(APPEND __qt_${target}_missing_deps "${__qt_${target}_pkg}")
            endif()
        endif()
    endforeach()

    list(POP_BACK __qt_${target}_find_qt_dependencies_save_QT_NO_PRIVATE_MODULE_WARNING
        QT_NO_PRIVATE_MODULE_WARNING
    )
endmacro()

# If a dependency package was not found, provide some hints in the error message on how to debug
# the issue.
#
# pkg_name_var should be the variable name that contains the package that was not found.
# e.g. __qt_Core_pkg
#
# message_out_var should contain the variable name of the  original "not found" message, and it
# will have the hints appended to it as a string. e.g. ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
#
# infix is used as a unique prefix to retrieve the find_package paths context for the last package
# that was not found, for debugging purposes.
#
# The function should not be called in Dependencies.cmake files directly, because find_dependency
# returns out of the included file.
macro(_qt_internal_suggest_dependency_debugging infix pkg_name_var message_out_var)
    if(${pkg_name_var}
        AND NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND
        AND ${message_out_var})
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
            string(APPEND ${message_out_var}
                "\nConfiguring with --debug-find-pkg=${${pkg_name_var}} might reveal \
details why the package was not found.")
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
            string(APPEND ${message_out_var}
                "\nConfiguring with -DCMAKE_FIND_DEBUG_MODE=TRUE might reveal \
details why the package was not found.")
        endif()

        if(NOT QT_DEBUG_FIND_PACKAGE)
            string(APPEND ${message_out_var}
                "\nConfiguring with -DQT_DEBUG_FIND_PACKAGE=ON will print the values of some of \
the path variables that find_package uses to try and find the package.")
        else()
            string(APPEND ${message_out_var}
                "\n find_package search path values and other context for the last package that was not found:"
                "\n  CMAKE_MODULE_PATH: ${_qt_${infix}_CMAKE_MODULE_PATH}"
                "\n  CMAKE_PREFIX_PATH: ${_qt_${infix}_CMAKE_PREFIX_PATH}"
                "\n  \$ENV{CMAKE_PREFIX_PATH}: $ENV{CMAKE_PREFIX_PATH}"
                "\n  CMAKE_FIND_ROOT_PATH: ${_qt_${infix}_CMAKE_FIND_ROOT_PATH}"
                "\n  _qt_additional_packages_prefix_paths: ${_qt_${infix}_qt_additional_packages_prefix_paths}"
                "\n  _qt_additional_host_packages_prefix_paths: ${_qt_${infix}_qt_additional_host_packages_prefix_paths}"
                "\n  _qt_cmake_dir: ${_qt_${infix}_qt_cmake_dir}"
                "\n  QT_HOST_PATH: ${QT_HOST_PATH}"
                "\n  Qt6HostInfo_DIR: ${Qt6HostInfo_DIR}"
                "\n  Qt6_DIR: ${Qt6_DIR}"
                "\n  CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}"
                "\n  CMAKE_FIND_ROOT_PATH_MODE_PACKAGE: ${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}"
                "\n  CMAKE_SYSROOT: ${CMAKE_SYSROOT}"
                "\n  \$ENV{PATH}: $ENV{PATH}"
            )
        endif()
    endif()
endmacro()

# Save find_package search paths context just before a find_package call, to be shown with a
# package not found message.
macro(_qt_internal_save_find_package_context_for_debugging infix)
    if(QT_DEBUG_FIND_PACKAGE)
        set(_qt_${infix}_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
        set(_qt_${infix}_CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
        set(_qt_${infix}_CMAKE_FIND_ROOT_PATH "${CMAKE_FIND_ROOT_PATH}")
        set(_qt_${infix}_qt_additional_packages_prefix_paths
            "${_qt_additional_packages_prefix_paths}")
        set(_qt_${infix}_qt_additional_host_packages_prefix_paths
            "${_qt_additional_host_packages_prefix_paths}")
        set(_qt_${infix}_qt_cmake_dir "${_qt_cmake_dir}")
    endif()
endmacro()

function(_qt_internal_determine_if_host_info_package_needed out_var)
    set(needed FALSE)

    # If a QT_HOST_PATH is provided when configuring qtbase, we assume it's a cross build
    # and thus we require the QT_HOST_PATH to be provided also when using the cross-built Qt.
    # This tells the QtConfigDependencies file to do appropriate requirement checks.
    if(NOT "${QT_HOST_PATH}" STREQUAL "" AND NOT QT_NO_REQUIRE_HOST_PATH_CHECK)
        set(needed TRUE)
    endif()
    set(${out_var} "${needed}" PARENT_SCOPE)
endfunction()

macro(_qt_internal_find_host_info_package platform_requires_host_info install_namespace)
    if(${platform_requires_host_info})
        find_package(${install_namespace}HostInfo
                     CONFIG
                     REQUIRED
                     PATHS "${QT_HOST_PATH}"
                           "${QT_HOST_PATH_CMAKE_DIR}"
                     NO_CMAKE_FIND_ROOT_PATH
                     NO_DEFAULT_PATH)
    endif()
endmacro()

macro(_qt_internal_setup_qt_host_path
        host_path_required
        initial_qt_host_path
        initial_qt_host_path_cmake_dir
    )
    # Set up QT_HOST_PATH and do sanity checks.
    # A host path is required when cross-compiling but optional when doing a native build.
    # Requiredness can be overridden via variable.
    if(DEFINED QT_REQUIRE_HOST_PATH_CHECK)
        set(_qt_platform_host_path_required "${QT_REQUIRE_HOST_PATH_CHECK}")
    elseif(DEFINED ENV{QT_REQUIRE_HOST_PATH_CHECK})
        set(_qt_platform_host_path_required "$ENV{QT_REQUIRE_HOST_PATH_CHECK}")
    else()
        set(_qt_platform_host_path_required "${host_path_required}")
    endif()

    if(_qt_platform_host_path_required)
        # QT_HOST_PATH precedence:
        # - cache variable / command line option
        # - environment variable
        # - initial QT_HOST_PATH when qtbase was configured (and the directory exists)
        if(NOT DEFINED QT_HOST_PATH)
            if(DEFINED ENV{QT_HOST_PATH})
                set(QT_HOST_PATH "$ENV{QT_HOST_PATH}" CACHE PATH "")
            elseif(NOT "${initial_qt_host_path}" STREQUAL "" AND EXISTS "${initial_qt_host_path}")
                set(QT_HOST_PATH "${initial_qt_host_path}" CACHE PATH "")
            endif()
        endif()

        if(NOT QT_HOST_PATH STREQUAL "")
            get_filename_component(_qt_platform_host_path_absolute "${QT_HOST_PATH}" ABSOLUTE)
        endif()

        if("${QT_HOST_PATH}" STREQUAL "" OR NOT EXISTS "${_qt_platform_host_path_absolute}")
            message(FATAL_ERROR
                "To use a cross-compiled Qt, please set the QT_HOST_PATH cache variable to the "
                "location of your host Qt installation.")
        endif()

        # QT_HOST_PATH_CMAKE_DIR is needed to work around the rerooting issue when looking for host
        # tools. See REROOT_PATH_ISSUE_MARKER.
        # Prefer initially configured path if none was explicitly set.
        if(NOT DEFINED QT_HOST_PATH_CMAKE_DIR)
            if(NOT "${initial_qt_host_path_cmake_dir}" STREQUAL ""
                  AND EXISTS "${initial_qt_host_path_cmake_dir}")
                set(QT_HOST_PATH_CMAKE_DIR "${initial_qt_host_path_cmake_dir}" CACHE PATH "")
            else()
                # First try to auto-compute the location instead of requiring to set
                # QT_HOST_PATH_CMAKE_DIR explicitly.
                set(__qt_candidate_host_path_cmake_dir "${QT_HOST_PATH}/lib/cmake")
                if(__qt_candidate_host_path_cmake_dir
                        AND EXISTS "${__qt_candidate_host_path_cmake_dir}")
                    set(QT_HOST_PATH_CMAKE_DIR
                        "${__qt_candidate_host_path_cmake_dir}" CACHE PATH "")
                endif()
            endif()
        endif()

        if(NOT QT_HOST_PATH_CMAKE_DIR STREQUAL "")
            get_filename_component(_qt_platform_host_path_cmake_dir_absolute
                                   "${QT_HOST_PATH_CMAKE_DIR}" ABSOLUTE)
        endif()

        if("${QT_HOST_PATH_CMAKE_DIR}" STREQUAL ""
                OR NOT EXISTS "${_qt_platform_host_path_cmake_dir_absolute}")
            message(FATAL_ERROR
                "To use a cross-compiled Qt, please set the QT_HOST_PATH_CMAKE_DIR cache variable "
                "to the location of your host Qt installation lib/cmake directory.")
        endif()
    endif()
endmacro()

function(_qt_internal_show_private_module_warning module)
    if(DEFINED QT_REPO_MODULE_VERSION OR QT_NO_PRIVATE_MODULE_WARNING OR QT_FIND_PRIVATE_MODULES)
        return()
    endif()

    get_cmake_property(warning_shown __qt_private_module_${module}_warning_shown)
    if(warning_shown)
        return()
    endif()

    message(WARNING
        "This project is using headers of the ${module} module and will therefore be tied "
        "to this specific Qt module build version. "
        "Running this project against other versions of the Qt modules may crash at any arbitrary "
        "point. This is not a bug, but a result of using Qt internals. You have been warned! "
        "\nYou can disable this warning by setting QT_NO_PRIVATE_MODULE_WARNING to ON."
    )
    set_property(GLOBAL PROPERTY __qt_private_module_${module}_warning_shown TRUE)
endfunction()
