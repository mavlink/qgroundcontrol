cmake_minimum_required(VERSION 3.25)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(QGCGPSFuzz LANGUAGES CXX)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Core Positioning)
endif()

option(QGC_BUILD_GPS_FUZZERS "Build isolated GPS parser fuzzers with Clang sanitizers" OFF)
if(NOT QGC_BUILD_GPS_FUZZERS)
    return()
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "QGC_BUILD_GPS_FUZZERS requires Clang with libFuzzer")
endif()

include(../Replay/ParserTargets.cmake)
qgc_add_gps_parser_runtime(QGCGPSFuzzRuntime)
target_compile_options(QGCGPSFuzzRuntime PRIVATE -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer)

# Build an instrumented parser harness and register a bounded corpus smoke test.
function(qgc_add_gps_fuzzer name source corpus)
    add_executable(${name} ${source} FuzzRuntime.cc)
    target_link_libraries(${name} PRIVATE QGCGPSFuzzRuntime)
    target_compile_options(${name} PRIVATE -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer)
    target_link_options(${name} PRIVATE -fsanitize=fuzzer,address,undefined)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/corpus/${corpus}" "${CMAKE_CURRENT_BINARY_DIR}/artifacts")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/corpus/${corpus}/"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/corpus/${corpus}"
    )
    add_test(
        NAME ${name}Smoke
        COMMAND
            ${name} -runs=1000 -seed=12345 -max_len=65536 -timeout=2 -rss_limit_mb=512
            "-artifact_prefix=${CMAKE_CURRENT_BINARY_DIR}/artifacts/" "${CMAKE_CURRENT_BINARY_DIR}/corpus/${corpus}"
    )
    set_tests_properties(${name}Smoke PROPERTIES LABELS "GPS;Fuzz" TIMEOUT 60)
endfunction()

qgc_add_gps_fuzzer(QGCGPSHttpFuzz HttpFuzz.cc http)
qgc_add_gps_fuzzer(QGCGPSRtcmFuzz RtcmFuzz.cc rtcm)
qgc_add_gps_fuzzer(QGCGPSNmeaFuzz NmeaFuzz.cc nmea)
