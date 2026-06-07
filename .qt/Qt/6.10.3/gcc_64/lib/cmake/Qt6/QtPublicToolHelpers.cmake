# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The function returns location of the imported 'tool', returns an empty string if tool is not
# imported.
function(__qt_internal_get_tool_imported_location out_var tool)
    unset(${out_var})
    if("${tool}" MATCHES "^Qt[0-9]?::.+$")
        # The tool target has namespace already
        set(target ${tool})
    else()
        set(target ${QT_CMAKE_EXPORT_NAMESPACE}::${tool})
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} is not a target.")
    endif()

    get_target_property(is_imported ${target} IMPORTED)
    if(NOT is_imported)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_target_property(configs ${target} IMPORTED_CONFIGURATIONS)
    list(TRANSFORM configs PREPEND _)
    # Well-known configuration types
    list(APPEND configs
        _RELWITHDEBINFO
        _RELEASE
        _MINSIZEREL
        _DEBUG
    )
    list(REMOVE_DUPLICATES configs)
    # Look for the default empty configuration type at the first place.
    list(PREPEND configs "")

    foreach(config ${configs})
        get_target_property(${out_var} ${target} "IMPORTED_LOCATION${config}")
        if(${out_var})
            break()
        endif()
    endforeach()

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

function(_qt_internal_collect_tool_paths out_paths)
    set(prefixes "")

    # In a prefix build, the just-built tools should pick up libraries from the current repo build
    # dir.
    if(QT_BUILD_DIR)
        list(APPEND prefixes "${QT_BUILD_DIR}")
    endif()

    # Pick up libraries from the main location where Qt was installed during a Qt build.
    if(QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX)
        list(APPEND prefixes "${QT_BUILD_INTERNALS_RELOCATABLE_INSTALL_PREFIX}")
    endif()

    # Needed for ExternalProjects examples, where the Qt build dir is passed via this variable
    # to the example project.
    if(QT_ADDITIONAL_PACKAGES_PREFIX_PATH)
        __qt_internal_prefix_paths_to_roots(additional_roots
            "${QT_ADDITIONAL_PACKAGES_PREFIX_PATH}")
        list(APPEND prefixes ${QT_ADDITIONAL_PACKAGES_PREFIX_PATH})
    endif()

    # Fallback to wherever Qt6 package is.
    if(QT6_INSTALL_PREFIX)
        list(APPEND prefixes "${QT6_INSTALL_PREFIX}")
    endif()

    # When building qtbase, QT6_INSTALL_BINS is not set yet.
    if(INSTALL_BINDIR)
        set(bin_suffix "${INSTALL_BINDIR}")
    else()
        set(bin_suffix "${QT6_INSTALL_BINS}")
    endif()

    set(path_dirs "")
    foreach(prefix IN LISTS prefixes)
        set(bin_dir "${prefix}/${bin_suffix}")
        if(EXISTS "${bin_dir}")
            file(TO_NATIVE_PATH "${bin_dir}" path_dir)
            list(APPEND path_dirs "${path_dir}")
        endif()
    endforeach()

    set(${out_paths} "${path_dirs}" PARENT_SCOPE)
endfunction()

function(_qt_internal_generate_tool_command_wrapper)
    get_property(is_called GLOBAL PROPERTY _qt_internal_generate_tool_command_wrapper_called)
    if(NOT CMAKE_HOST_WIN32 OR is_called)
        return()
    endif()
    _qt_internal_collect_tool_paths(path_dirs)

    set(tool_command_wrapper_dir "${CMAKE_BINARY_DIR}/.qt/bin")
    file(MAKE_DIRECTORY "${tool_command_wrapper_dir}")
    set(tool_command_wrapper_path "${tool_command_wrapper_dir}/qt_setup_tool_path.bat")

    file(WRITE "${tool_command_wrapper_path}" "@echo off
set PATH=${path_dirs};%PATH%
%*")

    set(QT_TOOL_COMMAND_WRAPPER_PATH "${tool_command_wrapper_path}"
        CACHE INTERNAL "Path to the wrapper of the tool commands")

    set_property(GLOBAL PROPERTY _qt_internal_generate_tool_command_wrapper_called TRUE)
endfunction()

# Gets the path to tool wrapper shell script.
function(_qt_internal_get_tool_wrapper_script_path out_variable)
    # Ensure the script wrapper exists.
    _qt_internal_generate_tool_command_wrapper()

    set(${out_variable} "${QT_TOOL_COMMAND_WRAPPER_PATH}" PARENT_SCOPE)
endfunction()

# Attempts to run execute_process command in the Qt environment. The macro
# sets the PATH on windows platforms so the executable can locate the required
# Qt dlls. Marco arguments should be preliminary packed inside variable. The
# variable name then should be used as an execute_process_args_var argument:
#
# set(execute_echo_command
#     ${CMAKE_COMMAND} -E echo [["Test _qt_internal_execute_proccess_in_qt_env"]]
# )
# _qt_internal_execute_proccess_in_qt_env(execute_echo_command)
#
macro(_qt_internal_execute_proccess_in_qt_env execute_process_args_var)
    if(CMAKE_HOST_WIN32)
        _qt_internal_collect_tool_paths(path_dirs)
        set(_qt_internal_execute_proccess_in_qt_env_path_backup "$ENV{PATH}")
        set(ENV{PATH} "$ENV{PATH};${path_dirs}")
    endif()
    # We avoid escaping issues this way.
    execute_process(${${execute_process_args_var}})
    if(CMAKE_HOST_WIN32)
        set(ENV{PATH} "${_qt_internal_execute_proccess_in_qt_env_path_backup}")
        unset(_qt_internal_execute_proccess_in_qt_env_path_backup)
    endif()
endmacro()
