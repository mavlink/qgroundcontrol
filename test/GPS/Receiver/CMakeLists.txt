cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCReceiverConfigTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Test)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS/Receiver" Receiver EXCLUDE_FROM_ALL)
    qt_add_executable(QGCGPSReceiverConfigTest GPSReceiverConfigTest.cc ../../Portable/PortableTest.h)
    set_target_properties(QGCGPSReceiverConfigTest PROPERTIES AUTOMOC ON)
    target_compile_features(QGCGPSReceiverConfigTest PRIVATE cxx_std_20)
    target_compile_definitions(QGCGPSReceiverConfigTest PRIVATE QGC_PORTABLE_TEST)
    target_include_directories(QGCGPSReceiverConfigTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../Portable")
    target_link_libraries(QGCGPSReceiverConfigTest PRIVATE Qt6::Test QGCGPSReceiverConfig)
    add_test(NAME GPSReceiverConfigTest COMMAND QGCGPSReceiverConfigTest)
    set_tests_properties(GPSReceiverConfigTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60 ENVIRONMENT
                                                          "QT_FATAL_WARNINGS=1"
    )
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverConfigTest.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSReceiverConfig)

add_qgc_test(GPSReceiverConfigTest LABELS Unit GPS)
