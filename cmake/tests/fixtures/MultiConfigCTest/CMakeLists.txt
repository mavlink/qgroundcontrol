cmake_minimum_required(VERSION 3.25)

project(QGCMultiConfigCTest LANGUAGES NONE)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()

add_custom_target(${PROJECT_NAME} COMMENT "Fixture target for QGCTest dependencies")
set(QGC_BUILD_TESTING ON)
include(CTest)
include("${QGC_MODULE_DIR}/QGCTest.cmake")

add_test(NAME PassingTest COMMAND "${CMAKE_COMMAND}" -E true)
set_tests_properties(PassingTest PROPERTIES LABELS Unit)
