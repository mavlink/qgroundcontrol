# ============================================================================
# Common Sensors Tests
# Unit tests for shared sensor calibration infrastructure
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        OrientationCalibrationStateTest.cc
        OrientationCalibrationStateTest.h
        SensorCalibrationSideTest.cc
        SensorCalibrationSideTest.h
        SensorsComponentBaseTest.cc
        SensorsComponentBaseTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
