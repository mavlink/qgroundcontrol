# ============================================================================
# Actuators Unit Tests
# Tests for motor assignment and actuator testing state machines
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ActuatorTestingStateMachineTest.cc
        ActuatorTestingStateMachineTest.h
        ActuatorTestingTest.cc
        ActuatorTestingTest.h
        MotorAssignmentStateMachineTest.cc
        MotorAssignmentStateMachineTest.h
        MotorAssignmentTest.cc
        MotorAssignmentTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
