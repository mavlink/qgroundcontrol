cmake_minimum_required(VERSION 3.25)

set(_gps_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/GPS")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSPx4DataTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Test)
    add_subdirectory("${_gps_root}/Receiver" Receiver EXCLUDE_FROM_ALL)
    add_subdirectory("${_gps_root}/Driver/Px4" Px4Adapter)

    qt_add_executable(QGCGPSPx4DataTest GPSPx4DataTest.cc ../../Portable/PortableTest.h)
    set_target_properties(QGCGPSPx4DataTest PROPERTIES AUTOMOC ON)
    target_compile_features(QGCGPSPx4DataTest PRIVATE cxx_std_20)
    target_compile_definitions(QGCGPSPx4DataTest PRIVATE QGC_PORTABLE_TEST)
    target_include_directories(QGCGPSPx4DataTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../Portable"
                                                         "${_gps_root}/Driver/Px4" "${_gps_root}/Receiver"
    )
    target_link_libraries(QGCGPSPx4DataTest PRIVATE Qt6::Test QGCGPSPx4Adapter QGCGPSDriverReports)
    add_test(NAME GPSPx4DataTest COMMAND QGCGPSPx4DataTest)
    set_tests_properties(GPSPx4DataTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60 ENVIRONMENT "QT_FATAL_WARNINGS=1")
    return()
endif()

# Native driver and PX4 callback-bridge tests.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSDriverTest.cc GPSDriverTest.h GPSLegacySafetyTest.cc
                                             GPSLegacySafetyTest.h ScriptedUBXReceiver.cc ScriptedUBXReceiver.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Only the callback-bridge and legacy-safety tests need PX4 implementation headers and definitions.
set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverTest.cc ${CMAKE_CURRENT_SOURCE_DIR}/GPSLegacySafetyTest.cc TARGET_DIRECTORY
    ${CMAKE_PROJECT_NAME}
    PROPERTIES INCLUDE_DIRECTORIES "$<TARGET_PROPERTY:px4-gpsdrivers,INCLUDE_DIRECTORIES>"
               COMPILE_DEFINITIONS "$<TARGET_PROPERTY:px4-gpsdrivers,COMPILE_DEFINITIONS>"
               SKIP_PRECOMPILE_HEADERS ON
)

add_qgc_test(GPSDriverTest LABELS Unit)
add_qgc_test(GPSLegacySafetyTest LABELS Unit GPS)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSPx4DataTest.cc)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSPx4Adapter QGCGPSDriverReports)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/GPSPx4DataTest.cc TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
                            PROPERTIES INCLUDE_DIRECTORIES "${_gps_root}/Driver/Px4;${_gps_root}/Receiver"
                                       SKIP_PRECOMPILE_HEADERS ON
)
add_qgc_test(GPSPx4DataTest LABELS Unit GPS)
