cmake_minimum_required(VERSION 3.25)

set(_required_arguments CMAKE_EXECUTABLE EXPECTED_REGEX)
if(DEFINED SCRIPT_FILE)
    if(DEFINED SOURCE_DIR OR DEFINED BINARY_DIR)
        message(FATAL_ERROR "SCRIPT_FILE cannot be combined with SOURCE_DIR or BINARY_DIR")
    endif()
    list(APPEND _required_arguments SCRIPT_FILE)
else()
    list(APPEND _required_arguments SOURCE_DIR BINARY_DIR)
endif()

foreach(_required IN LISTS _required_arguments)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

if(DEFINED SCRIPT_FILE)
    set(_command "${CMAKE_EXECUTABLE}")
    if(DEFINED TEST_CASE)
        list(APPEND _command "-DTEST_CASE=${TEST_CASE}")
    endif()
    list(APPEND _command -P "${SCRIPT_FILE}")
else()
    set(_command "${CMAKE_EXECUTABLE}" --fresh -S "${SOURCE_DIR}" -B "${BINARY_DIR}")
    if(DEFINED QGC_MODULE_DIR)
        list(APPEND _command "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}")
    endif()
    if(DEFINED TEST_CASE)
        list(APPEND _command "-DTEST_CASE=${TEST_CASE}")
    endif()
    if(DEFINED CONFIGURE_ARGUMENT)
        list(APPEND _command "${CONFIGURE_ARGUMENT}")
    endif()
endif()

execute_process(
    COMMAND ${_command}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
)
set(_output "${_stdout}\n${_stderr}")
string(REGEX REPLACE "[ \t\r\n]+" " " _normalized_output "${_output}")

if(_result EQUAL 0)
    message(FATAL_ERROR "Command unexpectedly succeeded; expected: ${EXPECTED_REGEX}")
endif()
if(NOT _normalized_output MATCHES "${EXPECTED_REGEX}")
    message(FATAL_ERROR
        "Command failed for an unexpected reason.\n"
        "Expected diagnostic: ${EXPECTED_REGEX}\n"
        "Actual output:\n${_output}"
    )
endif()
