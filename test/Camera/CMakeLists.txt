# ============================================================================
# Camera Unit Tests
# Tests for camera management and control
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCCameraManagerTest.cc
        QGCCameraManagerTest.h
        VehicleCameraControlTest.cc
        VehicleCameraControlTest.h
        QGCVideoStreamInfoTest.cc
        QGCVideoStreamInfoTest.h
        VideoManagerTest.cc
        VideoManagerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCCameraManagerTest LABELS Integration Vehicle)
add_qgc_test(VehicleCameraControlTest LABELS Integration Vehicle)
add_qgc_test(QGCVideoStreamInfoTest LABELS Unit Camera)
add_qgc_test(VideoManagerTest LABELS Unit)
