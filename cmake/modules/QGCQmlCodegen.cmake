include_guard(GLOBAL)

function(qgc_add_qml_codegen target)
    if(NOT target)
        message(FATAL_ERROR "qgc_add_qml_codegen: target name is required")
    endif()
    if(NOT target MATCHES "^[A-Za-z0-9_][A-Za-z0-9_.+-]*$")
        message(FATAL_ERROR "qgc_add_qml_codegen: invalid target name '${target}'")
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 ARG
        "GENERATE_AT_CONFIGURE"
        "GENERATOR_MODULE;OUTPUT_DIR;COMMENT"
        "QML_NAMES;EXTRA_ARGS;DEPENDS")

    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "qgc_add_qml_codegen(${target}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    foreach(_required GENERATOR_MODULE OUTPUT_DIR QML_NAMES)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_add_qml_codegen(${target}): ${_required} is required")
        endif()
    endforeach()
    if(TARGET ${target})
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): target already exists")
    endif()
    if(NOT TARGET Python3::Interpreter)
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): Python3::Interpreter target is required")
    endif()
    if(NOT ARG_GENERATOR_MODULE MATCHES
       "^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$"
    )
        message(FATAL_ERROR
            "qgc_add_qml_codegen(${target}): invalid Python module '${ARG_GENERATOR_MODULE}'")
    endif()
    if(NOT IS_ABSOLUTE "${ARG_OUTPUT_DIR}")
        message(FATAL_ERROR "qgc_add_qml_codegen(${target}): OUTPUT_DIR must be absolute")
    endif()
    cmake_path(NORMAL_PATH ARG_OUTPUT_DIR OUTPUT_VARIABLE _output_dir)
    cmake_path(IS_PREFIX CMAKE_BINARY_DIR "${_output_dir}" NORMALIZE _output_in_build_tree)
    if(NOT _output_in_build_tree)
        message(FATAL_ERROR
            "qgc_add_qml_codegen(${target}): OUTPUT_DIR must stay under CMAKE_BINARY_DIR")
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

    list(TRANSFORM ARG_QML_NAMES PREPEND "${_output_dir}/" OUTPUT_VARIABLE _outputs)

    if(ARG_GENERATE_AT_CONFIGURE)
        if(NOT Python3_EXECUTABLE OR NOT EXISTS "${Python3_EXECUTABLE}")
            message(FATAL_ERROR
                "qgc_add_qml_codegen(${target}): GENERATE_AT_CONFIGURE requires Python3_EXECUTABLE")
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARG_DEPENDS})
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" -m "${ARG_GENERATOR_MODULE}"
                    --output-dir "${_output_dir}" ${ARG_EXTRA_ARGS}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _generate_result
            OUTPUT_VARIABLE _generate_output
            ERROR_VARIABLE _generate_output
        )
        if(NOT _generate_result EQUAL 0)
            message(FATAL_ERROR
                "qgc_add_qml_codegen(${target}): configure-time generation failed "
                "(exit ${_generate_result}):\n${_generate_output}")
        endif()
    endif()

    add_custom_command(
        OUTPUT  ${_outputs}
        COMMAND Python3::Interpreter -m ${ARG_GENERATOR_MODULE}
                --output-dir "${_output_dir}" ${ARG_EXTRA_ARGS}
        DEPENDS ${ARG_DEPENDS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "${ARG_COMMENT}"
        VERBATIM
    )

    add_custom_target(${target} DEPENDS ${_outputs})

    set_source_files_properties(${_outputs} PROPERTIES GENERATED TRUE)
    foreach(_name IN LISTS ARG_QML_NAMES)
        set_source_files_properties("${_output_dir}/${_name}"
            PROPERTIES QT_RESOURCE_ALIAS "${_name}")
    endforeach()

    set(${target}_OUTPUTS ${_outputs} PARENT_SCOPE)
endfunction()
