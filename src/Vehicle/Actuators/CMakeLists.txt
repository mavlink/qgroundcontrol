# ============================================================================
# Actuators Subsystem
# Motor, servo, and actuator configuration and testing
# ============================================================================

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE
        ActuatorActions.cc
        ActuatorActions.h
        ActuatorOutputs.cc
        ActuatorOutputs.h
        Actuators.cc
        Actuators.h
        ActuatorTesting.cc
        ActuatorTesting.h
        Common.cc
        Common.h
        GeometryImage.cc
        GeometryImage.h
        Mixer.cc
        Mixer.h
        MotorAssignment.cc
        MotorAssignment.h
)

# File-scope using-directives (ActuatorOutputs and Mixer both define ChannelConfig) would leak into later sources of a
# unity unit.
set_source_files_properties(
    ActuatorActions.cc
    ActuatorOutputs.cc
    Actuators.cc
    ActuatorTesting.cc
    GeometryImage.cc
    Mixer.cc
    TARGET_DIRECTORY
    ${CMAKE_PROJECT_NAME}
    PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
