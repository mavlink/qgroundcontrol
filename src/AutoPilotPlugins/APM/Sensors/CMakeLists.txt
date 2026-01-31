# ============================================================================
# APM Sensors Component and Calibration State Machines
# Sensors component and state machine implementations for APM sensor calibration workflows
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AccelCalibrationMachine.cc
        AccelCalibrationMachine.h
        APMSensorCalibrationStateMachine.cc
        APMSensorCalibrationStateMachine.h
        APMSensorsComponent.cc
        APMSensorsComponent.h
        APMSensorsComponentController.cc
        APMSensorsComponentController.h
        CompassCalibrationMachine.cc
        CompassCalibrationMachine.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
