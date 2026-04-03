# ============================================================================
# SITL PX4-Specific Tests
# ============================================================================
# Tests that validate PX4-specific behavior: flight lifecycle, mode
# transitions, failsafe display. These use PX4 SITL-SIH and assert
# PX4 firmware plugin behavior.

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4SITLTestBase.h
        PX4SITLTestBase.cc
        tst_PX4Lifecycle.h
        tst_PX4Lifecycle.cc
        tst_PX4Modes.h
        tst_PX4Modes.cc
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(PX4LifecycleTest LABELS SITL PX4 Vehicle RESOURCE_LOCK SITLContainer)
add_qgc_test(PX4ModesTest     LABELS SITL PX4 Vehicle RESOURCE_LOCK SITLContainer)
