cmake_minimum_required(VERSION 3.16)
# The PARAMETERS file should specify the following variables for the correct work of
# this script:
#   HEADER_CHECK_EXCEPTIONS - path to file that contains exceptions.
#       The file is created by syncqt.
#
#   HEADER_CHECK_COMPILER_COMMAND_LINE - compiler command line
include("${PARAMETERS}")

if(EXISTS ${HEADER_CHECK_EXCEPTIONS})
    file(READ ${HEADER_CHECK_EXCEPTIONS} header_check_exception_list)
endif()

get_filename_component(header "${INPUT_HEADER_FILE}" REALPATH)
file(TO_CMAKE_PATH "${header}" header)
foreach(exception IN LISTS header_check_exception_list)
    file(TO_CMAKE_PATH "${exception}" exception)
    if(exception STREQUAL header)
        file(WRITE "${OUTPUT_ARTIFACT}" "skipped")
        return()
    endif()
endforeach()

execute_process(COMMAND ${HEADER_CHECK_COMPILER_COMMAND_LINE}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "${INPUT_HEADER_FILE} header check"
        " failed: ${HEADER_CHECK_COMPILER_COMMAND_LINE}\n"
        " ${output}")
endif()
