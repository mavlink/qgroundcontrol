find_package(Qt6 REQUIRED COMPONENTS Core Network Qml Xml)

target_sources(QGC
    PRIVATE
        CameraMetaData.cc
        CameraMetaData.h
        MavlinkCameraControl.cc
        MavlinkCameraControl.h
        QGCCameraIO.cc
        QGCCameraIO.h
        QGCCameraManager.cc
        QGCCameraManager.h
        SimulatedCameraControl.cc
        SimulatedCameraControl.h
        VehicleCameraControl.cc
        VehicleCameraControl.h
)

target_link_libraries(QGC
    PRIVATE
        Qt6::Network
        Qt6::Qml
        Qt6::Xml
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_resources(QGC "Camera_res"
#     PREFIX "/"
#     FILES
#         CameraMetaData.json
# )
