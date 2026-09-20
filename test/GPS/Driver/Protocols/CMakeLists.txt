get_filename_component(QGC_GPS_PROTOCOL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/GPS/Driver/Protocols"
                       ABSOLUTE
)
add_library(QGCGPSProtocolTestRuntime INTERFACE)
target_link_libraries(QGCGPSProtocolTestRuntime INTERFACE QGCGPSNative)
target_include_directories(
    QGCGPSProtocolTestRuntime
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.." "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../Corrections"
              "${QGC_GPS_PROTOCOL_SOURCE_DIR}/../../../Utilities/Math"
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSProtocolTestRuntime)

# Register an application test without leaking its injected-clock definition into other suites.
function(qgc_add_protocol_test name source)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    set_property(
        SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/${source}" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY COMPILE_DEFINITIONS QGC_GPS_TEST_CLOCK
    )
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${source}" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
                                PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
    )
    add_qgc_test(${name} LABELS Unit GPS NativeDriver)
endfunction()

qgc_add_protocol_test(GPSProtocolUtcTest gps-time-test.cpp)
get_test_property(GPSProtocolUtcTest ENVIRONMENT _utc_test_environment)
set_property(
    TEST GPSProtocolUtcTest
    APPEND
    PROPERTY ENVIRONMENT "TZ=UTC0"
)
foreach(timezone IN ITEMS EST5EDT JST-9)
    set(_utc_command $<TARGET_FILE:${CMAKE_PROJECT_NAME}> --unittest:GPSProtocolUtcTest --allow-multiple)
    if(QGC_TEST_ONSCREEN)
        list(APPEND _utc_command --onscreen)
    endif()
    add_test(
        NAME GPSProtocolUtcTest-${timezone}
        COMMAND ${_utc_command}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    set_tests_properties(
        GPSProtocolUtcTest-${timezone}
        PROPERTIES LABELS
                   "Unit;GPS;NativeDriver"
                   TIMEOUT
                   ${QGC_TEST_TIMEOUT_UNIT}
                   ENVIRONMENT
                   "${_utc_test_environment};TZ=${timezone}"
                   FAIL_REGULAR_EXPRESSION
                   "FAIL!;Segmentation fault"
    )
endforeach()
qgc_add_protocol_test(GPSProtocolContractsTest GPSProtocolContractsTest.cc)
if(QGC_GPS_ENABLE_UBX)
    qgc_add_protocol_test(GPSProtocolUbxTest gps-ubx-test.cpp)
    qgc_add_protocol_test(GPSProtocolAllocationTest gps-allocation-test.cpp)
    set_property(
        SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/gps-allocation-test.cpp" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
        APPEND
        PROPERTY COMPILE_DEFINITIONS GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    )
endif()
if(QGC_GPS_ENABLE_SBF OR QGC_GPS_ENABLE_FEMTO)
    qgc_add_protocol_test(GPSProtocolReceiverModesTest gps-receiver-mode-test.cpp)
endif()
qgc_add_protocol_test(GPSProtocolIOContractTest gps-io-contract-test.cpp)
qgc_add_protocol_test(GPSProtocolDecodeTest gps-decode-test.cpp)
qgc_add_protocol_test(GPSProtocolFixtureTest gps-fixture-test.cpp)
qgc_add_protocol_test(GPSAsciiProtocolTest GPSAsciiProtocolTest.cc)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSAsciiProtocolTest.h)
set_property(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/gps-fixture-test.cpp" TARGET_DIRECTORY ${CMAKE_PROJECT_NAME}
    APPEND
    PROPERTY COMPILE_DEFINITIONS GPS_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)
foreach(family Unicore Quectel Passive)
    string(TOUPPER "${family}" option_name)
    string(TOLOWER "${family}" source_name)
    if(QGC_GPS_ENABLE_${option_name})
        qgc_add_protocol_test(GPSProtocol${family}Test "gps-${source_name}-test.cpp")
    endif()
endforeach()

option(QGC_BUILD_GPS_PROTOCOL_FUZZER "Build the native protocol libFuzzer harness" OFF)
if(QGC_BUILD_GPS_PROTOCOL_FUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR MSVC)
        message(
            FATAL_ERROR "QGC_BUILD_GPS_PROTOCOL_FUZZER requires a Clang driver with libFuzzer and sanitizer runtimes"
        )
    endif()
    get_target_property(native_targets QGCGPSNative QGC_GPS_NATIVE_TARGETS)
    list(APPEND native_targets QGCGPSNMEAProtocol QGCGPSReceiver QGCGPSTransport QGCGPSRTCM)
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
    add_executable(QGCGPSProtocolFuzz ProtocolFuzz.cc)
    target_link_libraries(QGCGPSProtocolFuzz PRIVATE QGCGPSProtocolTestRuntime)
    target_compile_definitions(QGCGPSProtocolFuzz PRIVATE QGC_GPS_TEST_CLOCK)
    set_target_properties(
        QGCGPSProtocolFuzz
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
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

option(QGC_BUILD_GPS_MINMEA_TESTS "Build the pinned minmea differential tests" OFF)
option(QGC_BUILD_GPS_FIXPOSITION_PROTOTYPE "Build the experimental Fixposition parser comparison" OFF)
if(QGC_BUILD_GPS_MINMEA_TESTS)
    add_subdirectory(Comparisons/Minmea)
endif()
if(QGC_BUILD_GPS_FIXPOSITION_PROTOTYPE)
    add_subdirectory(Comparisons/Fixposition)
endif()
