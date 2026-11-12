# ============================================================================
# GPS Protocol Tests
# The protocol test helpers that need the unit test framework. The application-free helpers and the golden
# transcripts are QGCGPSProtocolTestSupport (Support/ and Scenarios/), which test/GPS/Core links; the receiver
# qualification runner (Hardware/) and the reply fuzzer's smoke test (Fuzz/) build here or standalone, and the
# libFuzzer harnesses (Fuzz/) and the golden transcripts' own program (Scenarios/) only standalone
# (test/GPS/Standalone).
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        Support/GPSProtocolTestBase.cc
        Support/GPSProtocolTestBase.h
)

# Linux test builds, including CI, keep the receiver qualification runner compiling and exercised. The runner opens a
# physical receiver only when invoked manually; its CTest uses a scripted receiver.
string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" _qgc_linux)
option(QGC_BUILD_GPS_HARDWARE_TESTS "Build the GPS receiver qualification runner and its scripted test" ${_qgc_linux})
if(QGC_BUILD_GPS_HARDWARE_TESTS)
    add_subdirectory(Hardware)
endif()
add_subdirectory(Fuzz)
