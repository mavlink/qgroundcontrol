
add_library(Actuators
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

target_link_libraries(Actuators PRIVATE
	qgc)

