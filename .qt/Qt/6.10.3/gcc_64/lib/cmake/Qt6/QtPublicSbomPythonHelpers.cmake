# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Helper macro to find python and a given dependency. Expects the caller to set all of the vars.
# Meant to reduce the line noise due to the repeated calls.
macro(_qt_internal_sbom_find_python_and_dependency_helper_lambda)
    _qt_internal_sbom_find_python_and_dependency_helper(
        PYTHON_ARGS
            ${extra_python_args}
            ${python_common_args}
        DEPENDENCY_ARGS
            DEPENDENCY_IMPORT_STATEMENT "${import_statement}"
        OUT_VAR_PYTHON_PATH python_path
        OUT_VAR_PYTHON_FOUND python_found
        OUT_VAR_PYTHON_VERSION python_version
        OUT_VAR_DEP_FOUND dep_found
        OUT_VAR_PYTHON_AND_DEP_FOUND everything_found
        OUT_VAR_DEP_FIND_OUTPUT dep_find_output
    )
endmacro()

# Tries to find python and a given dependency based on the args passed to PYTHON_ARGS and
# DEPENDENCY_ARGS which are forwarded to the respective finding functions.
# Returns the path to the python interpreter, whether it was found, whether the dependency was
# found, whether both were found, and the reason why the dependency might not be found.
function(_qt_internal_sbom_find_python_and_dependency_helper)
    set(opt_args)
    set(single_args
        OUT_VAR_PYTHON_PATH
        OUT_VAR_PYTHON_FOUND
        OUT_VAR_PYTHON_VERSION
        OUT_VAR_DEP_FOUND
        OUT_VAR_PYTHON_AND_DEP_FOUND
        OUT_VAR_DEP_FIND_OUTPUT
    )
    set(multi_args
        PYTHON_ARGS
        DEPENDENCY_ARGS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(everything_found_inner FALSE)
    set(deps_find_output_inner "")

    if(NOT arg_OUT_VAR_PYTHON_PATH)
        message(FATAL_ERROR "OUT_VAR_PYTHON_PATH var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_DEP_FOUND)
        message(FATAL_ERROR "OUT_VAR_DEP_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_AND_DEP_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_AND_DEP_FOUND var is required")
    endif()

    if(NOT arg_OUT_VAR_DEP_FIND_OUTPUT)
        message(FATAL_ERROR "OUT_VAR_DEP_FIND_OUTPUT var is required")
    endif()

    _qt_internal_sbom_find_python_helper(
        ${arg_PYTHON_ARGS}
        OUT_VAR_PYTHON_PATH python_path_inner
        OUT_VAR_PYTHON_FOUND python_found_inner
        OUT_VAR_PYTHON_VERSION python_version_inner
    )

    if(python_found_inner AND python_path_inner)
        _qt_internal_sbom_find_python_dependency_helper(
            ${arg_DEPENDENCY_ARGS}
            PYTHON_PATH "${python_path_inner}"
            OUT_VAR_FOUND dep_found_inner
            OUT_VAR_OUTPUT dep_find_output_inner
        )

        if(dep_found_inner)
            set(everything_found_inner TRUE)
        endif()
    endif()

    set(${arg_OUT_VAR_PYTHON_PATH} "${python_path_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_FOUND} "${python_found_inner}" PARENT_SCOPE)
    if(arg_OUT_VAR_PYTHON_VERSION)
        set(${arg_OUT_VAR_PYTHON_VERSION} "${python_version_inner}" PARENT_SCOPE)
    endif()
    set(${arg_OUT_VAR_DEP_FOUND} "${dep_found_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_AND_DEP_FOUND} "${everything_found_inner}" PARENT_SCOPE)
    set(${arg_OUT_VAR_DEP_FIND_OUTPUT} "${dep_find_output_inner}" PARENT_SCOPE)
endfunction()

# Tries to find the python intrepreter, given the QT_SBOM_PYTHON_INTERP path hint, as well as
# other options.
# Ignores any previously found python.
# Returns the python interpreter path and whether it was successfully found, along with the version
# found.
#
# This is intentionally a function, and not a macro, to prevent overriding the Python3_EXECUTABLE
# non-cache variable in a global scope in case if a different python is found and used for a
# different purpose (e.g. qtwebengine or qtinterfaceframework).
# The reason to use a different python is that an already found python might not be the version we
# need, or might lack the dependencies we need.
# https://gitlab.kitware.com/cmake/cmake/-/issues/21797#note_901621 claims that finding multiple
# python versions in separate directory scopes is possible, and I claim a function scope is as
# good as a directory scope.
function(_qt_internal_sbom_find_python_helper)
    set(opt_args
        SEARCH_IN_FRAMEWORKS
        QUIET
    )
    set(single_args
        VERSION
        OUT_VAR_PYTHON_PATH
        OUT_VAR_PYTHON_FOUND
        OUT_VAR_PYTHON_VERSION
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_OUT_VAR_PYTHON_PATH)
        message(FATAL_ERROR "OUT_VAR_PYTHON_PATH var is required")
    endif()

    if(NOT arg_OUT_VAR_PYTHON_FOUND)
        message(FATAL_ERROR "OUT_VAR_PYTHON_FOUND var is required")
    endif()

    # Allow disabling looking for a python interpreter shipped as part of a macOS system framework.
    if(NOT arg_SEARCH_IN_FRAMEWORKS)
        set(Python3_FIND_FRAMEWORK NEVER)
    endif()

    set(required_version "")
    if(arg_VERSION)
        set(required_version "${arg_VERSION}")
    endif()

    set(find_quiet "")
    if(arg_QUIET)
        set(find_quiet "QUIET")
    endif()

    # Locally reset any executable that was possibly already found.
    # We do this to ensure we always re-do the lookup/
    # This needs to be set to an empty string, to override any cache variable
    set(Python3_EXECUTABLE "")

    # This needs to be unset, because the Python module checks whether the variable is defined, not
    # whether it is empty.
    unset(_Python3_EXECUTABLE)

    if(QT_SBOM_PYTHON_INTERP)
        set(Python3_ROOT_DIR ${QT_SBOM_PYTHON_INTERP})
    endif()

    find_package(Python3 ${required_version} ${find_quiet} COMPONENTS Interpreter)

    set(${arg_OUT_VAR_PYTHON_PATH} "${Python3_EXECUTABLE}" PARENT_SCOPE)
    set(${arg_OUT_VAR_PYTHON_FOUND} "${Python3_Interpreter_FOUND}" PARENT_SCOPE)
    if(arg_OUT_VAR_PYTHON_VERSION)
        set(${arg_OUT_VAR_PYTHON_VERSION} "${Python3_VERSION}" PARENT_SCOPE)
    endif()
endfunction()

# Helper that takes an python import statement to run using the given python interpreter path,
# to confirm that the given python dependency can be found.
# Returns whether the dependency was found and the output of running the import, for error handling.
function(_qt_internal_sbom_find_python_dependency_helper)
    set(opt_args "")
    set(single_args
        DEPENDENCY_IMPORT_STATEMENT
        PYTHON_PATH
        OUT_VAR_FOUND
        OUT_VAR_OUTPUT
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PYTHON_PATH)
        message(FATAL_ERROR "Python interpreter path not given.")
    endif()

    if(NOT arg_DEPENDENCY_IMPORT_STATEMENT)
        message(FATAL_ERROR "Python depdendency import statement not given.")
    endif()

    if(NOT arg_OUT_VAR_FOUND)
        message(FATAL_ERROR "Out var found variable not given.")
    endif()

    set(python_path "${arg_PYTHON_PATH}")
    execute_process(
        COMMAND
            "${python_path}" -c "${arg_DEPENDENCY_IMPORT_STATEMENT}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE output
        ERROR_VARIABLE output
    )

    if("${res}" STREQUAL "0")
        set(found TRUE)
        set(output "${output}")
    else()
        set(found FALSE)
        string(CONCAT output "SBOM Python dependency ${arg_DEPENDENCY_IMPORT_STATEMENT} NOT found. "
            "Error:\n${output}")
    endif()

    set(${arg_OUT_VAR_FOUND} "${found}" PARENT_SCOPE)
    if(arg_OUT_VAR_OUTPUT)
        set(${arg_OUT_VAR_OUTPUT} "${output}" PARENT_SCOPE)
    endif()
endfunction()

# Helper to find a python installed CLI utility.
# Expected to be in PATH.
function(_qt_internal_sbom_find_python_dependency_program)
    set(opt_args
        REQUIRED
    )
    set(single_args
        NAME
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(program_name "${arg_NAME}")
    string(TOUPPER "${program_name}" upper_name)
    set(cache_var "QT_SBOM_PROGRAM_${upper_name}")

    set(hints "")

    # The path to python installed apps is different on Windows compared to UNIX, so we use
    # a different path than where the python interpreter might be located.
    if(QT_SBOM_PYTHON_APPS_PATH)
        list(APPEND hints ${QT_SBOM_PYTHON_APPS_PATH})
    endif()

    # Sometimes the installed apps might be in the same location as the interepreter, if both are
    # in a virtual env. So look also in the interpreter path.
    if(QT_SBOM_PYTHON_INTERP)
        list(APPEND hints ${QT_SBOM_PYTHON_INTERP})
    endif()

    find_program(${cache_var}
        NAMES ${program_name}
        HINTS ${hints}
    )

    if(NOT ${cache_var})
        if(arg_REQUIRED)
            set(message_type "FATAL_ERROR")
            set(prefix "Required ")
        else()
            set(message_type "STATUS")
            set(prefix "Optional ")
        endif()
        message(${message_type} "${prefix}SBOM python program '${program_name}' NOT found.")
    endif()
endfunction()
