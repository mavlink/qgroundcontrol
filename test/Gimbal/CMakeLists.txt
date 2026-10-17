# ============================================================================
# Gimbal Unit Tests
# Tests for GimbalController / Gimbal against MockLinkGimbal
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        GimbalControllerBasicsTest.cc
        GimbalControllerBasicsTest.h
        GimbalControllerDeltaYawTest.cc
        GimbalControllerDeltaYawTest.h
        GimbalControllerDiscoveryTest.cc
        GimbalControllerDiscoveryTest.h
        GimbalControllerTestBase.cc
        GimbalControllerTestBase.h
)

# Shares file-local constants with GimbalControllerBasicsTest.cc, so it cannot share a unity unit.
set_source_files_properties(
    GimbalControllerDeltaYawTest.cc
    TARGET_DIRECTORY
    QGCUnitTests
    PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GimbalControllerBasicsTest LABELS Integration Vehicle)
add_qgc_test(GimbalControllerDeltaYawTest LABELS Integration Vehicle)
add_qgc_test(GimbalControllerDiscoveryTest LABELS Integration Vehicle)
