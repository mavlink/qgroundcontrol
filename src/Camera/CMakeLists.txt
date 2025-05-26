target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        CameraMetaData.cc
        CameraMetaData.h
        MavlinkCameraControl.cc
        MavlinkCameraControl.h
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

# qt_add_resources(${CMAKE_PROJECT_NAME} "Camera_res"
#     PREFIX "/"
#     FILES
#         CameraMetaData.json
# )
