# ============================================================================
# PX4 Sensors Tests
# Unit tests for sensors component and calibration state machines
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PX4OrientationCalibrationMachineTest.cc
        PX4OrientationCalibrationMachineTest.h
        PX4SensorCalibrationStateMachineTest.cc
        PX4SensorCalibrationStateMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
