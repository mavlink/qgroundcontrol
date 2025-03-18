find_package(Qt6 REQUIRED COMPONENTS Core Gui Positioning Qml)

target_sources(QGC
    PRIVATE
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

target_link_libraries(QGC
    PRIVATE
        Qt6::Qml
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Positioning
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Actuators)
add_subdirectory(ComponentInformation)
add_subdirectory(FactGroups)
add_subdirectory(VehicleSetup)
