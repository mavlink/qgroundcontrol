# ============================================================================
# Vehicle Fact Groups Subsystem
# Vehicle telemetry data organization and binding
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        BatteryFactGroupListModel.cc
        BatteryFactGroupListModel.h
        EscStatusFactGroupListModel.cc
        EscStatusFactGroupListModel.h
        TerrainFactGroup.cc
        TerrainFactGroup.h
        VehicleClockFactGroup.cc
        VehicleClockFactGroup.h
        VehicleDistanceSensorFactGroup.cc
        VehicleDistanceSensorFactGroup.h
        VehicleEFIFactGroup.cc
        VehicleEFIFactGroup.h
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

# ----------------------------------------------------------------------------
# Fact Group Definition Resources
# ----------------------------------------------------------------------------
set(JSON_FILES
    RPMFact.json
    TemperatureFact.json
    GPSFact.json
    EscStatusFactGroup.json
    LocalPositionFact.json
    EstimatorStatusFactGroup.json
    EFIFact.json
    BatteryFact.json
    ClockFact.json
    SubmarineFact.json
    SetpointFact.json
    DistanceSensorFact.json
    GeneratorFact.json
    HygrometerFact.json
    TerrainFactGroup.json
    VehicleFact.json
    VibrationFact.json
    WindFact.json
)
qt_add_resources(${CMAKE_PROJECT_NAME} json_vehicle_fact_group
    PREFIX "/json/Vehicle"
    FILES ${JSON_FILES}
)

# Create alias for LocalPositionSetpointFact.json
set_source_files_properties(LocalPositionFact.json PROPERTIES QT_RESOURCE_ALIAS LocalPositionSetpointFact.json)
qt_add_resources(${CMAKE_PROJECT_NAME} json_vehicle_fact_group_alias
    PREFIX "/json/Vehicle"
    FILES LocalPositionFact.json
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
