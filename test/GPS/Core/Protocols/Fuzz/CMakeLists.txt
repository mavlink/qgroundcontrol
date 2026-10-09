# ============================================================================
# GPS Protocol Fuzzing
# The libFuzzer harness of the native protocols. Only test/GPS/Standalone builds it, with QGC_GPS_LIBFUZZER,
# which builds QGCGPSCore and everything that links it with ASan and UBSan.
# ============================================================================

add_executable(QGCGPSProtocolFuzz ProtocolFuzz.cc)
target_link_libraries(QGCGPSProtocolFuzz PRIVATE QGCGPSCore QGCGPSProtocolTestSupport)
set_target_properties(
    QGCGPSProtocolFuzz
    PROPERTIES AUTOMOC OFF
               AUTOUIC OFF
               AUTORCC OFF
)
target_link_options(QGCGPSProtocolFuzz PRIVATE -fsanitize=fuzzer)
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../corpus" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(
    COPY "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fixture-seeds"
    FILES_MATCHING
    PATTERN "*.gps"
    PATTERN "*.nmea"
    PATTERN "*.sbf"
    PATTERN "*.ubx"
)
add_test(NAME GPSProtocolFuzzSmoke
         COMMAND QGCGPSProtocolFuzz -runs=10000 -seed=12345 -max_len=8192 "${CMAKE_CURRENT_BINARY_DIR}/corpus"
                 "${CMAKE_CURRENT_BINARY_DIR}/fixture-seeds"
)
set_tests_properties(GPSProtocolFuzzSmoke PROPERTIES LABELS "GPS;Fuzz" TIMEOUT 60)
