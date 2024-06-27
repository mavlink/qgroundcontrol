find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_add_library(VehicleActuators STATIC
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

target_link_libraries(VehicleActuators
    PRIVATE
        QGC
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Comms
        FactSystem
        QmlControls
        Vehicle
)

target_include_directories(VehicleActuators PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
