set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraMetaData.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/CameraMetaData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkCameraControl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MavlinkCameraControl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCameraIO.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCameraIO.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCameraManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCameraManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SimulatedCameraControl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/SimulatedCameraControl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleCameraControl.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleCameraControl.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

# qt_add_resources(${CMAKE_PROJECT_NAME} "Camera_res"
#     PREFIX "/"
#     FILES
#         CameraMetaData.json
# )
