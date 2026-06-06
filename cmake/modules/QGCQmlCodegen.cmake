function(qgc_add_qml_codegen target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        "GENERATOR_MODULE;OUTPUT_DIR;COMMENT"
        "QML_NAMES;EXTRA_ARGS;DEPENDS")

    list(TRANSFORM ARG_QML_NAMES PREPEND "${ARG_OUTPUT_DIR}/" OUTPUT_VARIABLE _outputs)

    add_custom_command(
        OUTPUT  ${_outputs}
        COMMAND Python3::Interpreter -m ${ARG_GENERATOR_MODULE}
                --output-dir "${ARG_OUTPUT_DIR}" ${ARG_EXTRA_ARGS}
        DEPENDS ${ARG_DEPENDS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "${ARG_COMMENT}"
        VERBATIM
    )

    add_custom_target(${target} DEPENDS ${_outputs})

    set_source_files_properties(${_outputs} PROPERTIES GENERATED TRUE)
    foreach(_name IN LISTS ARG_QML_NAMES)
        set_source_files_properties("${ARG_OUTPUT_DIR}/${_name}"
            PROPERTIES QT_RESOURCE_ALIAS "${_name}")
    endforeach()

    set(${target}_OUTPUTS ${_outputs} PARENT_SCOPE)
endfunction()
