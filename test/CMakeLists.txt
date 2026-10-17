# ============================================================================
# QGroundControl Unit Tests
# ============================================================================

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# ----------------------------------------------------------------------------
# Test Target
# ----------------------------------------------------------------------------
# Tests build in their own object library so only they carry QtTest in a precompiled header;
# the application executable still links every test object and runs the suite.
add_library(QGCUnitTests OBJECT UnitTestList.cc UnitTestList.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCUnitTests)

# Tests compile against everything the application does, including what its libraries propagate.
target_include_directories(QGCUnitTests PRIVATE $<TARGET_PROPERTY:${CMAKE_PROJECT_NAME},INCLUDE_DIRECTORIES>)
target_compile_definitions(QGCUnitTests PRIVATE $<TARGET_PROPERTY:${CMAKE_PROJECT_NAME},COMPILE_DEFINITIONS>)
target_compile_options(QGCUnitTests PRIVATE $<TARGET_PROPERTY:${CMAKE_PROJECT_NAME},COMPILE_OPTIONS>)
target_precompile_headers(
    QGCUnitTests
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/pch.h"
        <QtTest/QTest>
)
# Tests include generated protocol headers, so order them after the generators.
add_dependencies(QGCUnitTests qgc-analysis-headers)

# ----------------------------------------------------------------------------
# Test Dependencies
# ----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Test)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Portable
)

# QGC_UNITTEST_BUILD is defined directory-wide from the root CMakeLists.txt when
# QGC_BUILD_TESTING is enabled, covering this target and all src/ targets.

# ----------------------------------------------------------------------------
# CTest Integration (CTest included in root CMakeLists.txt)
# ----------------------------------------------------------------------------
include(QGCTest)
include(Coverage)
include(Sanitizers)

# Sanitizers add significant runtime overhead; expand CTest per-test limits to
# avoid false timeout failures in CI sanitizer jobs.
if(QGC_ENABLE_ASAN
   OR QGC_ENABLE_UBSAN
   OR QGC_ENABLE_TSAN
   OR QGC_ENABLE_MSAN
)
    # Preserve longer user timeouts while allowing for sanitizer overhead.
    macro(_qgc_raise_test_timeout variable minimum description)
        if(${variable} LESS ${minimum})
            set(${variable}
                ${minimum}
                CACHE STRING "${description}" FORCE
            )
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

add_subdirectory(Utilities/Timing/Support)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCTestTiming)

add_subdirectory(UnitTestFramework)
add_subdirectory(ADSB)
add_subdirectory(AnalyzeView)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(VideoManager)
add_subdirectory(Comms)
add_subdirectory(FactSystem)
add_subdirectory(FlyView)
add_subdirectory(FollowMe)
add_subdirectory(GeoMap)
add_subdirectory(Gimbal)
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

# ----------------------------------------------------------------------------
# Test Target Dependencies
# ----------------------------------------------------------------------------
# Tests use the application's dependencies, whose usage requirements also mark third-party
# headers as system headers. Only targets visible here can be named; the executable still
# links every dependency itself, so this adds no objects or archives to the link.
get_target_property(_qgc_application_links ${CMAKE_PROJECT_NAME} LINK_LIBRARIES)
foreach(_qgc_link IN LISTS _qgc_application_links)
    if(TARGET "${_qgc_link}" AND NOT _qgc_link STREQUAL "QGCUnitTests")
        set_property(
            TARGET QGCUnitTests
            APPEND
            PROPERTY LINK_LIBRARIES "${_qgc_link}"
        )
    endif()
endforeach()

# Match the application: moc waits for generated protocol headers, not for linked libraries.
if(NOT QGC_CUSTOM_BUILD)
    set_target_properties(
        QGCUnitTests
        PROPERTIES
            AUTOGEN_ORIGIN_DEPENDS OFF
            AUTOGEN_TARGET_DEPENDS qgc-analysis-headers
    )
endif()
