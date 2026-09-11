# ============================================================================
# Gimbal Unit Tests
# Tests for GimbalController / Gimbal against MockLinkGimbal
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(GimbalControllerBasicsTest LABELS Integration Vehicle)
add_qgc_test(GimbalControllerDeltaYawTest LABELS Integration Vehicle)
add_qgc_test(GimbalControllerDiscoveryTest LABELS Integration Vehicle)
