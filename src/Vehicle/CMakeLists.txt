set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Autotune.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Autotune.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FTPManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/FTPManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/InitialConnectStateMachine.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/InitialConnectStateMachine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkLogManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MAVLinkLogManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiVehicleManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/MultiVehicleManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteIDManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/StandardModes.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/StandardModes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainProtocolHandler.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainProtocolHandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TrajectoryPoints.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TrajectoryPoints.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Vehicle.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Vehicle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleLinkManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleLinkManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleObjectAvoidance.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/VehicleObjectAvoidance.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "" FORCE
)

add_subdirectory(Actuators)
add_subdirectory(ComponentInformation)
add_subdirectory(FactGroups)
add_subdirectory(VehicleSetup)
