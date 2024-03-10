find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(PX4AutoPilotPluginQml
	SOURCES
		AirframeComponent.qml
		AirframeComponentSummary.qml
		BatteryParams.qml
		CameraComponent.qml
		CameraComponentSummary.qml
		FlightModesComponentSummary.qml
		PowerComponent.qml
		PowerComponentSummary.qml
		PX4FlightModes.qml
		PX4RadioComponentSummary.qml
		PX4SimpleFlightModes.qml
		PX4TuningComponentCopter.qml
		PX4TuningComponentPlane.qml
		PX4TuningComponentVTOL.qml
		SafetyComponent.qml
		SafetyComponentSummary.qml
		SensorsComponent.qml
		SensorsComponentSummary.qml
		SensorsComponentSummaryFixedWing.qml
		SensorsSetup.qml
)
