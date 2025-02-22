find_package(Qt6 REQUIRED COMPONENTS Core Network Qml Xml)

qt_add_library(Camera STATIC
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

target_link_libraries(Camera
    PRIVATE
        Qt6::Network
        Qt6::Qml
        Qt6::Xml
        API
        Comms
        FirmwarePlugin
        Joystick
        QGC
        Settings
        Utilities
        Vehicle
        VideoManager
    PUBLIC
        Qt6::Core
        FactSystem
        MAVLink
        QmlControls
)

target_include_directories(Camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# qt_add_resources(Camera "Camera_res"
#     PREFIX "/"
#     FILES
#         CameraMetaData.json
# )
