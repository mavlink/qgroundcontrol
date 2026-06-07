cmake_minimum_required(VERSION 3.16)

if(EXISTS "${PRIVATE_CONTENT_FILE}")
    file(READ "${PRIVATE_CONTENT_FILE}" PRIVATE_CONTENT)
endif()

if(NOT EXISTS "${IN_FILE}")
    message(FATAL_ERROR "Input file ${IN_FILE} doesn't exists")
endif()

if(OUT_FILE STREQUAL "")
    message(FATAL_ERROR "Output file is not specified")
endif()

configure_file("${IN_FILE}" "${OUT_FILE}" @ONLY)
