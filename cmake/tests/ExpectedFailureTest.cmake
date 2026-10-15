# Registration of CMake tests that must fail with a given diagnostic. Included by cmake/tests/CMakeLists.txt and
# cmake/GStreamer/tests/CMakeLists.txt, which the top-level CMakeLists.txt adds first.

include_guard(GLOBAL)

# qgc_add_expected_failure_test(NAME <name> EXPECTED_REGEX <regex> LABELS <label>... [TIMEOUT <seconds>]
#                               ARGS <-D<var>=<value>>...)
# Adds test <name>, which runs run_expected_failure_test.cmake with ARGS (SCRIPT_FILE [TEST_CASE] to run a script, or
# SOURCE_DIR BINARY_DIR [QGC_MODULE_DIR TEST_CASE CONFIGURE_ARGUMENT] to configure a project) and passes only when that
# fails with a diagnostic matching <regex>, so unrelated errors cannot satisfy it.
function(qgc_add_expected_failure_test)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "NAME;EXPECTED_REGEX;TIMEOUT" "LABELS;ARGS")
    if(ARG_UNPARSED_ARGUMENTS OR ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_add_expected_failure_test: unknown arguments or missing values: "
                            "${ARG_UNPARSED_ARGUMENTS} ${ARG_KEYWORDS_MISSING_VALUES}"
        )
    endif()
    foreach(_required IN ITEMS NAME EXPECTED_REGEX LABELS ARGS)
        if(NOT ARG_${_required})
            message(FATAL_ERROR "qgc_add_expected_failure_test: ${_required} is required")
        endif()
    endforeach()
    add_test(
        NAME "${ARG_NAME}"
        COMMAND
            "${CMAKE_COMMAND}" "-DCMAKE_EXECUTABLE=${CMAKE_COMMAND}" ${ARG_ARGS}
            "-DEXPECTED_REGEX=${ARG_EXPECTED_REGEX}" -P
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/run_expected_failure_test.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_tests_properties("${ARG_NAME}" PROPERTIES LABELS "${ARG_LABELS}")
    if(ARG_TIMEOUT)
        set_tests_properties("${ARG_NAME}" PROPERTIES TIMEOUT ${ARG_TIMEOUT})
    endif()
endfunction()
