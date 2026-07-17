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

# QGC_UNITTEST_BUILD is defined directory-wide from the root CMakeLists.txt when
# QGC_BUILD_TESTING is enabled, covering this target and all src/ targets.

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

# Sanitizers add significant runtime overhead; expand CTest per-test limits to
# avoid false timeout failures in CI sanitizer jobs.
if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN OR QGC_ENABLE_MSAN)
    macro(_qgc_raise_test_timeout variable minimum description)
        if(${variable} LESS ${minimum})
            set(${variable} ${minimum} CACHE STRING "${description}" FORCE)
        endif()
    endmacro()
    _qgc_raise_test_timeout(QGC_TEST_TIMEOUT_UNIT 180 "Timeout for unit tests (seconds)")
    _qgc_raise_test_timeout(QGC_TEST_TIMEOUT_INTEGRATION 360 "Timeout for integration tests (seconds)")
    _qgc_raise_test_timeout(QGC_TEST_TIMEOUT_SLOW 540 "Timeout for slow tests (seconds)")
    _qgc_raise_test_timeout(QGC_TEST_TIMEOUT_DEFAULT 270 "Default test timeout (seconds)")
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
add_subdirectory(Settings)
add_subdirectory(Terrain)
add_subdirectory(Utilities)
add_subdirectory(Viewer3D)
add_subdirectory(Vehicle)
add_subdirectory(QmlUITests)
