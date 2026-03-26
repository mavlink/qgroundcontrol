# GenerateSettingsQml.cmake
#
# Provides qgc_generate_settings_qml() for single-source generation and
# qgc_generate_all_settings_qml() for batch generation of all settings pages.
#
# Each *.SettingsGroup.json produces one QML page (1:1 mapping).
#
# Usage (single source):
#   include(GenerateSettingsQml)
#
#   qgc_generate_settings_qml(
#       JSON_FILE       src/Settings/FlyView.SettingsGroup.json
#       ACCESSOR        "QGroundControl.settingsManager.flyViewSettings"
#       PROPERTY        "_settings"
#       OUTPUT          "${CMAKE_CURRENT_SOURCE_DIR}/generated/FlyViewSettings.qml"
#   )
#
# Usage (batch — generates all pages):
#   qgc_generate_all_settings_qml(
#       OUTPUT_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/generated"
#   )

find_package(Python3 REQUIRED COMPONENTS Interpreter)

function(qgc_generate_settings_qml)
    cmake_parse_arguments(ARG "" "JSON_FILE;ACCESSOR;PROPERTY;OUTPUT" "" ${ARGN})

    if(NOT ARG_JSON_FILE)
        message(FATAL_ERROR "qgc_generate_settings_qml: JSON_FILE is required")
    endif()
    if(NOT ARG_ACCESSOR)
        message(FATAL_ERROR "qgc_generate_settings_qml: ACCESSOR is required")
    endif()
    if(NOT ARG_OUTPUT)
        message(FATAL_ERROR "qgc_generate_settings_qml: OUTPUT is required")
    endif()

    # Resolve paths relative to the source dir
    if(NOT IS_ABSOLUTE "${ARG_JSON_FILE}")
        set(ARG_JSON_FILE "${CMAKE_SOURCE_DIR}/${ARG_JSON_FILE}")
    endif()

    set(_args
        "${Python3_EXECUTABLE}" -m tools.generators.settings_qml.cli
        --json "${ARG_JSON_FILE}"
        --settings-accessor "${ARG_ACCESSOR}"
        --output "${ARG_OUTPUT}"
    )

    if(ARG_PROPERTY)
        list(APPEND _args --settings-property "${ARG_PROPERTY}")
    endif()

    # Generate at configure time — JSON metadata changes require re-running cmake
    execute_process(
        COMMAND ${_args}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE  _error
    )

    if(NOT _result EQUAL 0)
        message(FATAL_ERROR
            "qgc_generate_settings_qml failed for ${ARG_JSON_FILE}:\n${_error}")
    endif()

    message(STATUS "Generated settings QML: ${ARG_OUTPUT}")

    # Mark the JSON as a configure dependency so cmake re-runs when it changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ARG_JSON_FILE}")
endfunction()


function(qgc_generate_all_settings_qml)
    cmake_parse_arguments(ARG "" "OUTPUT_DIR" "" ${ARGN})

    if(NOT ARG_OUTPUT_DIR)
        message(FATAL_ERROR "qgc_generate_all_settings_qml: OUTPUT_DIR is required")
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m tools.generators.settings_qml.generate_all
                --output-dir "${ARG_OUTPUT_DIR}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE  _error
    )

    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "qgc_generate_all_settings_qml failed:\n${_error}")
    endif()

    message(STATUS "Generated all settings QML pages to: ${ARG_OUTPUT_DIR}")

    # Mark all JSON files as configure dependencies
    file(GLOB _json_files "${CMAKE_SOURCE_DIR}/src/Settings/*.SettingsGroup.json")
    foreach(_json ${_json_files})
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_json}")
    endforeach()

    # Export the output directory to parent scope
    set(QGC_GENERATED_SETTINGS_DIR "${ARG_OUTPUT_DIR}" PARENT_SCOPE)
endfunction()
