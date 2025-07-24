target_sources(${CMAKE_PROJECT_NAME}
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

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
