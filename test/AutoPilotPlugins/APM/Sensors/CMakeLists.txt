# ============================================================================
# APM Sensors Tests
# Unit tests for sensors component and calibration state machines
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        AccelCalibrationMachineTest.cc
        AccelCalibrationMachineTest.h
        CompassCalibrationMachineTest.cc
        CompassCalibrationMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
