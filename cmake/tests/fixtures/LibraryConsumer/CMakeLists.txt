cmake_minimum_required(VERSION 3.25)
project(LibraryConsumerFixture LANGUAGES CXX)

include("${QGC_MODULE_DIR}/modules/TestSupport.cmake")
enable_testing()
add_library(BoundaryLibrary INTERFACE)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Boundary.h" "#pragma once\n")
target_sources(BoundaryLibrary INTERFACE FILE_SET HEADERS BASE_DIRS "${CMAKE_CURRENT_BINARY_DIR}" FILES
                                         "${CMAKE_CURRENT_BINARY_DIR}/Boundary.h"
)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
get_filename_component(_source "${CMAKE_CURRENT_SOURCE_DIR}/../ModuleContracts/main.cpp" ABSOLUTE)
set(_labels Unit)
set(_timeout 30)
if(TEST_CASE STREQUAL "library-consumer-defaults")
    qgc_check_library_consumer(BoundaryLibrary SOURCE "${_source}")
elseif(TEST_CASE STREQUAL "library-consumer-metadata")
    set(_labels "Unit;GPS")
    set(_timeout 10)
    qgc_check_library_consumer(
        BoundaryLibrary
        SOURCE "${_source}"
        LABELS Unit GPS
        TIMEOUT 10
    )
elseif(TEST_CASE STREQUAL "library-consumer-missing-source")
    qgc_check_library_consumer(BoundaryLibrary)
elseif(TEST_CASE STREQUAL "library-consumer-missing-value")
    cmake_language(CALL qgc_check_library_consumer BoundaryLibrary SOURCE)
elseif(TEST_CASE STREQUAL "library-consumer-unknown-argument")
    qgc_check_library_consumer(BoundaryLibrary SOURCE "${_source}" UNKNOWN_OPTION)
elseif(TEST_CASE STREQUAL "library-consumer-missing-target")
    qgc_check_library_consumer(MissingLibrary SOURCE "${_source}")
elseif(TEST_CASE STREQUAL "library-consumer-invalid-timeout")
    qgc_check_library_consumer(
        BoundaryLibrary
        SOURCE "${_source}"
        TIMEOUT 0
    )
else()
    message(FATAL_ERROR "Unknown library consumer case: ${TEST_CASE}")
endif()
get_target_property(_verify BoundaryLibrary VERIFY_INTERFACE_HEADER_SETS)
if(NOT _verify OR NOT TARGET BoundaryLibraryHeaderChecks)
    message(FATAL_ERROR "Public header verification is missing")
endif()
foreach(_property AUTOMOC AUTOUIC AUTORCC)
    get_target_property(_enabled BoundaryLibraryConsumer ${_property})
    if(_enabled)
        message(FATAL_ERROR "${_property} must be disabled for boundary consumers")
    endif()
endforeach()
get_target_property(_links BoundaryLibraryConsumer LINK_LIBRARIES)
get_target_property(_sources BoundaryLibraryConsumer SOURCES)
get_test_property(BoundaryLibraryConsumer LABELS _actual_labels)
get_test_property(BoundaryLibraryConsumer TIMEOUT _actual_timeout)
if(NOT _links STREQUAL "BoundaryLibrary" OR NOT _sources STREQUAL "${_source}")
    message(FATAL_ERROR "Consumer must use the supplied source and only its owning library")
endif()
if(NOT _actual_labels STREQUAL "${_labels}" OR NOT _actual_timeout STREQUAL "${_timeout}")
    message(FATAL_ERROR "Consumer test metadata differs")
endif()
