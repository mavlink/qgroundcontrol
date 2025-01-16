find_package(Git)

if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" OFF)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
            OUTPUT_VARIABLE GIT_SUBMODULE_OUTPUT
            ERROR_VARIABLE GIT_SUBMODULE_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
            cmake_print_variables(GIT_SUBMODULE_RESULT GIT_SUBMODULE_OUTPUT GIT_SUBMODULE_ERROR)
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMODULE_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

include(CMakePrintHelpers)

execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref @
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QGC_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(QGC_GIT_BRANCH)

execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short @
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QGC_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(QGC_GIT_HASH)

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --tags
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QGC_APP_VERSION_STR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(QGC_APP_VERSION_STR)

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QGC_APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(QGC_APP_VERSION)

execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%aI ${QGC_APP_VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QGC_APP_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
cmake_print_variables(QGC_APP_DATE)

string(FIND ${QGC_APP_VERSION} "v" QGC_APP_VERSION_VALID)
if(QGC_APP_VERSION_VALID GREATER -1)
    string(REPLACE "v" "" QGC_APP_VERSION ${QGC_APP_VERSION})
else()
    set(QGC_APP_VERSION "0.0.0")
endif()
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" QGC_APP_VERSION_MATCH ${QGC_APP_VERSION})
set(QGC_APP_VERSION_MAJOR ${CMAKE_MATCH_1})
set(QGC_APP_VERSION_MINOR ${CMAKE_MATCH_2})
set(QGC_APP_VERSION_PATCH ${CMAKE_MATCH_3})
cmake_print_variables(QGC_APP_VERSION QGC_APP_VERSION_MAJOR QGC_APP_VERSION_MINOR QGC_APP_VERSION_PATCH)
