# ============================================================================
# QGroundControl Unit Tests
# ============================================================================

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# ----------------------------------------------------------------------------
# Test Framework Sources
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        UnitTestList.cc
        UnitTestList.h
)

# ----------------------------------------------------------------------------
# Test Dependencies
# ----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Test)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_UNITTEST_BUILD)

target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE
    <QtTest/QTest>
    <UnitTest.h>
    <MultiSignalSpy.h>
)

# ----------------------------------------------------------------------------
# CTest Integration (CTest included in root CMakeLists.txt)
# ----------------------------------------------------------------------------
include(QGCTest)
include(Coverage)
include(Sanitizers)

# ----------------------------------------------------------------------------
# Optional separate test binary (QGC_BUILD_TEST_BINARY, default OFF)
# ----------------------------------------------------------------------------
# When ON, test object code is built into a standalone `qgc_tests` executable so
# editing a single test relinks only test objects instead of the entire app.
# add_qgc_test() already dispatches through $<TARGET_FILE:qgc_tests> whenever the
# target exists (see cmake/QGCTest.cmake).
#
# FULL WIRING IS NOT YET COMPLETE — this is intentional scaffolding. The default
# OFF path is unaffected: tests stay compiled into ${CMAKE_PROJECT_NAME} via the
# target_sources() calls below and in each test subdirectory.
#
# Remaining wiring required to make ON functional (all CMake-only EXCEPT item 3):
#   1. Collect every test subdirectory's sources into an OBJECT library (e.g.
#      qgc_test_objects) instead of target_sources(${CMAKE_PROJECT_NAME} ...).
#      Each test/**/CMakeLists.txt would target a shared variable/target rather
#      than the app target.
#   2. add_executable(qgc_tests) linking qgc_test_objects + the app's core
#      libraries (the same libs QGroundControl links) + Qt6::Test.
#   3. C++ ENTRY POINT (not owned here): qgc_tests needs a main() that parses
#      `--unittest:<name>` and runs the UnitTestList dispatch, mirroring the app's
#      existing --unittest handling. Without this the binary cannot run tests.
#   4. Mirror QGC_UNITTEST_BUILD compile definition, include dirs, and PCH onto
#      qgc_tests.
# Until items 1-4 land, configuring with -DQGC_BUILD_TEST_BINARY=ON builds no
# qgc_tests target, so add_qgc_test() falls back to the app binary (no breakage).
if(QGC_BUILD_TEST_BINARY)
    message(WARNING
        "QGC_BUILD_TEST_BINARY=ON: qgc_tests target wiring is not yet implemented; "
        "tests will continue to run from the main app binary. See test/CMakeLists.txt.")
endif()

# Sanitizers add significant runtime overhead; expand CTest per-test limits to
# avoid false timeout failures in CI sanitizer jobs.
if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN OR QGC_ENABLE_MSAN)
    set(QGC_TEST_TIMEOUT_UNIT 180 CACHE STRING "Timeout for unit tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_INTEGRATION 360 CACHE STRING "Timeout for integration tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_SLOW 540 CACHE STRING "Timeout for slow tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_DEFAULT 270 CACHE STRING "Default test timeout (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_EXTENDED 360)
else()
    set(QGC_TEST_TIMEOUT_EXTENDED 180)
endif()

# ============================================================================
# Test Subdirectories
# ============================================================================

add_subdirectory(UnitTestFramework)
add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(VideoManager)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FollowMe)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MAVLink)
add_subdirectory(MissionManager)
add_subdirectory(QtLocationPlugin)
add_subdirectory(QmlControls)
add_subdirectory(Terrain)
add_subdirectory(Utilities)
add_subdirectory(Viewer3D)
add_subdirectory(Vehicle)
add_subdirectory(QmlUITests)
