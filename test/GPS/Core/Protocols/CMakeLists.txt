# ============================================================================
# GPS Protocol Tests
# The golden scenarios and the protocol test helpers that need the unit test framework. The application-free helpers
# are QGCGPSProtocolTestSupport (Support/), which test/GPS/Core links; the receiver qualification runner (Hardware/)
# builds here or standalone, and the libFuzzer harness (Fuzz/) only standalone (test/GPS/Standalone).
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE Scenarios/GoldenScenarios.cc
            Scenarios/GoldenScenarios.h
            Scenarios/GoldenScenariosAshtech.cc
            Scenarios/GoldenScenariosAutomatic.cc
            Scenarios/GoldenScenariosFemto.cc
            Scenarios/GoldenScenariosPassive.cc
            Scenarios/GoldenScenariosQuectel.cc
            Scenarios/GoldenScenariosSBF.cc
            Scenarios/GoldenScenariosUBX.cc
            Scenarios/GoldenScenariosUnicore.cc
            Support/GoldenFormat.cc
            Support/GoldenFormat.h
            Support/GoldenTranscript.cc
            Support/GoldenTranscript.h
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
