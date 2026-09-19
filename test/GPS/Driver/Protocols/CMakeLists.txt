cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCNativeGPSDriverTests LANGUAGES CXX)
    enable_testing()
endif()

get_filename_component(QGC_GPS_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/GPS/Driver/Protocols"
                       ABSOLUTE
)
if(NOT TARGET QGCGPSNative)
    add_subdirectory("${QGC_GPS_PROTOCOL_SOURCE_DIR}" Native)
endif()
add_library(QGCGPSProtocolTestRuntime INTERFACE)
target_link_libraries(QGCGPSProtocolTestRuntime INTERFACE QGCGPSNative)
target_include_directories(
    QGCGPSProtocolTestRuntime
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../Corrections"
              "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../../Utilities/Math"
)
target_compile_definitions(QGCGPSProtocolTestRuntime INTERFACE QGC_GPS_TEST_CLOCK)

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

qgc_add_protocol_test_target(QGCNativeGPSUtcTest gps-time-test.cpp)
if(QGC_GPS_ENABLE_UBX)
    qgc_add_protocol_test_target(QGCNativeGPSUbxTest gps-ubx-test.cpp)
    qgc_add_protocol_test_target(QGCNativeGPSAllocationTest gps-allocation-test.cpp)
    target_compile_definitions(QGCNativeGPSAllocationTest
                               PRIVATE GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    )
endif()
if(QGC_GPS_ENABLE_SBF OR QGC_GPS_ENABLE_FEMTO)
    qgc_add_protocol_test_target(QGCNativeGPSReceiverModesTest gps-receiver-mode-test.cpp)
endif()
qgc_add_protocol_test_target(QGCNativeGPSIOContractTest gps-io-contract-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSDecodeTest gps-decode-test.cpp)
qgc_add_protocol_test_target(QGCNativeGPSFixtureTest gps-fixture-test.cpp)
target_compile_definitions(QGCNativeGPSFixtureTest PRIVATE GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

foreach(test IN ITEMS Decode Ubx Allocation ReceiverModes IOContract Fixture)
    if(NOT TARGET QGCNativeGPS${test}Test)
        continue()
    endif()
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
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
        message(
            FATAL_ERROR "QGC_BUILD_GPS_PROTOCOL_FUZZER requires a Clang driver with libFuzzer and sanitizer runtimes"
        )
    endif()
    get_target_property(native_targets QGCGPSNative QGC_GPS_NATIVE_TARGETS)
    list(APPEND native_targets QGCGPSNMEAProtocol QGCGPSReceiverConfig)
    foreach(native_target IN LISTS native_targets)
        get_target_property(native_type ${native_target} TYPE)
        if(native_type STREQUAL "INTERFACE_LIBRARY")
            set(_sanitizer_scope INTERFACE)
        else()
            set(_sanitizer_scope PUBLIC)
        endif()
        target_compile_options(${native_target} ${_sanitizer_scope} -fsanitize=fuzzer-no-link,address,undefined
                               -fno-omit-frame-pointer
        )
        target_link_options(${native_target} INTERFACE -fsanitize=address,undefined)
    endforeach()
    add_executable(QGCGPSFuzzNMEAConsumer "../../Standalone/QGCGPSNMEAProtocolConsumer.cc")
    target_link_libraries(QGCGPSFuzzNMEAConsumer PRIVATE QGCGPSNMEAProtocol)
    add_test(NAME GPSProtocolFuzzNMEAConsumer COMMAND QGCGPSFuzzNMEAConsumer)
    set_tests_properties(GPSProtocolFuzzNMEAConsumer PROPERTIES LABELS "Unit;GPS;Fuzz" TIMEOUT 10)
    foreach(family UBX Ashtech SBF Femto)
        if(NOT TARGET QGCGPS${family})
            continue()
        endif()
        add_executable(QGCGPSFuzz${family}Consumer NativeFamilyConsumer.cc)
        target_compile_definitions(QGCGPSFuzz${family}Consumer
                                   PRIVATE QGC_GPS_DRIVER_HEADER="${family}/GPSDriver${family}.h"
                                           QGC_GPS_DRIVER_CLASS=GPSNative${family}
        )
        target_link_libraries(QGCGPSFuzz${family}Consumer PRIVATE QGCGPS${family})
        add_test(NAME GPSProtocolFuzz${family}Consumer COMMAND QGCGPSFuzz${family}Consumer)
        set_tests_properties(GPSProtocolFuzz${family}Consumer PROPERTIES LABELS "Unit;GPS;Fuzz" TIMEOUT 10)
    endforeach()
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
