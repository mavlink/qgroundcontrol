# ============================================================================
# PX4 Sensors Component and Calibration State Machines
# Sensors component and state machine implementations for PX4 sensor calibration workflows
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4OrientationCalibrationMachine.cc
        PX4OrientationCalibrationMachine.h
        PX4SensorCalibrationStateMachine.cc
        PX4SensorCalibrationStateMachine.h
        SensorsComponent.cc
        SensorsComponent.h
        SensorsComponentController.cc
        SensorsComponentController.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
