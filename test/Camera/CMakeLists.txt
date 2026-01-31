# ============================================================================
# Camera Unit Tests
# Tests for camera management and control
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CameraDiscoveryStateMachineTest.cc
        CameraDiscoveryStateMachineTest.h
        QGCCameraManagerTest.cc
        QGCCameraManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
