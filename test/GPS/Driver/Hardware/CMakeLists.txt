cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSHardwareTests LANGUAGES CXX)
    enable_testing()
    get_filename_component(QGC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
    list(APPEND CMAKE_MODULE_PATH "${QGC_SOURCE_ROOT}/cmake" "${QGC_SOURCE_ROOT}/cmake/modules")
    include(CPM)
    include(CompilerWarnings)
    option(QGC_NO_SERIAL_LINK "Build without serial receiver support" OFF)
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network)
    if(NOT QGC_NO_SERIAL_LINK)
        find_package(Qt6 6.8 REQUIRED COMPONENTS SerialPort)
    endif()
    add_subdirectory("${QGC_SOURCE_ROOT}/src/Utilities/Logging/Category" LoggingCategory)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Receiver" Receiver)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Transport" Transport)
    add_subdirectory("${QGC_SOURCE_ROOT}/src/GPS/Driver" Driver)
endif()

if(NOT TARGET QGCGPSDriver OR NOT TARGET QGCGPSReceiverTransports)
    message(FATAL_ERROR "GPS hardware runner requires QGCGPSDriver and QGCGPSReceiverTransports")
endif()

get_filename_component(_qgc_root "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${_qgc_root}/cmake/modules")
if(NOT TARGET QGCGPSLegacyDriver)
    if(NOT TARGET QGCGPSPx4Adapter)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../Legacy" LegacyPx4)
    endif()

    add_library(QGCGPSLegacyDriver STATIC GPSLegacyDriver.cc GPSLegacyDriver.h)
    target_compile_features(QGCGPSLegacyDriver PUBLIC cxx_std_20)
    target_include_directories(QGCGPSLegacyDriver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_include_directories(QGCGPSLegacyDriver PRIVATE "${_qgc_root}/src/GPS/Driver")
    target_link_libraries(
        QGCGPSLegacyDriver
        PUBLIC QGCGPSDriver
        PRIVATE QGCGPSTransport QGCLoggingCategory
    )
    qgc_add_px4_backend(QGCGPSLegacyDriver)
    # The SBF implementations still export this same free function.
    target_compile_definitions(px4-gpsdrivers PRIVATE crc16=GPSLegacyPX4Crc16)
endif()

add_executable(QGCGPSHardwareRunner GPSHardwareRunner.cc GPSEvidenceTransport.cc GPSEvidenceTransport.h
                                    "${CMAKE_CURRENT_SOURCE_DIR}/../ScriptedUBXReceiver.cc"
)
target_compile_features(QGCGPSHardwareRunner PRIVATE cxx_std_20)
target_include_directories(QGCGPSHardwareRunner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(QGCGPSHardwareRunner PRIVATE QGCGPSLegacyDriver QGCGPSReceiverTransports QGC::Clock)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME GPSHardwareRunner.Scripted COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/check_runner.py"
                                                 "$<TARGET_FILE:QGCGPSHardwareRunner>" "$<BOOL:${QGC_NO_SERIAL_LINK}>"
)
set_tests_properties(GPSHardwareRunner.Scripted PROPERTIES LABELS "Unit;GPS" TIMEOUT 120)

find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(QGCGPSLegacySafetyTest GPSLegacySafetyTest.cc GPSLegacySafetyTest.h ../GPSDriverTestBase.h
                  ../../../Portable/PortableTest.h
)
set_target_properties(QGCGPSLegacySafetyTest PROPERTIES AUTOMOC ON)
get_target_property(_qgc_px4_includes px4-gpsdrivers INCLUDE_DIRECTORIES)
get_target_property(_qgc_px4_definitions px4-gpsdrivers COMPILE_DEFINITIONS)
target_include_directories(QGCGPSLegacySafetyTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${_qgc_root}/test/Portable"
                                                          ${_qgc_px4_includes}
)
target_compile_definitions(QGCGPSLegacySafetyTest PRIVATE QGC_PORTABLE_TEST ${_qgc_px4_definitions})
target_link_libraries(QGCGPSLegacySafetyTest PRIVATE Qt6::Test QGCGPSLegacyDriver)
add_test(NAME GPSHardwareRunner.LegacySafety COMMAND QGCGPSLegacySafetyTest)
set_tests_properties(GPSHardwareRunner.LegacySafety PROPERTIES LABELS "Unit;GPS" TIMEOUT 120)
