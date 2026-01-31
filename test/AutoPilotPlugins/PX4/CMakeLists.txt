# ============================================================================
# PX4 AutoPilotPlugins Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        PowerCalibrationStateMachineTest.cc
        PowerCalibrationStateMachineTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(Sensors)
