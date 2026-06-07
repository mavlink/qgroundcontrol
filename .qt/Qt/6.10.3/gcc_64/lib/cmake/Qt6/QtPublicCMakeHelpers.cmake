# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# copy_if_different works incorrect in Windows if file size if bigger than 2GB.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/23052 and QTBUG-99491 for details.
function(_qt_internal_copy_file_if_different_command out_var src_file dst_file)
    # The CMake version higher than 3.23 doesn't contain the issue
    if(CMAKE_HOST_WIN32 AND CMAKE_VERSION VERSION_LESS 3.23)
        set(${out_var} "${CMAKE_COMMAND}"
            "-DSRC_FILE_PATH=${src_file}"
            "-DDST_FILE_PATH=${dst_file}"
            -P "${_qt_6_config_cmake_dir}/QtCopyFileIfDifferent.cmake"
            PARENT_SCOPE
        )
    else()
        set(${out_var} "${CMAKE_COMMAND}"
            -E copy_if_different
            "${src_file}"
            "${dst_file}"
            PARENT_SCOPE
        )
    endif()
endfunction()

# The function checks if add_custom_command has the support of the DEPFILE argument.
function(_qt_internal_check_depfile_support out_var)
    if(CMAKE_GENERATOR MATCHES "Ninja" OR
        (CMAKE_VERSION VERSION_GREATER_EQUAL 3.20 AND CMAKE_GENERATOR MATCHES "Makefiles")
        OR (CMAKE_VERSION VERSION_GREATER_EQUAL 3.21
        AND (CMAKE_GENERATOR MATCHES "Xcode"
            OR (CMAKE_GENERATOR MATCHES "Visual Studio ([0-9]+)" AND CMAKE_MATCH_1 GREATER_EQUAL 12)
            )
        )
    )
        set(${out_var} TRUE)
    else()
        set(${out_var} FALSE)
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Checks if the path points to the cmake directory, like lib/cmake.
function(__qt_internal_check_path_points_to_cmake_dir result path)
    string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}" export_namespace_upper)
    if((INSTALL_LIBDIR AND path MATCHES "/${INSTALL_LIBDIR}/cmake$") OR
        (${export_namespace_upper}_INSTALL_LIBS AND
            path MATCHES "/${${export_namespace_upper}_INSTALL_LIBS}/cmake$") OR
        path MATCHES "/lib/cmake$"
    )
        set(${result} TRUE PARENT_SCOPE)
    else()
        set(${result} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Creates a reverse path to prefix from possible cmake directories. Returns the unchanged path
# if it doesn't point to cmake directory.
function(__qt_internal_reverse_prefix_path_from_cmake_dir result cmake_path)
    string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}" export_namespace_upper)
    if(INSTALL_LIBDIR AND cmake_path MATCHES "(.+)/${INSTALL_LIBDIR}/cmake$")
        if(CMAKE_MATCH_1)
            set(${result} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    elseif(${export_namespace_upper}_INSTALL_LIBS AND
        cmake_path MATCHES "(.+)/${${export_namespace_upper}_INSTALL_LIBS}/cmake$")
        if(CMAKE_MATCH_1)
            set(${result} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    elseif(result MATCHES "(.+)/lib/cmake$")
        if(CMAKE_MATCH_1)
            set(${result} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        endif()
    else()
        set(${result} "${cmake_path}" PARENT_SCOPE)
    endif()
endfunction()

# Returns the possible cmake directories based on prefix_path.
function(__qt_internal_get_possible_cmake_dirs out_paths prefix_path)
    set(${out_paths} "")

    if(EXISTS "${prefix_path}/lib/cmake")
        list(APPEND ${out_paths} "${prefix_path}/lib/cmake")
    endif()

    string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}" export_namespace_upper)
    set(next_path "${prefix_path}/${${export_namespace_upper}_INSTALL_LIBS}/cmake")
    if(${export_namespace_upper}_INSTALL_LIBS AND EXISTS "${next_path}")
        list(APPEND ${out_paths} "${next_path}")
    endif()

    set(next_path "${prefix_path}/${INSTALL_LIBDIR}/cmake")
    if(INSTALL_LIBDIR AND EXISTS "${next_path}")
        list(APPEND ${out_paths} "${next_path}")
    endif()

    list(REMOVE_DUPLICATES ${out_paths})
    set(${out_paths} "${${out_paths}}" PARENT_SCOPE)
endfunction()

# Collect additional package prefix paths to look for Qt packages, both from command line and the
# env variable ${prefixes_var}. The result is stored in ${out_var} and is a list of paths ending
# with "/lib/cmake".
function(__qt_internal_collect_additional_prefix_paths out_var prefixes_var)
    if(DEFINED "${out_var}")
        return()
    endif()

    set(additional_packages_prefix_paths "")

    set(additional_packages_prefixes "")
    if(${prefixes_var})
        list(APPEND additional_packages_prefixes ${${prefixes_var}})
    endif()
    if(DEFINED ENV{${prefixes_var}}
        AND NOT "$ENV{${prefixes_var}}" STREQUAL "")
        set(prefixes_from_env "$ENV{${prefixes_var}}")
        if(NOT CMAKE_HOST_WIN32)
            string(REPLACE ":" ";" prefixes_from_env "${prefixes_from_env}")
        endif()
        list(APPEND additional_packages_prefixes ${prefixes_from_env})
    endif()

    foreach(additional_path IN LISTS additional_packages_prefixes)
        file(TO_CMAKE_PATH "${additional_path}" additional_path)

        # The prefix paths need to end with lib/cmake to ensure the packages are found when
        # cross compiling. Search for REROOT_PATH_ISSUE_MARKER in the qt.toolchain.cmake file for
        # details.
        # We must pass the values via the PATHS options because the main find_package call uses
        # NO_DEFAULT_PATH, and thus CMAKE_PREFIX_PATH values are discarded.
        # CMAKE_FIND_ROOT_PATH values are not discarded and togegher with the PATHS option, it
        # ensures packages from additional prefixes are found.
        __qt_internal_check_path_points_to_cmake_dir(is_path_to_cmake "${additional_path}")
        if(is_path_to_cmake)
            list(APPEND additional_packages_prefix_paths "${additional_path}")
        else()
            __qt_internal_get_possible_cmake_dirs(additional_cmake_dirs "${additional_path}")
            list(APPEND additional_packages_prefix_paths ${additional_cmake_dirs})
        endif()
    endforeach()

    set("${out_var}" "${additional_packages_prefix_paths}" PARENT_SCOPE)
endfunction()

# Collects CMAKE_MODULE_PATH from QT_ADDITIONAL_PACKAGES_PREFIX_PATH
function(__qt_internal_collect_additional_module_paths)
    if(__qt_additional_module_paths_set)
        return()
    endif()
    foreach(prefix_path IN LISTS QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        __qt_internal_check_path_points_to_cmake_dir(is_path_to_cmake "${prefix_path}")
        if(is_path_to_cmake)
            list(APPEND CMAKE_MODULE_PATH "${prefix_path}/${QT_CMAKE_EXPORT_NAMESPACE}")
        else()
            __qt_internal_get_possible_cmake_dirs(additional_cmake_dirs "${prefix_path}")
            list(TRANSFORM additional_cmake_dirs APPEND "/${QT_CMAKE_EXPORT_NAMESPACE}")
            list(APPEND CMAKE_MODULE_PATH ${additional_cmake_dirs})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES CMAKE_MODULE_PATH)
    set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
    set(__qt_additional_module_paths_set TRUE PARENT_SCOPE)
endfunction()

# Take a list of prefix paths ending with "/lib/cmake", and return a list of absolute paths with
# "/lib/cmake" removed.
function(__qt_internal_prefix_paths_to_roots out_var prefix_paths)
    set(result "")
    foreach(path IN LISTS prefix_paths)
        __qt_internal_reverse_prefix_path_from_cmake_dir(path "${path}")
        list(APPEND result "${path}")
    endforeach()
    set("${out_var}" "${result}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_check_cxx_source_compiles_out_var out_output_var out_func_args)
    # This just resets the output var in the parent scope to an empty string.
    set(${out_output_var} "" PARENT_SCOPE)

    # OUTPUT_VARIABLE is an internal undocumented variable of check_cxx_source_compiles
    # since 3.23. Allow an opt out in case this breaks in the future.
    set(extra_func_args "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23"
            AND NOT QT_INTERNAL_NO_TRY_COMPILE_OUTPUT_VARIABLE)
        set(extra_func_args OUTPUT_VARIABLE ${out_output_var})
    endif()

    set(${out_func_args} "${extra_func_args}" PARENT_SCOPE)
endfunction()

# Sets TARGET_SUPPORTS_SHARED_LIBS to TRUE when using Emscripten to build Qt for WebAssembly
# or user projects.
# The upstream Emscripten cmake toolchain file sets TARGET_SUPPORTS_SHARED_LIBS to FALSE, claiming
# true shared library support is not available.
# See https://github.com/emscripten-core/emscripten/pull/8362#issuecomment-3050586255
# Upstream CMake 4.2+ will set it to TRUE itself, when not using the Emscripten cmake toolchain
# file directly.
function(_qt_internal_handle_target_supports_shared_libs)
    if(NOT EMSCRIPTEN)
        return()
    endif()

    if(QT_NO_ENABLE_TARGET_SUPPORTS_SHARED_LIBS)
        return()
    endif()

    get_property(target_supports_shared_libs GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
    if(target_supports_shared_libs)
        return()
    endif()

    set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)
endfunction()

# This function gets all targets below this directory
#
# Multi-value Arguments:
#     EXCLUDE list of target types that should be filtered from resulting list.
#
#     INCLUDE list of target types that should be filtered from resulting list.
#             EXCLUDE has higher priority than INCLUDE.
function(_qt_internal_collect_buildsystem_targets result dir)
    cmake_parse_arguments(arg "" "" "EXCLUDE;INCLUDE" ${ARGN})

    if(NOT _qt_internal_collect_buildsystem_targets_inner)
        set(${result} "")
        set(_qt_internal_collect_buildsystem_targets_inner TRUE)
    endif()

    set(forward_args "")
    if(arg_EXCLUDE)
        set(forward_args APPEND EXCLUDE ${arg_EXCLUDE})
    endif()

    if(arg_INCLUDE)
        set(forward_args APPEND INCLUDE ${arg_INCLUDE})
    endif()

    get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)

    # Make sure that we don't hit endless recursion when running qt-cmake-standalone-test on a
    # in-source test dir, where the currently processed directory lists itself in its SUBDIRECTORIES
    # property.
    # See https://bugreports.qt.io/browse/QTBUG-119998
    # and https://gitlab.kitware.com/cmake/cmake/-/issues/25489
    # Do it only when QT_INTERNAL_IS_STANDALONE_TEST is set, to avoid the possible slowdown when
    # processing many subdirectores when configuring all standalone tests rather than just one.
    if(QT_INTERNAL_IS_STANDALONE_TEST)
        list(REMOVE_ITEM subdirs "${dir}")
    endif()

    foreach(subdir IN LISTS subdirs)
        _qt_internal_collect_buildsystem_targets(${result} "${subdir}" ${forward_args})
    endforeach()
    get_property(sub_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(real_targets "")
    if(sub_targets)
        foreach(target IN LISTS sub_targets)
            get_target_property(target_type ${target} TYPE)
            if((NOT arg_INCLUDE OR target_type IN_LIST arg_INCLUDE) AND
                (NOT arg_EXCLUDE OR (NOT target_type IN_LIST arg_EXCLUDE)))
                list(APPEND real_targets ${target})
            endif()
        endforeach()
    endif()
    set(${result} ${${result}} ${real_targets} PARENT_SCOPE)
endfunction()

# Add a custom target ${target} that is *not* added to the default build target in a safe way.
# Dependencies must then be added with _qt_internal_add_phony_target_dependencies.
#
# What's "safe" in this context? For the Visual Studio generators, we cannot use add_dependencies,
# because this would enable the dependencies in the default build of the solution. See QTBUG-115166
# and upstream CMake issue #16668 for details. Instead, we record the dependencies (added with
# _qt_internal_add_phony_target_dependencies) and create the target at the end of the top-level
# directory scope.
#
# This only works if at least CMake 3.19 is used. Older CMake versions will trigger a warning that
# can be turned off with the variable ${WARNING_VARIABLE}.
#
# For other generators, this is just a call to add_custom_target, unless the target already exists,
# followed by add_dependencies.
#
# Use this function for targets that are not part of the default build, i.e. that should be
# triggered by the user.
#
# TARGET_CREATED_HOOK is the name of a function that is called after the target has been created.
# It takes the target's name as first and only argument.
#
# Example:
#     _qt_internal_add_phony_target(update_translations
#          WARNING_VARIABLE QT_NO_GLOBAL_LUPDATE_TARGET_CREATED_WARNING
#     )
#     _qt_internal_add_phony_target_dependencies(update_translations
#          narf_lupdate_zort_lupdate
#     )
#
function(_qt_internal_add_phony_target target)
    set(no_value_options "")
    set(single_value_options
        TARGET_CREATED_HOOK
        WARNING_VARIABLE
    )
    set(multi_value_options "")
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    if("${arg_WARNING_VARIABLE}" STREQUAL "")
        message(FATAL_ERROR "WARNING_VARIABLE must be provided.")
    endif()
    if(CMAKE_GENERATOR MATCHES "^Visual Studio ")
        if(${CMAKE_VERSION} VERSION_LESS "3.19.0")
            if(NOT ${${arg_WARNING_VARIABLE}})
                message(WARNING
                    "Cannot create target ${target} with this CMake version. "
                    "Please upgrade to CMake 3.19.0 or newer. "
                    "Set ${WARNING_VARIABLE} to ON to disable this warning."
                )
            endif()
            return()
        endif()

        get_property(already_deferred GLOBAL PROPERTY _qt_target_${target}_creation_deferred)
        if(NOT already_deferred)
            cmake_language(EVAL CODE
                "cmake_language(DEFER DIRECTORY \"${CMAKE_SOURCE_DIR}\" CALL _qt_internal_add_phony_target_deferred \"${target}\")"
            )
            if(DEFINED arg_TARGET_CREATED_HOOK)
                set_property(GLOBAL
                    PROPERTY _qt_target_${target}_creation_hook ${arg_TARGET_CREATED_HOOK}
                )
            endif()
        endif()
        set_property(GLOBAL APPEND PROPERTY _qt_target_${target}_creation_deferred ON)
    else()
        if(NOT TARGET ${target})
            add_custom_target(${target})
            if(DEFINED arg_TARGET_CREATED_HOOK)
                if(CMAKE_VERSION VERSION_LESS "3.19")
                    set(incfile
                        "${CMAKE_CURRENT_BINARY_DIR}/.qt_internal_add_phony_target.cmake"
                    )
                    file(WRITE "${incfile}" "${arg_TARGET_CREATED_HOOK}(${target})")
                    include("${incfile}")
                    file(REMOVE "${incfile}")
                else()
                    cmake_language(CALL "${arg_TARGET_CREATED_HOOK}" "${target}")
                endif()
            endif()
        endif()
    endif()
endfunction()

# Adds dependencies to a custom target that has been created with
# _qt_internal_add_phony_target. See the docstring at _qt_internal_add_phony_target for
# more details.
function(_qt_internal_add_phony_target_dependencies target)
    set(dependencies ${ARGN})
    if(CMAKE_GENERATOR MATCHES "^Visual Studio ")
        set_property(GLOBAL APPEND PROPERTY _qt_target_${target}_dependencies ${dependencies})

        # Exclude the dependencies from the solution's default build to avoid them being enabled
        # accidentally should the user add another dependency to them.
        set_target_properties(${dependencies} PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD ON)
    else()
        add_dependencies(${target} ${dependencies})
    endif()
endfunction()

# Hack for the Visual Studio generator. Create the custom target named ${target} and work
# around the lack of a working add_dependencies by calling 'cmake --build' for every dependency.
function(_qt_internal_add_phony_target_deferred target)
    get_property(target_dependencies GLOBAL PROPERTY _qt_target_${target}_dependencies)
    set(target_commands "")
    foreach(dependency IN LISTS target_dependencies)
        list(APPEND target_commands
            COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" -t ${dependency}
        )
    endforeach()
    add_custom_target(${target} ${target_commands})
    get_property(creation_hook GLOBAL PROPERTY _qt_target_${target}_creation_hook)
    if(creation_hook)
        cmake_language(CALL ${creation_hook} ${target})
    endif()
endfunction()

# The helper function that checks if module was included multiple times, and has the inconsistent
# set of targets that belong to the module. It's expected that either all 'targets' or none of them
# will be written to the 'targets_not_defined' variable, if the module was not or was
# searched before accordingly.
function(_qt_internal_check_multiple_inclusion targets_not_defined targets)
    set(targets_defined "")
    set(${targets_not_defined} "")
    set(expected_targets "")
    foreach(expected_target ${targets})
        list(APPEND expected_targets ${expected_target})
        if(NOT TARGET Qt::${expected_target})
            list(APPEND ${targets_not_defined} ${expected_target})
        endif()
        if(TARGET Qt::${expected_target})
            list(APPEND targets_defined ${expected_target})
        endif()
    endforeach()
    if("${targets_defined}" STREQUAL "${expected_targets}")
        set(${targets_not_defined} "" PARENT_SCOPE)
        return()
    endif()
    if(NOT "${targets_defined}" STREQUAL "")
        message(FATAL_ERROR "Some (but not all) targets in this export set were already defined."
            "\nTargets Defined: ${targets_defined}\nTargets not yet defined: "
            "${${targets_not_defined}}\n"
        )
    endif()
    set(${targets_not_defined} "${${targets_not_defined}}" PARENT_SCOPE)
endfunction()

# The function is used when creating version less targets using ALIASes.
function(_qt_internal_create_versionless_alias_targets targets install_namespace)
    foreach(target IN LISTS targets)
        add_library(Qt::${target} ALIAS ${install_namespace}::${target})
    endforeach()
endfunction()

# The function is used when creating version less targets from scratch but not using ALIASes.
# It assigns the known properties from the versioned targets to the versionless created in this
# function. This allows versionless targets mimic the versioned.
function(_qt_internal_create_versionless_targets targets install_namespace)
    set(known_imported_properties
        IMPORTED_LINK_DEPENDENT_LIBRARIES
    )

    set(known_interface_properties
        QT_MAJOR_VERSION
        AUTOMOC_MACRO_NAMES
        AUTOUIC_OPTIONS
        COMPILE_DEFINITIONS
        COMPILE_FEATURES
        COMPILE_OPTIONS
        CXX_MODULE_SETS
        HEADER_SETS
        HEADER_SETS_TO_VERIFY
        INCLUDE_DIRECTORIES
        LINK_DEPENDS
        LINK_DIRECTORIES
        LINK_LIBRARIES
        LINK_LIBRARIES_DIRECT
        LINK_LIBRARIES_DIRECT_EXCLUDE
        LINK_OPTIONS
        POSITION_INDEPENDENT_CODE
        PRECOMPILE_HEADERS
        SOURCES
        SYSTEM_INCLUDE_DIRECTORIES
    )

    set(known_qt_exported_properties
        MODULE_PLUGIN_TYPES
        QT_DISABLED_PRIVATE_FEATURES
        QT_DISABLED_PUBLIC_FEATURES
        QT_ENABLED_PRIVATE_FEATURES
        QT_ENABLED_PUBLIC_FEATURES
        QT_QMAKE_PRIVATE_CONFIG
        QT_QMAKE_PUBLIC_CONFIG
        QT_QMAKE_PUBLIC_QT_CONFIG
    )

    set(known_qt_exported_properties_interface_allowed
        _qt_config_module_name
        _qt_is_public_module
        _qt_module_has_headers
        _qt_module_has_private_headers
        _qt_module_has_public_headers
        _qt_module_has_qpa_headers
        _qt_module_has_rhi_headers
        _qt_module_include_name
        _qt_module_interface_name
        _qt_package_name
        _qt_package_version
        _qt_private_module_target_name
        _qt_sbom_spdx_id
        _qt_sbom_spdx_repo_document_namespace
        _qt_sbom_spdx_relative_installed_repo_document_path
        _qt_sbom_spdx_repo_project_name_lowercase
    )

    set(supported_target_types STATIC_LIBRARY MODULE_LIBRARY SHARED_LIBRARY OBJECT_LIBRARY
        INTERFACE_LIBRARY)

    foreach(target IN LISTS targets)
        if(NOT TARGET ${install_namespace}::${target})
            message(FATAL_ERROR "${install_namespace}::${target} is not a target, can not extend"
                " an alias target")
        endif()

        get_target_property(type ${install_namespace}::${target} TYPE)
        if(NOT type)
            message(FATAL_ERROR "Cannot get the ${install_namespace}::${target} target type.")
        endif()

        if(NOT "${type}" IN_LIST supported_target_types)
            message(AUTHOR_WARNING "${install_namespace}::${target} requires the versionless"
                " target creation, but it has incompatible type ${type}.")
            continue()
        endif()

        string(REPLACE "_LIBRARY" "" creation_type "${type}")
        add_library(Qt::${target} ${creation_type} IMPORTED)

        if(NOT "${type}" STREQUAL "INTERFACE_LIBRARY")
            foreach(config "" _RELEASE _DEBUG _RELWITHDEBINFO _MINSIZEREL)
                get_target_property(target_imported_location
                        ${install_namespace}::${target} IMPORTED_LOCATION${config})
                if(NOT target_imported_location)
                    if("${config}" STREQUAL "")
                        message(FATAL_ERROR "Cannot create versionless target for"
                            " ${install_namespace}::${target}. IMPORTED_LOCATION property is "
                            "missing."
                        )
                    else()
                        continue()
                    endif()
                endif()
                set_property(TARGET Qt::${target} PROPERTY
                    IMPORTED_LOCATION${config} "${target_imported_location}")

                foreach(property IN LISTS known_imported_properties)
                    get_target_property(exported_property_value
                        ${install_namespace}::${target} ${property}${config})
                    if(exported_property_value)
                        set_property(TARGET Qt::${target} APPEND PROPERTY
                            ${property} "${exported_property_value}")
                    endif()
                endforeach()
            endforeach()

            foreach(property IN LISTS known_qt_exported_properties)
                get_target_property(exported_property_value
                    ${install_namespace}::${target} ${property})
                if(exported_property_value)
                    set_property(TARGET Qt::${target} APPEND PROPERTY
                        ${property} "${exported_property_value}")
                endif()
            endforeach()
        endif()

        foreach(property IN LISTS known_interface_properties)
            get_target_property(interface_property_value
                ${install_namespace}::${target} INTERFACE_${property})
            if(interface_property_value)
                set_property(TARGET Qt::${target} APPEND PROPERTY
                    INTERFACE_${property} "${interface_property_value}")
            endif()
        endforeach()

        foreach(property IN LISTS known_qt_exported_properties_interface_allowed)
            get_target_property(exported_property_value
                ${install_namespace}::${target} ${property})
            if(exported_property_value)
                set_property(TARGET Qt::${target} APPEND PROPERTY
                    ${property} "${exported_property_value}")
            endif()
        endforeach()

        set_property(TARGET Qt::${target} PROPERTY _qt_is_versionless_target TRUE)
    endforeach()
endfunction()

# Checks whether any unparsed arguments have been passed to the function at the call site.
# Use this right after `cmake_parse_arguments`.
function(_qt_internal_validate_all_args_are_parsed prefix)
    if(DEFINED ${prefix}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: (${${prefix}_UNPARSED_ARGUMENTS})")
    endif()
endfunction()

# Takes a list of path components and joins them into one path separated by forward slashes "/",
# and saves the path in out_var.
# Filters out any path parts that are bare "."s.
function(_qt_internal_path_join out_var)
    set(args ${ARGN})

    # Remove any bare ".", to avoid any CMP0177 warnings for paths passed to install().
    list(REMOVE_ITEM args ".")

    string(JOIN "/" path ${args})
    set(${out_var} ${path} PARENT_SCOPE)
endfunction()

# _qt_internal_qt_remove_args can remove arguments from an existing list of function
# arguments in order to pass a filtered list of arguments to a different function.
# Parameters:
#   out_var: result of remove all arguments specified by ARGS_TO_REMOVE from ALL_ARGS
#   ARGS_TO_REMOVE: Arguments to remove.
#   ALL_ARGS: All arguments supplied to cmake_parse_arguments
#   from which ARGS_TO_REMOVE should be removed from. We require all the
#   arguments or we can't properly identify the range of the arguments detailed
#   in ARGS_TO_REMOVE.
#   ARGS: Arguments passed into the function, usually ${ARGV}
#
#   E.g.:
#   We want to forward all arguments from foo to bar, execpt ZZZ since it will
#   trigger an error in bar.
#
#   foo(target BAR .... ZZZ .... WWW ...)
#   bar(target BAR.... WWW...)
#
#   function(foo target)
#       cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "BAR;ZZZ;WWW")
#       qt_remove_args(forward_args
#           ARGS_TO_REMOVE ${target} ZZZ
#           ALL_ARGS ${target} BAR ZZZ WWW
#           ARGS ${ARGV}
#       )
#       bar(${target} ${forward_args})
#   endfunction()
#
function(_qt_internal_remove_args out_var)
    cmake_parse_arguments(arg "" "" "ARGS_TO_REMOVE;ALL_ARGS;ARGS" ${ARGN})
    set(result ${arg_ARGS})
    foreach(arg IN LISTS arg_ARGS_TO_REMOVE)
        # find arg
        list(FIND result ${arg} find_result)
        if (NOT find_result EQUAL -1)
            # remove arg
            list(REMOVE_AT result ${find_result})
            list(LENGTH result result_len)
            if(find_result EQUAL result_len)
                # We removed the last argument, could have been an option keyword
                continue()
            endif()
            list(GET result ${find_result} arg_current)
            # remove values until we hit another arg or the end of the list
            while(NOT "${arg_current}" IN_LIST arg_ALL_ARGS AND find_result LESS result_len)
                list(REMOVE_AT result ${find_result})
                list(LENGTH result result_len)
                if (NOT find_result EQUAL result_len)
                    list(GET result ${find_result} arg_current)
                endif()
            endwhile()
        endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(__qt_internal_handle_find_all_qt_module_packages out_var)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        COMPONENTS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_COMPONENTS)
        return()
    endif()

    set(components ${arg_COMPONENTS})

    if("ALL_QT_MODULES" IN_LIST components)
        list(FIND components "ALL_QT_MODULES" all_qt_modules_index)
        list(REMOVE_AT components ${all_qt_modules_index})

        # Find the path to dir with module.json files. # We consider each file name to be a
        # Qt package (component) name that contains a target with the same name.
        set(json_modules_path "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_DESCRIPTIONSDIR}")
        file(GLOB json_modules "${json_modules_path}/*.json")

        set(all_qt_modules "")

        foreach(json_module IN LISTS json_modules)
            get_filename_component(module_name "${json_module}" NAME_WE)
            list(APPEND all_qt_modules ${module_name})
        endforeach()

        if(all_qt_modules)
            list(INSERT components "${all_qt_modules_index}" ${all_qt_modules})
        endif()
    endif()

    set(${out_var} "${components}" PARENT_SCOPE)
endfunction()

# Append ${ARGN} to ${target}'s ${property_name} property, removing duplicates.
function(_qt_internal_append_to_target_property_without_duplicates target property_name)
    get_target_property(property "${target}" "${property_name}")
    if(NOT property)
        set(property "")
    endif()
    list(APPEND property ${ARGN})
    list(REMOVE_DUPLICATES property)
    set_property(TARGET "${target}" PROPERTY "${property_name}" "${property}")
endfunction()

# Append ${ARGN} to global CMake ${property_name} property, removing duplicates.
function(_qt_internal_append_to_cmake_property_without_duplicates property_name)
    get_cmake_property(property "${property_name}")
    if(NOT property)
        set(property "")
    endif()
    list(APPEND property ${ARGN})
    list(REMOVE_DUPLICATES property)
    set_property(GLOBAL PROPERTY "${property_name}" "${property}")
endfunction()

# Helper function to forward options from one function to another.
#
# This is somewhat the opposite of _qt_internal_remove_args.
#
# Parameters:
# FORWARD_PREFIX is usually arg because we pass cmake_parse_arguments(PARSE_ARGV 0 arg) in most code
# FORWARD_OPTIONS, FORWARD_SINGLE, FORWARD_MULTI are the options that should be forwarded.
#
# The forwarded args will be either set in arg_FORWARD_OUT_VAR or appended if FORWARD_APPEND is set.
#
# The function reads the options like ${arg_FORWARD_PREFIX}_${option} in the parent scope.
function(_qt_internal_forward_function_args)
    set(opt_args
        FORWARD_APPEND
    )
    set(single_args
        FORWARD_PREFIX
    )
    set(multi_args
        FORWARD_OPTIONS
        FORWARD_SINGLE
        FORWARD_MULTI
        FORWARD_OUT_VAR
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_FORWARD_OUT_VAR)
        message(FATAL_ERROR "FORWARD_OUT_VAR must be provided.")
    endif()

    set(forward_args "")
    foreach(option_name IN LISTS arg_FORWARD_OPTIONS)
        if(${arg_FORWARD_PREFIX}_${option_name})
            list(APPEND forward_args "${option_name}")
        endif()
    endforeach()

    foreach(option_name IN LISTS arg_FORWARD_SINGLE)
        if(NOT "${${arg_FORWARD_PREFIX}_${option_name}}" STREQUAL "")
            list(APPEND forward_args "${option_name}" "${${arg_FORWARD_PREFIX}_${option_name}}")
        endif()
    endforeach()

    foreach(option_name IN LISTS arg_FORWARD_MULTI)
        if(NOT "${${arg_FORWARD_PREFIX}_${option_name}}" STREQUAL "")
            list(APPEND forward_args "${option_name}" ${${arg_FORWARD_PREFIX}_${option_name}})
        endif()
    endforeach()

    if(arg_FORWARD_APPEND)
        set(forward_args ${${arg_FORWARD_OUT_VAR}} "${forward_args}")
    endif()

    set(${arg_FORWARD_OUT_VAR} "${forward_args}" PARENT_SCOPE)
endfunction()

# Function adds the transitive property of the specified type to a target, avoiding duplicates.
# Supported types: COMPILE, LINK
#
# See:
#   https://cmake.org/cmake/help/latest/prop_tgt/TRANSITIVE_COMPILE_PROPERTIES.html
#   https://cmake.org/cmake/help/latest/prop_tgt/TRANSITIVE_LINK_PROPERTIES.html
function(_qt_internal_add_transitive_property target type property)
    if(CMAKE_VERSION VERSION_LESS 3.30)
        return()
    endif()
    if(NOT type MATCHES "^(COMPILE|LINK)$")
        message(FATAL_ERROR "Attempt to assign unknown TRANSITIVE_${type}_PROPERTIES property")
    endif()

    _qt_internal_dealias_target(target)
    get_target_property(transitive_properties ${target}
        TRANSITIVE_${type}_PROPERTIES)
    if(NOT "${property}" IN_LIST transitive_properties)
        set_property(TARGET ${target}
            APPEND PROPERTY TRANSITIVE_${type}_PROPERTIES ${property})
    endif()
endfunction()

# Compatibility of `cmake_path(RELATIVE_PATH)`
#
# In order to be compatible with `file(RELATIVE_PATH)`, path normalization of the result is
# always performed, with the trailing slash stripped.
#
# Synopsis
#
#   _qt_internal_relative_path(<path-var>
#       [BASE_DIRECTORY <input>]
#       [OUTPUT_VARIABLE <out-var>]
#   )
#
# Arguments
#
# `path-var`
#   Equivalent to `cmake_path(RELATIVE_PATH <path-var>)`.
#
# `BASE_DIRECTORY`
#   Equivalent to `cmake_path(RELATIVE_PATH BASE_DIRECTORY)`.
#
# `OUTPUT_VARIABLE`
#   Equivalent to `cmake_path(RELATIVE_PATH OUTPUT_VARIABLE)`.
function(_qt_internal_relative_path path_var)
    set(option_args "")
    set(single_args
        BASE_DIRECTORY
        OUTPUT_VARIABLE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 1 arg "${option_args}" "${single_args}" "${multi_args}")

    if(NOT arg_BASE_DIRECTORY)
        set(arg_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    if(NOT arg_OUTPUT_VARIABLE)
        set(arg_OUTPUT_VARIABLE ${path_var})
    endif()

    if(CMAKE_VERSION VERSION_LESS 3.20)
        file(RELATIVE_PATH ${arg_OUTPUT_VARIABLE}
            "${arg_BASE_DIRECTORY}"
            "${${path_var}}")
    else()
        cmake_path(RELATIVE_PATH ${path_var}
            BASE_DIRECTORY "${arg_BASE_DIRECTORY}"
            OUTPUT_VARIABLE ${arg_OUTPUT_VARIABLE}
        )
        cmake_path(NORMAL_PATH ${arg_OUTPUT_VARIABLE})
        string(REGEX REPLACE "/$" "" ${arg_OUTPUT_VARIABLE}
                "${${arg_OUTPUT_VARIABLE}}")
    endif()
    set(${arg_OUTPUT_VARIABLE} "${${arg_OUTPUT_VARIABLE}}" PARENT_SCOPE)
endfunction()

# Compatibility of `cmake_path(IS_PREFIX)`
#
# NORMALIZE keyword is not supported
#
# Synopsis
#
#   _qt_internal_path_is_prefix(<path-var> <input> <out-var>)
#
# Arguments
#
# `path-var`
#   Equivalent to `cmake_path(IS_PREFIX <path-var>)`.
#
# `input`
#   Equivalent to `cmake_path(IS_PREFIX <input>)`.
#
# `out-var`
#   Equivalent to `cmake_path(IS_PREFIX <out-var>)`.
function(_qt_internal_path_is_prefix path_var input out_var)

    # Make sure the path ends with `/`
    if(NOT ${path_var} MATCHES "/$")
        set(${path_var} "${${path_var}}/")
    endif()
    # For the case input == path_var, we need to also include a trailing `/` to match the
    # previous change. We discard the actual path for input so we can add it unconditionally
    set(input "${input}/")
    if(CMAKE_VERSION VERSION_LESS 3.20)
        string(FIND "${input}" "${${path_var}}" find_pos)
        if(find_pos EQUAL 0)
            # input starts_with path_var
            set(${out_var} ON)
        else()
            set(${out_var} OFF)
        endif()
    else()
        cmake_path(IS_PREFIX ${path_var} ${input} ${out_var})
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Configures the file using either the input template or the CONTENT.
# Behaves as either file(CONFIGURE or configure_file( command, but do not depend
# on CMake version.
#
# Synopsis
#    _qt_internal_configure_file(<CONFIGURE|GENERATE>
#        OUTPUT <path>
#        <INPUT path|CONTENT data>
#    )
#
# Arguments
# `CONFIGURE` Run in pure CONFIGURE mode.
#
# `GENERATE` Configure CONTENT and generate file with the generator expression
#   support.
#
# `OUTPUT` The output file name to generate.
#
# `INPUT` The input template file name.
#
# `CONTENT` The template content. If both INPUT and CONTENT are specified, INPUT
#   argument is ignored.
function(_qt_internal_configure_file mode)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "OUTPUT;INPUT;CONTENT" "")

    if(NOT arg_OUTPUT)
        message(FATAL_ERROR "No OUTPUT file provided to _qt_internal_configure_file.")
    endif()

    # Substitute the '@' wrapped variables and generate the file with the
    # the generator expressions evaluation inside the resulting CONTENT.
    if(mode STREQUAL "GENERATE")
        if(arg_INPUT)
            configure_file("${arg_INPUT}" "${arg_OUTPUT}.tmp" @ONLY)
            file(GENERATE OUTPUT "${arg_OUTPUT}" INPUT "${arg_OUTPUT}.tmp")
        else()
            string(CONFIGURE "${arg_CONTENT}" arg_CONTENT @ONLY)
            file(GENERATE OUTPUT "${arg_OUTPUT}" CONTENT "${arg_CONTENT}")
        endif()
        return()
    endif()

    # We use this check for the cases when the specified CONTENT is empty. The value of arg_CONTENT
    # is undefined, but we still want to create a file with empty content.
    if("CONTENT" IN_LIST ARGV)
        if(arg_INPUT)
            message(WARNING "Both CONTENT and INPUT are specified. CONTENT will be used to generate"
                " output")
        endif()
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.18)
            file(CONFIGURE OUTPUT "${arg_OUTPUT}" CONTENT "${arg_CONTENT}" @ONLY)
            return()
        endif()
        set(template_name "QtFileConfigure.txt.in")
        # When building qtbase, use the source template file.
        # Otherwise use the installed file (basically wherever Qt6 package is found).
        # This should work for non-prefix and superbuilds as well.
        if(QtBase_SOURCE_DIR)
            set(input_file "${QtBase_SOURCE_DIR}/cmake/${template_name}")
        else()
            set(input_file "${_qt_6_config_cmake_dir}/${template_name}")
        endif()
        set(__qt_file_configure_content "${arg_CONTENT}")
    elseif(arg_INPUT)
        set(input_file "${arg_INPUT}")
    else()
        message(FATAL_ERROR "No input value provided to _qt_internal_configure_file.")
    endif()

    configure_file("${input_file}" "${arg_OUTPUT}" @ONLY)
endfunction()

# The function checks if `value` is a valid C indentifier.
#
# Synopsis
#
#  _qt_internal_is_c_identifier(<out_var> <value>)
#
# Arguments
#
#  `out_var`
#    Variable name for the evaluation result.
#
#  `value`
#    The string for the evaluation.
function(_qt_internal_is_c_identifier out_var value)
    string(MAKE_C_IDENTIFIER "${value}" value_valid)

    if(value AND "${value}" STREQUAL "${value_valid}")
        set(${out_var} "TRUE" PARENT_SCOPE)
    else()
        set(${out_var} "FALSE" PARENT_SCOPE)
    endif()
endfunction()

# Makes appending of the CMake configure time dependencies unique.
function(_qt_internal_append_cmake_configure_depends)
    get_property(configure_depends DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS)
    foreach(path IN LISTS ARGN)
        get_filename_component(abs_path "${path}" REALPATH)
        if(NOT "${abs_path}" IN_LIST configure_depends)
            list(APPEND configure_depends "${abs_path}")
        endif()
    endforeach()
    set_property(DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS "${configure_depends}")
endfunction()

function(_qt_internal_get_moc_compiler_flavor_flags out_var)
    set(flags "")
    if(WIN32)
        list(APPEND flags -DWIN32)
    endif()
    if(MSVC)
        list(APPEND flags --compiler-flavor=msvc)
    endif()

    set(${out_var} "${flags}" PARENT_SCOPE)
endfunction()

# Create a non-existent, unique target name.
# If the target ${name} doesn't exist, return that name in ${out_var}.
# Otherwise, append an incrementing number, starting with 1.
function(_qt_internal_unique_target_name out_var name)
    set(target "${name}")
    set(id 0)
    while(TARGET "${target}")
        math(EXPR id "${id} + 1")
        set(target "${name}_${id}")
    endwhile()
    set("${out_var}" "${target}" PARENT_SCOPE)
endfunction()
