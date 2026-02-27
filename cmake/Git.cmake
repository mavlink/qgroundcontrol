find_package(Git)

# Initialize default values
set(QGC_GIT_BRANCH "unknown")
set(QGC_GIT_HASH "unknown") 
set(QGC_APP_VERSION_STR "v5.0.8")
set(QGC_APP_VERSION "5.0.8")
set(QGC_APP_DATE "unknown")

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

    include(CMakePrintHelpers)

    # Get git branch with error handling
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_BRANCH_RESULT
        OUTPUT_VARIABLE QGC_GIT_BRANCH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_BRANCH_RESULT EQUAL 0)
        set(QGC_GIT_BRANCH "unknown")
    endif()

    # Get git hash with error handling
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_HASH_RESULT
        OUTPUT_VARIABLE QGC_GIT_HASH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_HASH_RESULT EQUAL 0)
        set(QGC_GIT_HASH "unknown")
    endif()

    # Get version string with error handling
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --tags
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DESCRIBE_RESULT
        OUTPUT_VARIABLE QGC_APP_VERSION_STR
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_DESCRIBE_RESULT EQUAL 0)
        set(QGC_APP_VERSION_STR "v5.0.8")
    endif()

    # Get version tag with error handling
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_VERSION_RESULT
        OUTPUT_VARIABLE QGC_APP_VERSION
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_VERSION_RESULT EQUAL 0)
        set(QGC_APP_VERSION "v5.0.8")
    endif()

    # Get commit date with error handling
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --format=%aI HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_DATE_RESULT
        OUTPUT_VARIABLE QGC_APP_DATE
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT GIT_DATE_RESULT EQUAL 0)
        set(QGC_APP_DATE "unknown")
    endif()

else()
    # Not a git repository, use hardcoded values
    message(STATUS "Not a git repository, using hardcoded version 5.0.7")
    set(QGC_GIT_BRANCH "release")
    set(QGC_GIT_HASH "unknown")
    set(QGC_APP_VERSION_STR "v5.0.8")
    set(QGC_APP_VERSION "v5.0.8")
    set(QGC_APP_DATE "2025-01-01")
endif()

# Safe version parsing
string(FIND "${QGC_APP_VERSION}" "v" QGC_APP_VERSION_VALID)
if(QGC_APP_VERSION_VALID GREATER -1)
    string(REPLACE "v" "" QGC_APP_VERSION ${QGC_APP_VERSION})
else()
    set(QGC_APP_VERSION "5.0.8")
endif()

# Safe regex matching
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" QGC_APP_VERSION_MATCH "${QGC_APP_VERSION}")
if(QGC_APP_VERSION_MATCH)
    set(QGC_APP_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(QGC_APP_VERSION_MINOR ${CMAKE_MATCH_2})
    set(QGC_APP_VERSION_PATCH ${CMAKE_MATCH_3})
else()
    # Fallback if regex fails
    set(QGC_APP_VERSION_MAJOR "5")
    set(QGC_APP_VERSION_MINOR "0")
    set(QGC_APP_VERSION_PATCH "8")
endif()

message(STATUS "QGC Version: ${QGC_APP_VERSION_MAJOR}.${QGC_APP_VERSION_MINOR}.${QGC_APP_VERSION_PATCH}")
message(STATUS "Git Branch: ${QGC_GIT_BRANCH}")
message(STATUS "Git Hash: ${QGC_GIT_HASH}")