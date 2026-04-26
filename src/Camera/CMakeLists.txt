# ============================================================================
# Camera Module
# MAVLink camera control and video stream management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CameraMetaData.cc
        CameraMetaData.h
        MavlinkCameraControlInterface.cc
        MavlinkCameraControlInterface.h
        QGCCameraIO.cc
        QGCCameraIO.h
        QGCCameraManager.cc
        QGCCameraManager.h
        QGCVideoStreamInfo.cc
        QGCVideoStreamInfo.h
        SimulatedCameraControl.cc
        SimulatedCameraControl.h
        VehicleCameraControl.cc
        VehicleCameraControl.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# Camera Metadata Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_mavlink_camera
    PREFIX "/json"
    FILES CameraMetaData.json
)

qt_add_resources(${CMAKE_PROJECT_NAME} camera_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/images"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/images/camera_photo.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/images/camera_video.svg"
)
