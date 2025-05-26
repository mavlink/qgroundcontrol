target_sources(${CMAKE_PROJECT_NAME}
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Actuators)
add_subdirectory(ComponentInformation)
add_subdirectory(FactGroups)
add_subdirectory(VehicleSetup)
