# ============================================================================
# Common Sensors Component
# Shared sensor calibration infrastructure for APM and PX4
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        OrientationCalibrationState.cc
        OrientationCalibrationState.h
        SensorCalibrationControllerBase.cc
        SensorCalibrationControllerBase.h
        SensorCalibrationSide.h
        SensorCalibrationStateMachineBase.cc
        SensorCalibrationStateMachineBase.h
        SensorsComponentBase.cc
        SensorsComponentBase.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
