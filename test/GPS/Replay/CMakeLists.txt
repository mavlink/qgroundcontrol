cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSReplay LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test QmlIntegration)
endif()

include(ParserTargets.cmake)
qgc_add_gps_parser_runtime(QGCGPSReplayRuntime)
qgc_add_gps_replay_driver(QGCGPSReplayDriverRuntime QGCGPSReplayRuntime)
if(NOT TARGET QGCJsonValidation)
    add_subdirectory("${QGC_GPS_REPOSITORY}/src/Utilities/Parsing/Json" JsonValidation)
endif()
add_executable(
    QGCGPSReplayTest
    GPSReplayTest.cc
    GPSReplayTransport.cc
    GPSReplayTransport.h
    GPSReplayDriver.cc
    GPSReplayDriver.h
    GPSReplayDefinitions.h
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingBuffer.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingBuffer.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingFormat.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingFormat.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingTransport.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingTransport.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingDevice.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingDevice.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingController.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingController.h"
)
set_target_properties(QGCGPSReplayTest PROPERTIES AUTOMOC ON)
target_compile_definitions(QGCGPSReplayTest PRIVATE QGC_GPS_REPLAY_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_include_directories(QGCGPSReplayTest
                           PRIVATE "${QGC_GPS_PARSER_SOURCE}/Driver" "${QGC_GPS_PARSER_SOURCE}/Driver/PX4"
                                   "${QGC_GPS_PARSER_SOURCE}/Recording" "${QGC_GPS_PARSER_SOURCE}/Receiver"
)
target_link_libraries(QGCGPSReplayTest PRIVATE QGCGPSReplayDriverRuntime QGCJsonValidation Qt6::Test
                                               Qt6::QmlIntegration
)
add_test(NAME GPSReplayTest COMMAND QGCGPSReplayTest)
set_tests_properties(GPSReplayTest PROPERTIES LABELS "Unit;GPS;Replay" TIMEOUT 60)
