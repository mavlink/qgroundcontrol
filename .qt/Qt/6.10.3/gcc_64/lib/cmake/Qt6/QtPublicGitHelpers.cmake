# Copyright (C) 2024 The Qt Company Ltd.
# Copyright (C) 2023-2024 Jochem Rutgers
# SPDX-License-Identifier: BSD-3-Clause AND MIT

macro(_qt_internal_find_git_package)
    find_package(Git)
endmacro()

# Helper to set the various git version variables in the parent scope across multiple return points.
macro(_qt_internal_set_git_query_variables)
    set("${arg_OUT_VAR_PREFIX}git_hash" "${version_git_hash}" PARENT_SCOPE)
    set("${arg_OUT_VAR_PREFIX}git_hash_short" "${version_git_head}" PARENT_SCOPE)
    set("${arg_OUT_VAR_PREFIX}git_version" "${git_version}" PARENT_SCOPE)

    # git version sanitized for file paths.
    string(REGEX REPLACE "[^-a-zA-Z0-9_.]+" "+" git_version_path "${git_version}")
    set("${arg_OUT_VAR_PREFIX}git_version_path" "${git_version_path}" PARENT_SCOPE)
endmacro()

# Caches the results per working-directory in global cmake properties.
# Sets the following variables in the outer scope:
# - git_hash: Full git hash.
# - git_hash_short: Short git hash.
# - git_version: Git version string.
# - git_version_path: Git version string sanitized for file paths.
function(_qt_internal_query_git_version)
    set(opt_args
        EMPTY_VALUE_WHEN_NOT_GIT_REPO
    )
    set(single_args
        WORKING_DIRECTORY
        OUT_VAR_PREFIX
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_EMPTY_VALUE_WHEN_NOT_GIT_REPO)
        set(version_git_head "")
        set(version_git_hash "")
        set(version_git_branch "")
        set(version_git_tag "")
        set(git_version "")
    else()
        set(version_git_head "unknown")
        set(version_git_hash "")
        set(version_git_branch "dev")
        set(version_git_tag "")
        set(git_version "${version_git_head}+${version_git_branch}")
    endif()

    if(QT_SBOM_FAKE_GIT_VERSION)
        set(version_git_head "fakegithead")
        set(version_git_hash "fakegithash")
        set(version_git_branch "fakegitbranch")
        set(version_git_tag "fakegittag")
        set(git_version "${version_git_head}+${version_git_branch}")
        _qt_internal_set_git_query_variables()
        return()
    endif()

    if(NOT Git_FOUND)
        message(STATUS "Git not found, skipping querying git version.")
        _qt_internal_set_git_query_variables()
        return()
    endif()

    if(arg_WORKING_DIRECTORY)
        set(working_directory "${arg_WORKING_DIRECTORY}")
    else()
        set(working_directory "${PROJECT_SOURCE_DIR}")
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE is_inside_work_tree_output
        RESULT_VARIABLE is_inside_work_tree_result
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if((NOT is_inside_work_tree_result EQUAL 0) OR (NOT is_inside_work_tree_output STREQUAL "true"))
        message(STATUS "Git repo not found, skipping querying git version.")
        _qt_internal_set_git_query_variables()
        return()
    endif()

    get_cmake_property(git_hash_cache _qt_git_hash_cache_${working_directory})
    get_cmake_property(git_hash_short_cache _qt_git_hash_short_cache_${working_directory})
    get_cmake_property(git_version_cache _qt_git_version_cache_${working_directory})
    get_cmake_property(git_version_path_cache _qt_git_version_path_cache_${working_directory})
    if(git_hash_cache)
        set(git_hash "${git_hash_cache}")
        set(git_hash_short "${git_hash_short_cache}")
        set(git_version "${git_version_cache}")
        set(git_version_path "${git_version_path_cache}")
        _qt_internal_set_git_query_variables()
        return()
    endif()

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE version_git_head
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE version_git_hash
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE version_git_branch
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE version_git_tag
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REGEX REPLACE "[ \t\r\n].*$" "" version_git_tag "${version_git_tag}")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} status -s
        WORKING_DIRECTORY "${working_directory}"
        OUTPUT_VARIABLE version_git_dirty
        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT "${version_git_dirty}" STREQUAL "")
        set(version_git_dirty "+dirty")
    endif()

    if(NOT "${version_git_tag}" STREQUAL "")
        set(git_version "${version_git_tag}")

        if("${git_version}" MATCHES "^v[0-9]+\.")
            string(REGEX REPLACE "^v" "" git_version "${git_version}")
        endif()

        set(git_version "${git_version}${version_git_dirty}")
    else()
        set(git_version
            "${version_git_head}+${version_git_branch}${version_git_dirty}"
        )
    endif()

    set_property(GLOBAL PROPERTY _qt_git_hash_cache_${working_directory} "${git_hash}")
    set_property(GLOBAL PROPERTY _qt_git_hash_short_cache_${working_directory} "${git_hash_short}")
    set_property(GLOBAL PROPERTY _qt_git_version_cache_${working_directory} "${git_version}")
    set_property(GLOBAL PROPERTY _qt_git_version_path_cache_${working_directory}
        "${git_version_path}")

    _qt_internal_set_git_query_variables()
endfunction()
