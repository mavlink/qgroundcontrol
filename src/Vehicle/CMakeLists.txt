add_subdirectory(Actuators)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Positioning Qml)

qt_add_library(Vehicle STATIC
	Autotune.cpp
	Autotune.h
	CompInfo.cc
	CompInfo.h
	CompInfoActuators.cc
	CompInfoActuators.h
	CompInfoEvents.cc
	CompInfoEvents.h
	CompInfoGeneral.cc
	CompInfoGeneral.h
	CompInfoParam.cc
	CompInfoParam.h
	ComponentInformationCache.cc
	ComponentInformationCache.h
	ComponentInformationManager.cc
	ComponentInformationManager.h
	ComponentInformationTranslation.cc
	ComponentInformationTranslation.h
	EventHandler.cc
	EventHandler.h
	FTPManager.cc
	FTPManager.h
	GPSRTKFactGroup.cc
	GPSRTKFactGroup.h
	HealthAndArmingCheckReport.cc
	HealthAndArmingCheckReport.h
	ImageProtocolManager.cc
	ImageProtocolManager.h
	InitialConnectStateMachine.cc
	InitialConnectStateMachine.h
	MAVLinkLogManager.cc
	MAVLinkLogManager.h
	MAVLinkStreamConfig.cc
	MAVLinkStreamConfig.h
	MultiVehicleManager.cc
	MultiVehicleManager.h
	RemoteIDManager.cc
	RemoteIDManager.h
	StandardModes.cc
	StandardModes.h
	StateMachine.cc
	StateMachine.h
	SysStatusSensorInfo.cc
	SysStatusSensorInfo.h
	TerrainFactGroup.cc
	TerrainFactGroup.h
	TerrainProtocolHandler.cc
	TerrainProtocolHandler.h
	TrajectoryPoints.cc
	TrajectoryPoints.h
	UASMessageHandler.cc
	UASMessageHandler.h
	Vehicle.cc
	Vehicle.h
	VehicleBatteryFactGroup.cc
	VehicleBatteryFactGroup.h
	VehicleClockFactGroup.cc
	VehicleClockFactGroup.h
	VehicleDistanceSensorFactGroup.cc
	VehicleDistanceSensorFactGroup.h
	VehicleEFIFactGroup.cc
	VehicleEFIFactGroup.h
	VehicleEscStatusFactGroup.cc
	VehicleEscStatusFactGroup.h
	VehicleEstimatorStatusFactGroup.cc
	VehicleEstimatorStatusFactGroup.h
	VehicleGeneratorFactGroup.cc
	VehicleGeneratorFactGroup.h
	VehicleGPS2FactGroup.cc
	VehicleGPS2FactGroup.h
	VehicleGPSFactGroup.cc
	VehicleGPSFactGroup.h
	VehicleLinkManager.cc
	VehicleLinkManager.h
	VehicleLocalPositionFactGroup.cc
	VehicleLocalPositionFactGroup.h
	VehicleLocalPositionSetpointFactGroup.cc
	VehicleLocalPositionSetpointFactGroup.h
	VehicleObjectAvoidance.cc
	VehicleObjectAvoidance.h
	VehicleSetpointFactGroup.cc
	VehicleSetpointFactGroup.h
	VehicleTemperatureFactGroup.cc
	VehicleTemperatureFactGroup.h
	VehicleVibrationFactGroup.cc
	VehicleVibrationFactGroup.h
	VehicleWindFactGroup.cc
	VehicleWindFactGroup.h
	VehicleHygrometerFactGroup.cc
	VehicleHygrometerFactGroup.h
)

# include(FetchContent)
# FetchContent_Declare(libevents
# 	GIT_REPOSITORY https://github.com/mavlink/libevents.git
# 	GIT_TAG main
# 	GIT_SHALLOW TRUE
# )
# FetchContent_MakeAvailable(libevents)

add_subdirectory(${CMAKE_SOURCE_DIR}/libs/libevents libevents.build)

target_link_libraries(Vehicle
	PRIVATE
		Qt6::Qml
		Actuators
		ADSB
        api
		Audio
                Camera
		Compression
		FirmwarePlugin
        Geo
        Utilities
		UTMSP
	PUBLIC
		Qt6::Core
		Qt6::Gui
		Qt6::Positioning
		libevents_wrapper
		comm
		FactSystem
                MissionManager
        qgc
)

target_include_directories(Vehicle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
