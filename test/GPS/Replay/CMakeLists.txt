cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSReplay LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning Test QmlIntegration Concurrent)
endif()

include(ParserTargets.cmake)
qgc_add_gps_parser_runtime(QGCGPSReplayRuntime)
qgc_add_gps_replay_driver(QGCGPSReplayDriverRuntime QGCGPSReplayRuntime)
add_executable(
    QGCGPSReplayTest
    GPSReplayTest.cc
    GPSReplayTransport.cc
    GPSReplayTransport.h
    GPSReplayDriver.cc
    GPSReplayDriver.h
    GPSReplayDefinitions.h
    GPSReplayScheduler.cc
    GPSReplayScheduler.h
    GPSReplayDevice.cc
    GPSReplayDevice.h
    GPSReplayLifecycle.cc
    GPSReplayLifecycle.h
)
set_target_properties(QGCGPSReplayTest PROPERTIES AUTOMOC ON)
target_compile_definitions(QGCGPSReplayTest PRIVATE QGC_GPS_REPLAY_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_link_libraries(QGCGPSReplayTest PRIVATE QGCGPSReplayDriverRuntime QGCJsonValidation Qt6::Test
                                               Qt6::QmlIntegration Qt6::Concurrent
)
add_test(NAME GPSReplayTest COMMAND QGCGPSReplayTest)
set_tests_properties(GPSReplayTest PROPERTIES LABELS "Unit;GPS;Replay" TIMEOUT 60)
