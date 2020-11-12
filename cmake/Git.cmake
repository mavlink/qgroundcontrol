find_package(Git)

if(NOT GIT_FOUND OR NOT EXISTS "${PROJECT_SOURCE_DIR}/.git")
    return()
endif()

# Update submodules as needed
option(GIT_SUBMODULE "Check submodules during build" ON)
if(NOT GIT_SUBMODULE)
    return()
endif()

message(STATUS "Submodule update")
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE GIT_SUBMODULE_RESULT)
if(NOT GIT_SUBMODULE_RESULT EQUAL "0")
    message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMODULE_RESULT}, please checkout submodules")
endif()

# Fetch the necessary git variables
execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --tags
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE)
