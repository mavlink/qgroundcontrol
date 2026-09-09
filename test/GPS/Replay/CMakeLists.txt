cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSReplay LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Positioning Test QmlIntegration)
endif()

include(ParserTargets.cmake)
qgc_add_gps_parser_runtime(QGCGPSReplayRuntime)
add_executable(
    QGCGPSReplayTest
    GPSReplayTest.cc
    GPSReplayTransport.cc
    GPSReplayTransport.h
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingBuffer.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingBuffer.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingTransport.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingTransport.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingDevice.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingDevice.h"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingController.cc"
    "${QGC_GPS_PARSER_SOURCE}/Recording/GPSRecordingController.h"
    "${QGC_GPS_PARSER_SOURCE}/Driver/GPSTransport.cc"
    "${QGC_GPS_PARSER_SOURCE}/Driver/PX4/ubx.cpp"
    "${QGC_GPS_PARSER_SOURCE}/Driver/PX4/gps_helper.cpp"
    "${QGC_GPS_PARSER_SOURCE}/Driver/PX4/rtcm.cpp"
)
set_target_properties(QGCGPSReplayTest PROPERTIES AUTOMOC ON)
target_compile_definitions(QGCGPSReplayTest
                           PRIVATE GPS_DEFINITIONS_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/../Driver/PX4/definitions.h"
                                   QGC_GPS_REPLAY_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
target_include_directories(QGCGPSReplayTest
                           PRIVATE "${QGC_GPS_PARSER_SOURCE}/Driver" "${QGC_GPS_PARSER_SOURCE}/Driver/PX4"
                                   "${QGC_GPS_PARSER_SOURCE}/Recording"
)
target_link_libraries(QGCGPSReplayTest PRIVATE QGCGPSReplayRuntime Qt6::Test Qt6::QmlIntegration)
add_test(NAME GPSReplayTest COMMAND QGCGPSReplayTest)
set_tests_properties(GPSReplayTest PROPERTIES LABELS "Unit;GPS;Replay" TIMEOUT 60)
