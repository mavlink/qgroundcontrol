cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCNativeGPSDriverTests LANGUAGES CXX)
    enable_testing()
endif()

# Compile the checked-in driver bytes with the host test platform contract. These
# tests need neither a Qt installation nor the separate PX4-GPSDrivers checkout.
get_filename_component(QGC_GPS_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/GPS/Driver/Protocols"
                       ABSOLUTE
)

include("${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../../Utilities/Geo/GeographicLib.cmake")

include("${QGC_GPS_PROTOCOL_SOURCE_DIR}/NMEA/NMEAProtocol.cmake")
include("${QGC_GPS_PROTOCOL_SOURCE_DIR}/GPSProtocolSources.cmake")
add_library(QGCGPSProtocolTestRuntime STATIC ${GPS_PROTOCOL_SOURCES})
target_compile_features(QGCGPSProtocolTestRuntime PUBLIC cxx_std_20)
target_link_libraries(QGCGPSProtocolTestRuntime PUBLIC GeographicLib::GeographicLib QGCGPSNMEAProtocol)
target_include_directories(
    QGCGPSProtocolTestRuntime
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}"
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}/.."
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../Contracts"
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../Core"
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../Corrections"
           "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../../Utilities/Math"
)
target_compile_definitions(QGCGPSProtocolTestRuntime
                           PUBLIC GPS_PLATFORM_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/definitions.h"
)
set_target_properties(
    QGCGPSProtocolTestRuntime
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
if(NOT MSVC)
    target_compile_options(QGCGPSProtocolTestRuntime PRIVATE -Wall -Wextra)
endif()

# Build a host test against the shared native protocol runtime.
function(qgc_add_protocol_test_target target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE QGCGPSProtocolTestRuntime)
    set_target_properties(
        ${target}
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
endfunction()

add_subdirectory(NMEA)

qgc_add_protocol_test_target(QGCNativeGPSUtcTest gps-time-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSUbxTest gps-ubx-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSReceiverModesTest gps-receiver-mode-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSIOContractTest gps-io-contract-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSDecodeTest gps-decode-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSFixtureTest gps-fixture-test.cpp)
target_compile_definitions(QGCNativeGPSFixtureTest PRIVATE GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

foreach(test IN ITEMS Decode Ubx ReceiverModes IOContract Fixture)
    add_test(NAME GPSProtocol${test} COMMAND QGCNativeGPS${test}Test)
    set_tests_properties(GPSProtocol${test} PROPERTIES LABELS "Unit;GPS;NativeDriver" TIMEOUT 60)
endforeach()

foreach(timezone IN ITEMS UTC0 EST5EDT JST-9)
    foreach(variant IN ITEMS Utc)
        add_test(NAME GPSProtocol${variant}-${timezone} COMMAND QGCNativeGPS${variant}Test)
        set_tests_properties(GPSProtocol${variant}-${timezone} PROPERTIES LABELS "Unit;GPS;NativeDriver" TIMEOUT 60
                                                                          ENVIRONMENT "TZ=${timezone}"
        )
    endforeach()
endforeach()

option(QGC_BUILD_GPS_PROTOCOL_FUZZER "Build the native protocol libFuzzer harness" OFF)
if(QGC_BUILD_GPS_PROTOCOL_FUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "QGC_BUILD_GPS_PROTOCOL_FUZZER requires Clang")
    endif()
    target_compile_options(QGCGPSProtocolTestRuntime PRIVATE -fsanitize=fuzzer-no-link,address,undefined
                                                             -fno-omit-frame-pointer
    )
    target_link_options(QGCGPSProtocolTestRuntime INTERFACE -fsanitize=address,undefined)
    qgc_add_protocol_test_target(QGCGPSProtocolFuzz ProtocolFuzz.cc)
    target_compile_options(QGCGPSProtocolFuzz PRIVATE -fsanitize=fuzzer-no-link,address,undefined
                                                      -fno-omit-frame-pointer
    )
    target_link_options(QGCGPSProtocolFuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/corpus" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME GPSProtocolFuzzSmoke COMMAND QGCGPSProtocolFuzz -runs=10000 -seed=12345 -max_len=8192
                                               "${CMAKE_CURRENT_BINARY_DIR}/corpus"
    )
    set_tests_properties(GPSProtocolFuzzSmoke PROPERTIES LABELS "GPS;Fuzz" TIMEOUT 60)
endif()
