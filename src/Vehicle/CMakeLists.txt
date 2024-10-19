add_subdirectory(Actuators)
add_subdirectory(Components)
add_subdirectory(FactGroups)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Positioning Qml)

qt_add_library(Vehicle STATIC
    Autotune.cpp
    Autotune.h
    FTPManager.cc
    FTPManager.h
    InitialConnectStateMachine.cc
    InitialConnectStateMachine.h
    MAVLinkLogManager.cc
    MAVLinkLogManager.h
    MultiVehicleManager.cc
    MultiVehicleManager.h
    RemoteIDManager.cc
    RemoteIDManager.h
    StandardModes.cc
    StandardModes.h
    TerrainProtocolHandler.cc
    TerrainProtocolHandler.h
    TrajectoryPoints.cc
    TrajectoryPoints.h
    Vehicle.cc
    Vehicle.h
    VehicleLinkManager.cc
    VehicleLinkManager.h
    VehicleObjectAvoidance.cc
    VehicleObjectAvoidance.h
)

target_link_libraries(Vehicle
    PRIVATE
        Qt6::Qml
        VehicleActuators
        VehicleComponents
        ADSB
        API
        Audio
        AutoPilotPlugins
        Camera
        FirmwarePlugin
        Joystick
        MockLink
        Utilities
        UTMSP
        VideoManager
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
        Comms
        FactSystem
        Gimbal
        LibEventsWrapper
        MAVLink
        MissionManager
        QGC
        VehicleFactGroups
)

target_include_directories(Vehicle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
