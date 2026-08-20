cmake_minimum_required(VERSION 3.25)

foreach(_required CMAKE_EXECUTABLE SOURCE_DIR BINARY_DIR EXPECTED_REGEX)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

set(_configure_command "${CMAKE_EXECUTABLE}" --fresh -S "${SOURCE_DIR}" -B "${BINARY_DIR}")
if(DEFINED QGC_MODULE_DIR)
    list(APPEND _configure_command "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}")
endif()
if(DEFINED TEST_CASE)
    list(APPEND _configure_command "-DTEST_CASE=${TEST_CASE}")
endif()
if(DEFINED CONFIGURE_ARGUMENT)
    list(APPEND _configure_command "${CONFIGURE_ARGUMENT}")
endif()

execute_process(
    COMMAND ${_configure_command}
    RESULT_VARIABLE _configure_result
    OUTPUT_VARIABLE _configure_stdout
    ERROR_VARIABLE _configure_stderr
)
set(_configure_output "${_configure_stdout}\n${_configure_stderr}")
string(REGEX REPLACE "[ \t\r\n]+" " " _normalized_configure_output "${_configure_output}")

if(_configure_result EQUAL 0)
    message(FATAL_ERROR "Configure unexpectedly succeeded; expected: ${EXPECTED_REGEX}")
endif()
if(NOT _normalized_configure_output MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR
        "Configure failed for an unexpected reason.\n"
        "Expected diagnostic: ${EXPECTED_REGEX}\n"
        "Actual output:\n${_configure_output}"
    )
endif()
