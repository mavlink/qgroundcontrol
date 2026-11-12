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
# Test include directories and test support libraries go on QGCUnitTests, never on the application:
# its objects would otherwise recompile whenever a test directory is added.
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
    QGCUnitTests
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
target_link_libraries(QGCUnitTests PRIVATE QGCTestTiming)

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
get_target_property(_qgc_application_includes ${CMAKE_PROJECT_NAME} INCLUDE_DIRECTORIES)
foreach(_qgc_link IN LISTS _qgc_application_links)
    if(TARGET "${_qgc_link}" AND NOT _qgc_link STREQUAL "QGCUnitTests")
        set_property(
            TARGET QGCUnitTests
            APPEND
            PROPERTY LINK_LIBRARIES "${_qgc_link}"
        )
        get_target_property(_qgc_link_includes ${_qgc_link} INTERFACE_INCLUDE_DIRECTORIES)
        if(_qgc_link_includes)
            list(APPEND _qgc_application_includes ${_qgc_link_includes})
        endif()
    endif()
endforeach()

# The application must not regain a test include directory, its own or a linked library's.
foreach(_qgc_include IN LISTS _qgc_application_includes)
    string(REGEX REPLACE "^\\$<BUILD_INTERFACE:(.*)>$" "\\1" _qgc_include "${_qgc_include}")
    cmake_path(IS_PREFIX CMAKE_CURRENT_SOURCE_DIR "${_qgc_include}" NORMALIZE _qgc_is_test_include)
    if(_qgc_is_test_include)
        message(
            FATAL_ERROR "QGC: '${_qgc_include}' is an include directory of ${CMAKE_PROJECT_NAME}. Attach test include "
                        "directories and test support libraries to QGCUnitTests instead."
        )
    endif()
endforeach()
