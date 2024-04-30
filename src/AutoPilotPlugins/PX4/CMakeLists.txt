find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_add_library(PX4AutoPilotPlugin STATIC
	ActuatorComponent.cc
	ActuatorComponent.h
	AirframeComponent.cc
	AirframeComponent.h
	AirframeComponentAirframes.cc
	AirframeComponentAirframes.h
	AirframeComponentController.cc
	AirframeComponentController.h
	CameraComponent.cc
	CameraComponent.h
	FlightModesComponent.cc
	FlightModesComponent.h
	PowerComponent.cc
	PowerComponent.h
	PowerComponentController.cc
	PowerComponentController.h
	PX4AirframeLoader.cc
	PX4AirframeLoader.h
	PX4AutoPilotPlugin.cc
	PX4AutoPilotPlugin.h
	PX4FlightBehavior.cc
	PX4FlightBehavior.h
	PX4RadioComponent.cc
	PX4RadioComponent.h
	PX4SimpleFlightModesController.cc
	PX4SimpleFlightModesController.h
	PX4TuningComponent.cc
	PX4TuningComponent.h
	SafetyComponent.cc
	SafetyComponent.h
	SensorsComponent.cc
	SensorsComponent.h
	SensorsComponentController.cc
	SensorsComponentController.h
)

target_link_libraries(PX4AutoPilotPlugin
	PUBLIC
		Qt6::Core
		Qt6::Quick
		Actuators
		AutoPilotPlugins
		CommonAutoPilotPlugin
		FactControls
		FactSystem
		Vehicle
        VehicleSetup
    PRIVATE
		comm
        qgc
		Utilities
)

target_include_directories(PX4AutoPilotPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
