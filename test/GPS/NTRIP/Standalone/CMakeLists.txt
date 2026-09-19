cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCNTRIPTests LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Positioning Test)
    set(_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")
    set(QGC_GPS_COMPONENTS_ENABLED NTRIP NTRIPHttp NMEAUtils NMEAProtocol RTCM RTCMFramer)
    add_subdirectory("${_root}/src/Utilities/Logging/Category" LoggingCategory)
    add_subdirectory("${_root}/src/Utilities/Timing/Clock" Clock)
    add_subdirectory("${_root}/src/Utilities/IO" IO)
    add_subdirectory("${_root}/src/Utilities/Network/Client" NetworkClient)
    add_subdirectory("${_root}/src/GPS/NMEA" NMEA)
    add_subdirectory("${_root}/src/GPS/RTCM" RTCM)
    add_subdirectory("${_root}/src/GPS/NTRIP" NTRIP)
    add_subdirectory("${_root}/test/GPS/Standalone" Consumers)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Test)
set(_root "${CMAKE_CURRENT_SOURCE_DIR}/../../../..")

foreach(test NTRIPReentrancyTest NTRIPTlsTest)
    qt_add_executable(QGC${test} ${test}.cc "${_root}/test/Portable/PortableTest.h")
    set_target_properties(
        QGC${test}
        PROPERTIES AUTOMOC ON
                   MACOSX_BUNDLE OFF
                   WIN32_EXECUTABLE OFF
    )
    target_compile_features(QGC${test} PRIVATE cxx_std_20)
    target_compile_definitions(QGC${test} PRIVATE QGC_PORTABLE_TEST QGC_UNITTEST_BUILD)
    target_include_directories(QGC${test} PRIVATE "${_root}/test/Portable")
    target_link_libraries(QGC${test} PRIVATE QGCGPSNTRIP Qt6::Test)
    add_test(NAME ${test} COMMAND QGC${test})
    set_tests_properties(${test} PROPERTIES LABELS "Unit;GPS" TIMEOUT 60 ENVIRONMENT "QT_FATAL_CRITICALS=1")
endforeach()
target_sources(QGCNTRIPReentrancyTest PRIVATE ../MockNTRIPTransport.h)
