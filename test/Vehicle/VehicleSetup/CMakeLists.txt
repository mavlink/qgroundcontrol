# ============================================================================
# Vehicle Setup Unit Tests
# Tests for remote control calibration and vehicle setup controllers
# ============================================================================

add_subdirectory(Firmware)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        RCCalibrationStateMachineTest.cc
        RCCalibrationStateMachineTest.h
        RemoteControlCalibrationControllerTest.cc
        RemoteControlCalibrationControllerTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
