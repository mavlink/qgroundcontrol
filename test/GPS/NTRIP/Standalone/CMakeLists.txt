cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCNTRIPTests LANGUAGES CXX)
    enable_testing()
endif()

find_package(
    Qt6 6.8 REQUIRED
    COMPONENTS Core
               Network
               Positioning
               Test
               HttpServer
               Bluetooth
               QmlIntegration
)
set(_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
if(NOT TARGET QGCLoggingCategory)
    add_subdirectory("${_root}/src/Utilities/Logging/Category" LoggingCategory)
endif()
if(NOT TARGET QGCClock)
    add_subdirectory("${_root}/src/Utilities/Timing/Clock" Clock)
endif()
if(NOT TARGET QGCIO)
    add_subdirectory("${_root}/src/Utilities/IO" IO)
endif()
if(NOT TARGET QGCGPSRTCM)
    add_subdirectory("${_root}/src/GPS/RTCM" RTCM)
endif()

qt_add_executable(
    QGCNTRIPReentrancyTest
    NTRIPReentrancyTest.cc
    "${_root}/test/Portable/PortableTest.h"
    ../MockNTRIPTransport.h
    "${_root}/src/GPS/NTRIP/NTRIPError.h"
    "${_root}/src/GPS/NTRIP/NTRIPTransport.h"
    "${_root}/src/GPS/NTRIP/NTRIPConfiguration.cc"
    "${_root}/src/GPS/NTRIP/NTRIPHttpTransport.cc"
    "${_root}/src/GPS/NTRIP/NTRIPSourceTableController.cc"
    "${_root}/src/GPS/NTRIP/NTRIPSourceTable.cc"
    "${_root}/src/GPS/NTRIP/NTRIPGgaProvider.cc"
    "${_root}/src/GPS/NTRIP/NTRIPConnectionStats.cc"
    "${_root}/src/GPS/NMEA/NMEAUtils.cc"
    "${_root}/src/GPS/NMEA/NMEASentence.cc"
    "${_root}/src/Utilities/Network/QGCNetworkHelper.cc"
)
set_target_properties(
    QGCNTRIPReentrancyTest
    PROPERTIES AUTOMOC ON
               MACOSX_BUNDLE OFF
               WIN32_EXECUTABLE OFF
)
target_compile_features(QGCNTRIPReentrancyTest PRIVATE cxx_std_20)
target_compile_definitions(QGCNTRIPReentrancyTest PRIVATE QGC_PORTABLE_TEST QGC_UNITTEST_BUILD)
target_include_directories(
    QGCNTRIPReentrancyTest
    PRIVATE "${_root}/src/GPS" "${_root}/src/GPS/NTRIP" "${_root}/src/GPS/NMEA" "${_root}/src/Utilities/Network"
            "${_root}/src/Utilities/Compression" "${_root}/test/Portable"
)
target_link_libraries(
    QGCNTRIPReentrancyTest
    PRIVATE QGCGPSRTCM
            QGCIO
            QGCLoggingCategory
            QGCClock
            Qt6::Core
            Qt6::Network
            Qt6::Positioning
            Qt6::Test
            Qt6::HttpServer
            Qt6::Bluetooth
            Qt6::QmlIntegration
)
add_test(NAME NTRIPReentrancyTest COMMAND QGCNTRIPReentrancyTest)
set_tests_properties(NTRIPReentrancyTest PROPERTIES LABELS "Unit;GPS" TIMEOUT 60 ENVIRONMENT "QT_FATAL_CRITICALS=1")
