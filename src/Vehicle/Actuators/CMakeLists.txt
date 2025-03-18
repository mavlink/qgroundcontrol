find_package(Qt6 REQUIRED COMPONENTS Core Gui)

target_sources(QGC
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

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
        Qt6::Gui
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
