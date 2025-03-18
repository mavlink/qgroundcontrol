find_package(Qt6 REQUIRED COMPONENTS Core)

target_sources(QGC
    PRIVATE
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
        VehicleFactGroup.cc
        VehicleFactGroup.h
        VehicleGPS2FactGroup.cc
        VehicleGPS2FactGroup.h
        VehicleGPSFactGroup.cc
        VehicleGPSFactGroup.h
        VehicleLocalPositionFactGroup.cc
        VehicleLocalPositionFactGroup.h
        VehicleLocalPositionSetpointFactGroup.cc
        VehicleLocalPositionSetpointFactGroup.h
        VehicleRPMFactGroup.cc
        VehicleRPMFactGroup.h
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

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
