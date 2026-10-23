include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# qgc_add_qml_codegen()
# ----------------------------------------------------------------------------
# Generates QML files with a Python generator module and adds a custom target for them.
#
# Arguments:
#   target                - Name of the custom target to create
#   GENERATOR_MODULE mod  - Python module run with `python -m` from the source root
#   OUTPUT_DIR dir        - Absolute output directory under CMAKE_BINARY_DIR
#   QML_NAMES name...     - Generated files, relative to OUTPUT_DIR
#   LIST_OUTPUTS          - Instead of QML_NAMES, take the names the generator prints for --list-outputs (with
#                           EXTRA_ARGS) at configure
#   EXTRA_ARGS arg...     - Extra generator arguments
#   DEPENDS file...       - Generator inputs; with GENERATE_AT_CONFIGURE or LIST_OUTPUTS they also re-run configure
#   BUILD_DEPENDS file... - Inputs that only re-run the generator at build time
#   COMMENT text          - Build-time comment
#   GENERATE_AT_CONFIGURE - Also generate during configure, so the outputs exist before qt_add_qml_module
#
# A tools.generators.<package> module also depends on that package's modules and templates and on the shared
# tools modules it imports.
#
# Sets <target>_OUTPUTS in the parent scope to the absolute generated paths.
function(qgc_add_qml_codegen target)
    # Argument validation keeps this entry point long; <target>_OUTPUTS is the documented result
    # variable, and the custom target is only an aggregate of the generated outputs.
    # cmake-lint: disable=R0912,R0915,C0103,C0113
    if("${target}" STREQUAL "")
        message(FATAL_ERROR "qgc_add_qml_codegen: target name is required")
    endif()
    if(NOT "${target}" MATCHES "^[A-Za-z0-9_][A-Za-z0-9_.+-]*$")
        message(FATAL_ERROR "qgc_add_qml_codegen: invalid target name '${target}'")
    endif()

    cmake_parse_arguments(
        PARSE_ARGV 1 ARG "GENERATE_AT_CONFIGURE;LIST_OUTPUTS" "GENERATOR_MODULE;OUTPUT_DIR;COMMENT"
        "QML_NAMES;EXTRA_ARGS;DEPENDS;BUILD_DEPENDS"
    )

    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    foreach(_required GENERATOR_MODULE OUTPUT_DIR)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): ${_required} is required")
        endif()
    endforeach()
    if(ARG_LIST_OUTPUTS AND ARG_QML_NAMES)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): QML_NAMES and LIST_OUTPUTS are mutually exclusive")
    endif()
    if(NOT ARG_LIST_OUTPUTS AND NOT ARG_QML_NAMES)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): QML_NAMES is required unless LIST_OUTPUTS is set")
    endif()
    if(TARGET ${target})
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): target already exists")
    endif()
    if(NOT TARGET Python3::Interpreter)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): Python3::Interpreter target is required")
    endif()
    if(NOT ARG_GENERATOR_MODULE MATCHES "^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$")
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): invalid Python module '${ARG_GENERATOR_MODULE}'")
    endif()
    if(NOT IS_ABSOLUTE "${ARG_OUTPUT_DIR}")
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): OUTPUT_DIR must be absolute")
    endif()
    cmake_path(NORMAL_PATH ARG_OUTPUT_DIR OUTPUT_VARIABLE _output_dir)
    cmake_path(IS_PREFIX CMAKE_BINARY_DIR "${_output_dir}" NORMALIZE _output_in_build_tree)
    if(NOT _output_in_build_tree)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): OUTPUT_DIR must stay under CMAKE_BINARY_DIR")
    endif()

    if(ARG_LIST_OUTPUTS)
        if(NOT Python3_EXECUTABLE OR NOT EXISTS "${Python3_EXECUTABLE}")
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): LIST_OUTPUTS requires Python3_EXECUTABLE")
        endif()
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m "${ARG_GENERATOR_MODULE}" --list-outputs ${ARG_EXTRA_ARGS}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _list_result
            OUTPUT_VARIABLE _listed_names
            ERROR_VARIABLE _list_error
        )
        if(NOT _list_result EQUAL 0)
            message(
                FATAL_ERROR "qgc_add_qml_codegen(${target}): --list-outputs failed "
                            "(exit ${_list_result}):\n${_list_error}"
            )
        endif()
        # Python emits CRLF on Windows; execute_process does not normalize it
        string(REPLACE "\r" "" _listed_names "${_listed_names}")
        string(REPLACE "\n" ";" ARG_QML_NAMES "${_listed_names}")
        list(REMOVE_ITEM ARG_QML_NAMES "")
        if(NOT ARG_QML_NAMES)
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): --list-outputs listed no outputs:\n${_list_error}")
        endif()
    endif()

    if(ARG_GENERATOR_MODULE MATCHES "^tools\\.generators\\.([A-Za-z0-9_]+)\\.")
        set(_tools_dir "${CMAKE_SOURCE_DIR}/tools")
        set(_package_dir "${_tools_dir}/generators/${CMAKE_MATCH_1}")
        file(GLOB _generator_sources CONFIGURE_DEPENDS "${_package_dir}/*.py" "${_package_dir}/templates/*.j2"
             "${_tools_dir}/generators/common/*.py"
        )
        list(
            APPEND
            ARG_DEPENDS
            ${_generator_sources}
            "${_tools_dir}/_bootstrap.py"
            "${_tools_dir}/common/__init__.py"
            "${_tools_dir}/common/io.py"
            "${_tools_dir}/generators/__init__.py"
            "${_tools_dir}/generators/_jinja.py"
        )
    endif()

    set(_unique_qml_names ${ARG_QML_NAMES})
    list(REMOVE_DUPLICATES _unique_qml_names)
    list(LENGTH ARG_QML_NAMES _qml_name_count)
    list(LENGTH _unique_qml_names _unique_qml_name_count)
    if(NOT _qml_name_count EQUAL _unique_qml_name_count)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): QML_NAMES contains duplicate outputs")
    endif()
    foreach(_name IN LISTS ARG_QML_NAMES)
        if(IS_ABSOLUTE "${_name}" OR _name MATCHES "(^|[/\\\\])\\.\\.([/\\\\]|$)")
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): QML_NAMES must stay under OUTPUT_DIR: ${_name}")
        endif()
    endforeach()
    foreach(_dependency IN LISTS ARG_BUILD_DEPENDS)
        if(_dependency IN_LIST ARG_DEPENDS)
            message(
                FATAL_ERROR "qgc_add_qml_codegen(${target}): BUILD_DEPENDS entry is also in DEPENDS: ${_dependency}"
            )
        endif()
    endforeach()

    list(TRANSFORM ARG_QML_NAMES PREPEND "${_output_dir}/" OUTPUT_VARIABLE _outputs)

    # BUILD_DEPENDS stay out of this so editing them re-runs only the build-time command, not configure
    if(ARG_GENERATE_AT_CONFIGURE OR ARG_LIST_OUTPUTS)
        set_property(
            DIRECTORY
            APPEND
            PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARG_DEPENDS}
        )
    endif()

    if(ARG_GENERATE_AT_CONFIGURE)
        if(NOT Python3_EXECUTABLE OR NOT EXISTS "${Python3_EXECUTABLE}")
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): GENERATE_AT_CONFIGURE requires Python3_EXECUTABLE")
        endif()
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m "${ARG_GENERATOR_MODULE}" --output-dir "${_output_dir}" ${ARG_EXTRA_ARGS}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _generate_result
            OUTPUT_VARIABLE _generate_output
            ERROR_VARIABLE _generate_output
        )
        if(NOT _generate_result EQUAL 0)
            message(
                FATAL_ERROR "qgc_add_qml_codegen(${target}): configure-time generation failed "
                            "(exit ${_generate_result}):\n${_generate_output}"
            )
        endif()
        foreach(_output IN LISTS _outputs)
            if(NOT EXISTS "${_output}")
                message(
                    FATAL_ERROR "qgc_add_qml_codegen(${target}): configure-time generator did not produce "
                                "declared output '${_output}'"
                )
            endif()
        endforeach()
    endif()

    add_custom_command(
        OUTPUT ${_outputs}
        COMMAND Python3::Interpreter -m ${ARG_GENERATOR_MODULE} --output-dir "${_output_dir}" ${ARG_EXTRA_ARGS}
        DEPENDS ${ARG_DEPENDS} ${ARG_BUILD_DEPENDS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "${ARG_COMMENT}"
        VERBATIM
    )

    add_custom_target(${target} DEPENDS ${_outputs})

    set_source_files_properties(
        ${_outputs}
        PROPERTIES
            GENERATED TRUE
    )
    foreach(_name IN LISTS ARG_QML_NAMES)
        set_source_files_properties(
            "${_output_dir}/${_name}"
            PROPERTIES
                QT_RESOURCE_ALIAS "${_name}"
        )
    endforeach()

    set(${target}_OUTPUTS
        ${_outputs}
        PARENT_SCOPE
    )
endfunction()
