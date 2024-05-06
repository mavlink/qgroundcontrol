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
    PRIVATE
		Comms
		FactSystem
        QGC
		Utilities
		Vehicle
        VehicleActuators
	PUBLIC
		Qt6::Core
		Qt6::Quick
		AutoPilotPlugins
		CommonAutoPilotPlugin
		FactControls
		MAVLink
        VehicleSetup
)

target_include_directories(PX4AutoPilotPlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(PX4AutoPilotPlugin "PX4AutoPilotPlugin_res" FILES AirframeFactMetaData.xml)

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Images/*.*)
# qt_add_qml_module(PX4AutoPilotPlugin
#     URI QGroundControl.PX4AutoPilotPlugin
#     VERSION 1.0
#     QML_FILES ${QML_SOURCES}
#     RESOURCES ${QML_RESOURCES}
#     OUTPUT_TARGETS PX4AutoPilotPlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Controls
#         QtQuick.Dialogs
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         QGroundControl.Controls
#         QGroundControl.ScreenTools
#         QGroundControl.Controllers
#         QGroundControl.Palette
#         QGroundControl.Vehicle
# )
