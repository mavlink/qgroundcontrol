# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Adds an SBOM build tool target to the current SBOM project.
# Expects the target not to be created yet.
#
# A build tool is something like a compiler, linker, cmake executable, which participates in the
# build process of the current project in some way.
#
# There is a predefined list of known tools which when specified will automatically query
# information like tool version and create an appropriate relationship comment.
#
# If CUSTOM is specified as the build tool type, it's expected that the caller sets any additional
# relevant SBOM options to the SBOM_ARGS option.
#
# If AUTO_ADD_PROJECT_RELATIONSHIP is specified, an automatic relationship entry is created
# that the tool is a BUILD_DEPENDENCY_OF the current project.
function(_qt_internal_add_sbom_build_tool target)
    if(NOT QT_GENERATE_SBOM)
        return()
    endif()

    set(opt_args
        AUTO_ADD_PROJECT_RELATIONSHIP
    )
    set(single_args
        BUILD_TOOL_TYPE
    )
    set(multi_args
        SBOM_ARGS
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(known_build_tool_types
        CMAKE
        CMAKE_GENERATOR

        COMPILER
        LINKER

        CUSTOM
    )

    if(NOT arg_BUILD_TOOL_TYPE)
        message(FATAL_ERROR "BUILD_TOOL_TYPE must be specified.")
    endif()

    if(NOT arg_BUILD_TOOL_TYPE IN_LIST known_build_tool_types)
        message(FATAL_ERROR "Unknown BUILD_TOOL_TYPE value: ${arg_BUILD_TOOL_TYPE}")
    endif()

    if(arg_BUILD_TOOL_TYPE STREQUAL "CUSTOM" AND arg_AUTO_ADD_PROJECT_RELATIONSHIP)
        message(FATAL_ERROR
            "AUTO_ADD_PROJECT_RELATIONSHIP cannot be used with CUSTOM BUILD_TOOL_TYPE. Consider "
            "creating a relationship manually by passing appropriate SBOM_RELATIONSHIP_ENTRIES "
            "to the SBOM_ARGS option.")
    endif()

    string(TOLOWER "${arg_BUILD_TOOL_TYPE}" build_tool_type_lower_case)

    if(NOT arg_BUILD_TOOL_TYPE STREQUAL "CUSTOM")
        set(build_tool_type_args "")
        if(arg_AUTO_ADD_PROJECT_RELATIONSHIP)
            list(APPEND build_tool_type_args GET_RELATIONSHIP_COMMENT)
        endif()

        _qt_internal_sbom_handle_default_build_tool_type(
            BUILD_TOOL_TYPE "${arg_BUILD_TOOL_TYPE}"
            OUT_VAR_SBOM_ARGS extra_args
            OUT_VAR_RELATIONSHIP_COMMENT relationship_comment
            ${build_tool_type_args}
        )
    endif()

    set(forward_args
        ${arg_SBOM_ARGS}
        SBOM_ENTITY_TYPE BUILD_TOOL
    )
    if(extra_args)
        list(APPEND forward_args ${extra_args})
    endif()

    _qt_internal_add_sbom("${target}" ${forward_args})

    if(arg_AUTO_ADD_PROJECT_RELATIONSHIP)
        _qt_internal_sbom_get_current_project_target(project_target)
        set(relationships
            SBOM_RELATIONSHIP_ENTRY
                SBOM_RELATIONSHIP_FROM
                    "${target}"
                SBOM_RELATIONSHIP_TYPE
                    BUILD_DEPENDENCY_OF
                SBOM_RELATIONSHIP_TO
                    "${project_target}"
                SBOM_RELATIONSHIP_COMMENT
                    "${relationship_comment}"
        )
        _qt_internal_extend_sbom("${project_target}"
            SBOM_RELATIONSHIP_ENTRIES
                ${relationships}
        )
    endif()
endfunction()

# Adds a set of default build tools that CMake uses for the current SBOM project.
# This includes the compiler, cmake, the make tool, and optionally the linker.
function(_qt_internal_sbom_add_project_default_build_tools)
    _qt_internal_sbom_get_project_default_build_tool_types(default_build_tool_types)

    foreach(build_tool_type IN LISTS default_build_tool_types)
        _qt_internal_sbom_get_build_tool_target_for_type(
            BUILD_TOOL_TYPE "${build_tool_type}"
            OUT_VAR_TARGET target
        )
        _qt_internal_add_sbom_build_tool("${target}"
            BUILD_TOOL_TYPE "${build_tool_type}"
            AUTO_ADD_PROJECT_RELATIONSHIP
        )
    endforeach()
endfunction()

# Returns the list of default build tool types that CMake uses for the current SBOM project.
function(_qt_internal_sbom_get_project_default_build_tool_types out_var)
    set(default_build_tool_types
        CMAKE
        CMAKE_GENERATOR
        COMPILER
    )

    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
        list(APPEND default_build_tool_types LINKER)
    endif()

    set(${out_var} "${default_build_tool_types}" PARENT_SCOPE)
endfunction()

# Returns the target name for one of the project default build tool types.
function(_qt_internal_sbom_get_build_tool_target_for_type)
    if(NOT QT_GENERATE_SBOM)
        if(arg_OUT_VAR_TARGET)
            set(${arg_OUT_VAR_TARGET} "" PARENT_SCOPE)
        endif()
        return()
    endif()

    set(opt_args "")
    set(single_args
        BUILD_TOOL_TYPE
        OUT_VAR_TARGET
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_BUILD_TOOL_TYPE)
        message(FATAL_ERROR "BUILD_TOOL_TYPE must be specified.")
    endif()

    if(NOT arg_OUT_VAR_TARGET)
        message(FATAL_ERROR "OUT_VAR_TARGET must be specified.")
    endif()

    set(known_types
        CMAKE
        CMAKE_GENERATOR
        COMPILER
        LINKER
    )

    if(NOT arg_BUILD_TOOL_TYPE IN_LIST known_types)
        message(FATAL_ERROR "Unknown BUILD_TOOL_TYPE value: ${arg_BUILD_TOOL_TYPE}")
    endif()

    _qt_internal_sbom_get_root_project_name_lower_case(project_name_lower_case)
    set(prefix "${project_name_lower_case}SbomBuildTool")

    if(arg_BUILD_TOOL_TYPE STREQUAL "CMAKE")
        set(target "${prefix}CMake")
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "CMAKE_GENERATOR")
        set(target "${prefix}CMakeGenerator")
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "COMPILER")
        set(target "${prefix}Compiler")
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "LINKER")
        set(target "${prefix}Linker")
    endif()

    set(${arg_OUT_VAR_TARGET} "${target}" PARENT_SCOPE)
endfunction()

# Returns common build tool information depending on the build tool type.
function(_qt_internal_sbom_handle_default_build_tool_type)
    set(opt_args
        GET_RELATIONSHIP_COMMENT
    )
    set(single_args
        BUILD_TOOL_TYPE
        OUT_VAR_SBOM_ARGS
        OUT_VAR_RELATIONSHIP_COMMENT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_BUILD_TOOL_TYPE)
        message(FATAL_ERROR "BUILD_TOOL_TYPE must be specified.")
    endif()

    if(NOT arg_OUT_VAR_SBOM_ARGS)
        message(FATAL_ERROR "OUT_VAR_SBOM_ARGS must be specified.")
    endif()

    if(arg_GET_RELATIONSHIP_COMMENT AND NOT arg_OUT_VAR_RELATIONSHIP_COMMENT)
        message(FATAL_ERROR "OUT_VAR_RELATIONSHIP_COMMENT must be specified.")
    endif()

    set(sbom_args "")
    set(build_tool_args "")

    if(arg_GET_RELATIONSHIP_COMMENT)
        _qt_internal_sbom_get_current_project_target(project_target)
        _qt_internal_sbom_get_current_project_spdx_id(project_spdx_id)
    endif()

    if(arg_BUILD_TOOL_TYPE STREQUAL "CMAKE")
        set(sbom_args
            FRIENDLY_PACKAGE_NAME "CMake"
            PACKAGE_VERSION "${CMAKE_VERSION}"
        )
        if(arg_GET_RELATIONSHIP_COMMENT)
            string(CONCAT relationship_comment
                "${project_spdx_id} is built using CMake version ${CMAKE_VERSION}"
            )
        endif()
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "CMAKE_GENERATOR")
        set(version "")

        set(sbom_args
            FRIENDLY_PACKAGE_NAME "CMake generator ${CMAKE_GENERATOR}"
        )

        if((CMAKE_GENERATOR MATCHES "Ninja"
                OR CMAKE_GENERATOR STREQUAL "Unix Makefiles"
                OR CMAKE_GENERATOR STREQUAL "MinGW Makefiles"
                OR CMAKE_GENERATOR STREQUAL "NMake Makefiles JOM"
                OR CMAKE_GENERATOR STREQUAL "Xcode")
                AND CMAKE_MAKE_PROGRAM
                AND NOT QT_SBOM_NO_QUERY_MAKE_PROGRAM_VERSION
            )
            if(CMAKE_GENERATOR STREQUAL "Xcode")
                set(query_version_option "-version")
            elseif(CMAKE_GENERATOR STREQUAL "NMake Makefiles JOM")
                # Regular NMake Makefiles doesn't have a /version option, so we don't extract
                # that atm.
                set(query_version_option "/version")
            else()
                # Ninja, Unix Makefiles, MinGW Makefiles
                set(query_version_option "--version")
            endif()

            execute_process(
                COMMAND "${CMAKE_MAKE_PROGRAM}" ${query_version_option}
                OUTPUT_VARIABLE make_tool_output
                ERROR_VARIABLE make_tool_output
                RESULT_VARIABLE make_tool_version_result
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(make_tool_version_result EQUAL 0)
                # Get the first line of output.
                string(REGEX MATCH "^[^\n\r]+" version_first_line "${make_tool_output}")

                # Strip characters that might break CMake argument parsing or SBOM generation.
                # Leave only alpha numeric characters, dots, dashes and spaces.
                string(REGEX REPLACE "[^a-zA-Z0-9. -]" "" version_first_line
                    "${version_first_line}")

                set(version "${version_first_line}")
            endif()
        endif()

        if(version)
            list(APPEND sbom_args PACKAGE_VERSION "${version}")
        endif()

        if(arg_GET_RELATIONSHIP_COMMENT)
            set(version_comment "")
            if(version)
                string(APPEND version_comment "version '${version}'")
            endif()

            if(CMAKE_GENERATOR_PLATFORM)
                string(APPEND version_comment
                    ", CMAKE_GENERATOR_PLATFORM '${CMAKE_GENERATOR_PLATFORM}'")
            endif()

            if(CMAKE_GENERATOR_TOOLSET)
                string(APPEND version_comment
                    ", CMAKE_GENERATOR_TOOLSET '${CMAKE_GENERATOR_TOOLSET}'")
            endif()

            if(CMAKE_GENERATOR_INSTANCE)
                string(APPEND version_comment
                    ", CMAKE_GENERATOR_INSTANCE '${CMAKE_GENERATOR_INSTANCE}'")
            endif()

            # This isn't set for the Visual Studio generator.
            if(CMAKE_MAKE_PROGRAM)
                get_filename_component(make_program_name "${CMAKE_MAKE_PROGRAM}" NAME)
                set(make_program_comment
                    "and CMAKE_MAKE_PROGRAM: '${make_program_name}' ")
            endif()

            string(CONCAT relationship_comment
                "${project_spdx_id} is built using generator CMAKE_GENERATOR: '${CMAKE_GENERATOR}' "
                "${make_program_comment}"
                "${version_comment}"
            )
        endif()
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "COMPILER")
        string(CONCAT package_summary
            "The compiler as identified by CMake, running on '${CMAKE_HOST_SYSTEM_NAME}' "
            "(${CMAKE_HOST_SYSTEM_PROCESSOR})")

        set(sbom_args
            FRIENDLY_PACKAGE_NAME "Compiler ${CMAKE_CXX_COMPILER_ID}"
            PACKAGE_VERSION "${CMAKE_CXX_COMPILER_VERSION}"
            PACKAGE_SUMMARY "${package_summary}"
        )

        if(arg_GET_RELATIONSHIP_COMMENT)
            string(CONCAT relationship_comment
                "${project_spdx_id} is built by compiler: '${CMAKE_CXX_COMPILER_ID}' "
                "version: '${CMAKE_CXX_COMPILER_VERSION}' "
                "frontend variant: '${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}'"
            )
        endif()
    elseif(arg_BUILD_TOOL_TYPE STREQUAL "LINKER")
        # Note this info is only available with CMake 3.29+, caller needs to ensure they don't call
        # this when the info is not available.
        set(sbom_args
            FRIENDLY_PACKAGE_NAME "Linker ${CMAKE_CXX_COMPILER_LINKER_ID}"
            PACKAGE_VERSION "${CMAKE_CXX_COMPILER_LINKER_VERSION}"
        )

        if(arg_GET_RELATIONSHIP_COMMENT)
            string(CONCAT relationship_comment
                "${project_spdx_id} is built by linker '${CMAKE_CXX_COMPILER_LINKER_ID}' "
                "version '${CMAKE_CXX_COMPILER_LINKER_VERSION}' "
                "frontend variant: '${CMAKE_CXX_COMPILER_LINKER_FRONTEND_VARIANT}'"
            )
        endif()
    endif()

    set(${arg_OUT_VAR_SBOM_ARGS} "${sbom_args}" PARENT_SCOPE)
    if(arg_GET_RELATIONSHIP_COMMENT)
        set(${arg_OUT_VAR_RELATIONSHIP_COMMENT} "${relationship_comment}" PARENT_SCOPE)
    endif()
endfunction()
