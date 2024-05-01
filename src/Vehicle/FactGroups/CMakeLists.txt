find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(VehicleFactGroups STATIC
	GPSRTKFactGroup.cc
	GPSRTKFactGroup.h
	TerrainFactGroup.cc
	TerrainFactGroup.h
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
	VehicleLocalPositionFactGroup.cc
	VehicleLocalPositionFactGroup.h
	VehicleLocalPositionSetpointFactGroup.cc
	VehicleLocalPositionSetpointFactGroup.h
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

target_link_libraries(VehicleFactGroups
	PRIVATE
        Geo
		Vehicle
	PUBLIC
		comm
		FactSystem
)

target_include_directories(VehicleFactGroups PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
