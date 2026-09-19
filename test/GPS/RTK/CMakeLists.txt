cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCBaseStationConfigTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Test)
    set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")
    add_subdirectory("${_gps_root}/Receiver" Receiver EXCLUDE_FROM_ALL)
    qt_add_executable(QGCGPSBaseStationConfigTest GPSBaseStationConfigTest.cc
                      "${_gps_root}/Driver/GPSReceiverConfigValidation.cc" ../../Portable/PortableTest.h
    )
    set_target_properties(QGCGPSBaseStationConfigTest PROPERTIES AUTOMOC ON)
    target_compile_features(QGCGPSBaseStationConfigTest PRIVATE cxx_std_20)
    target_compile_definitions(QGCGPSBaseStationConfigTest PRIVATE QGC_PORTABLE_TEST)
    target_include_directories(QGCGPSBaseStationConfigTest PRIVATE "${_gps_root}/Driver"
                                                                   "${CMAKE_CURRENT_SOURCE_DIR}/../../Portable"
    )
    target_link_libraries(QGCGPSBaseStationConfigTest PRIVATE Qt6::Test QGCGPSReceiverConfig)
    add_test(NAME GPSBaseStationConfigTest COMMAND QGCGPSBaseStationConfigTest)
    set_tests_properties(GPSBaseStationConfigTest PROPERTIES LABELS "Unit;GPS;RTK" TIMEOUT 60 ENVIRONMENT
                                                             "QT_FATAL_WARNINGS=1"
    )
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSBaseStationConfigTest.cc GPSProviderTest.cc GPSProviderTest.h
                                             GPSRtkTest.cc GPSRtkTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GPSProviderTest LABELS Unit)
add_qgc_test(GPSBaseStationConfigTest LABELS Unit GPS RTK)
add_qgc_test(GPSRtkTest LABELS Unit RESOURCE_LOCK Settings)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKAutoConnectTest.cc RTKAutoConnectTest.h)
    add_qgc_test(RTKAutoConnectTest LABELS Unit RESOURCE_LOCK Settings)
endif()
