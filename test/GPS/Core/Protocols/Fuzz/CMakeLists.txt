# ============================================================================
# GPS Protocol Fuzzing
# The smoke tests of the fuzzers, in the QGC test build and test/GPS/Standalone, and the libFuzzer harnesses: of the
# native protocols, where one decodes its input and the other feeds it to the driver as the replies of the receiver it
# configures or detects, and of what a caster sends, the RTCM station position and the NTRIP source table. Only the
# standalone project builds the harnesses, with QGC_GPS_LIBFUZZER, which builds QGCGPSCore, QGCNTRIPHttp and everything
# that links them with ASan and UBSan.
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

# The station position and source table fuzzers keep their seeds in ../corpus/<harness>. CorpusReplaySmoke.cc reads
# them there, so no test writes into a corpus.
add_executable(QGCRTCMStationPositionFuzzSmoke StationPositionFuzz.cc CorpusReplaySmoke.cc)
target_compile_features(QGCRTCMStationPositionFuzzSmoke PRIVATE cxx_std_20)
target_link_libraries(
    QGCRTCMStationPositionFuzzSmoke
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
)
set_target_properties(
    QGCRTCMStationPositionFuzzSmoke
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
add_test(
    NAME RTCMStationPositionFuzzSmoke COMMAND QGCRTCMStationPositionFuzzSmoke
                                              "${CMAKE_CURRENT_SOURCE_DIR}/../corpus/station-position"
)

# The source table is a source of the application that needs Qt Core and Qt Positioning only, so its fuzzer compiles
# it. QGCNTRIPHttp, which the table arrives through, brings the standalone project's sanitizers.
if(NOT TARGET Qt6::Positioning)
    find_package(Qt6 REQUIRED COMPONENTS Positioning)
endif()
get_target_property(_qgc_ntrip_http_dir QGCNTRIPHttp SOURCE_DIR)
get_filename_component(_qgc_ntrip_dir "${_qgc_ntrip_http_dir}/.." ABSOLUTE)
set(_qgc_ntrip_source_table "${_qgc_ntrip_dir}/NTRIPSourceTable.cc" "${_qgc_ntrip_dir}/NTRIPSourceTable.h")
add_executable(QGCNTRIPSourceTableFuzzSmoke SourceTableFuzz.cc CorpusReplaySmoke.cc ${_qgc_ntrip_source_table})
target_compile_features(QGCNTRIPSourceTableFuzzSmoke PRIVATE cxx_std_20)
target_include_directories(QGCNTRIPSourceTableFuzzSmoke PRIVATE "${_qgc_ntrip_dir}")
target_link_libraries(
    QGCNTRIPSourceTableFuzzSmoke
    PRIVATE
        QGC::NTRIPHttp
        Qt6::Positioning
)
set_target_properties(
    QGCNTRIPSourceTableFuzzSmoke
    PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
)
add_test(
    NAME NTRIPSourceTableFuzzSmoke COMMAND QGCNTRIPSourceTableFuzzSmoke
                                           "${CMAKE_CURRENT_SOURCE_DIR}/../corpus/source-table"
)
set_tests_properties(
    RTCMStationPositionFuzzSmoke NTRIPSourceTableFuzzSmoke
    PROPERTIES
        LABELS "Unit;GPS;Fuzz"
        TIMEOUT 60
)

if(NOT QGC_GPS_LIBFUZZER)
    return()
endif()

add_executable(QGCRTCMStationPositionFuzz StationPositionFuzz.cc)
target_link_libraries(
    QGCRTCMStationPositionFuzz
    PRIVATE
        QGCGPSCore
        QGCGPSProtocolTestSupport
)
set_target_properties(
    QGCRTCMStationPositionFuzz
    PROPERTIES
        AUTOMOC OFF
        AUTOUIC OFF
        AUTORCC OFF
)
target_link_options(QGCRTCMStationPositionFuzz PRIVATE -fsanitize=fuzzer)

add_executable(QGCNTRIPSourceTableFuzz SourceTableFuzz.cc ${_qgc_ntrip_source_table})
target_include_directories(QGCNTRIPSourceTableFuzz PRIVATE "${_qgc_ntrip_dir}")
target_link_libraries(
    QGCNTRIPSourceTableFuzz
    PRIVATE
        QGC::NTRIPHttp
        Qt6::Positioning
)
set_target_properties(
    QGCNTRIPSourceTableFuzz
    PROPERTIES
        AUTOMOC ON
        AUTOUIC OFF
        AUTORCC OFF
)
target_link_options(QGCNTRIPSourceTableFuzz PRIVATE -fsanitize=fuzzer)

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
# Without the other harnesses' seeds, which libFuzzer would read from the subdirectories.
file(
    COPY "${CMAKE_CURRENT_SOURCE_DIR}/../corpus"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
    PATTERN "source-table" EXCLUDE
    PATTERN "station-position" EXCLUDE
)
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
# The smoke test reads the same seeds from the source tree, as a file list instead of corpus directories: libFuzzer
# writes every input it keeps into its first corpus directory, which would grow with each run and slow the next.
set(_qgc_gps_fuzz_seed_globs "${CMAKE_CURRENT_SOURCE_DIR}/../corpus/*")
foreach(suffix cap gps nmea sbf ubx)
    list(APPEND _qgc_gps_fuzz_seed_globs "${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/*.${suffix}")
endforeach()
file(
    GLOB _qgc_gps_fuzz_seeds
    LIST_DIRECTORIES false
    ${_qgc_gps_fuzz_seed_globs}
)
list(SORT _qgc_gps_fuzz_seeds)
# libFuzzer splits the list at commas and takes the rest of the file literally, so it ends without a newline.
list(JOIN _qgc_gps_fuzz_seeds "," _qgc_gps_fuzz_seed_list)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/smoke-seeds.txt" "${_qgc_gps_fuzz_seed_list}")
add_test(
    NAME GPSProtocolFuzzSmoke COMMAND QGCGPSProtocolFuzz -runs=10000 -seed=12345 -max_len=8192
                                      "-seed_inputs=@${CMAKE_CURRENT_BINARY_DIR}/smoke-seeds.txt"
)
# Always built with ASan and UBSan, so it gets the unit-test timeout of the application's sanitizer builds.
set_tests_properties(
    GPSProtocolFuzzSmoke
    PROPERTIES
        LABELS "GPS;Fuzz"
        TIMEOUT 180
)
