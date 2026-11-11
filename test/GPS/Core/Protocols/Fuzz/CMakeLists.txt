# ============================================================================
# GPS Protocol Fuzzing
# A smoke test that replays the reply fuzzer's seeds, in the QGC test build and test/GPS/Standalone, and the libFuzzer
# harnesses of the native protocols: one decodes its input, the other feeds it to the driver as the replies of the
# receiver it configures or detects. Only the standalone project builds the harnesses, with QGC_GPS_LIBFUZZER, which
# builds QGCGPSCore and everything that links it with ASan and UBSan.
# ============================================================================

add_executable(QGCGPSReplyFuzzSmoke ReplyFuzz.cc ReplyFuzz.h ReplyFuzzSmoke.cc)
target_compile_features(QGCGPSReplyFuzzSmoke PRIVATE cxx_std_20)
target_link_libraries(
    QGCGPSReplyFuzzSmoke
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
)
set_target_properties(
    QGCGPSReplyFuzzSmoke
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
add_test(NAME GPSReplyFuzzSmoke COMMAND QGCGPSReplyFuzzSmoke)
# The unit-test timeout of the application's sanitizer builds, which run it unoptimized.
set_tests_properties(
    GPSReplyFuzzSmoke
    PROPERTIES
        LABELS "Unit;GPS;Fuzz"
        TIMEOUT 180
)

if(NOT QGC_GPS_LIBFUZZER)
    return()
endif()

add_executable(QGCGPSReplyFuzz ReplyFuzz.cc ReplyFuzz.h)
target_link_libraries(
    QGCGPSReplyFuzz
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
)
set_target_properties(
    QGCGPSReplyFuzz
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
target_link_options(QGCGPSReplyFuzz PRIVATE -fsanitize=fuzzer)

add_executable(QGCGPSProtocolFuzz ProtocolFuzz.cc)
target_link_libraries(
    QGCGPSProtocolFuzz
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
)
set_target_properties(
    QGCGPSProtocolFuzz
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
target_link_options(QGCGPSProtocolFuzz PRIVATE -fsanitize=fuzzer)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../corpus" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(
    COPY "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fixture-seeds"
    FILES_MATCHING
    PATTERN "*.cap"
    PATTERN "*.gps"
    PATTERN "*.nmea"
    PATTERN "*.sbf"
    PATTERN "*.ubx"
)
add_test(
    NAME GPSProtocolFuzzSmoke COMMAND QGCGPSProtocolFuzz -runs=10000 -seed=12345 -max_len=8192
                                      "${CMAKE_CURRENT_BINARY_DIR}/corpus" "${CMAKE_CURRENT_BINARY_DIR}/fixture-seeds"
)
# Always built with ASan and UBSan, so it gets the unit-test timeout of the application's sanitizer builds.
set_tests_properties(
    GPSProtocolFuzzSmoke
    PROPERTIES
        LABELS "GPS;Fuzz"
        TIMEOUT 180
)
