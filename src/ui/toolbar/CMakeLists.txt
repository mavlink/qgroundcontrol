find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(UiToolbarQml
	SOURCES
		ArmedIndicator.qml
		BatteryIndicator.qml
		FlightModeMenuIndicator.qml
		GPSAuthenticationIndicator.qml
		GPSInterferenceIndicator.qml
		GPSIndicator.qml
		GPSIndicatorPage.qml
		GPSRTKIndicator.qml
		JoystickIndicator.qml
		LinkIndicator.qml
		MainStatusIndicator.qml
		MessageIndicator.qml
		ModeIndicator.qml
		MultiVehicleSelector.qml
		RCRSSIIndicator.qml
		RemoteIDIndicator.qml
		RemoteIDIndicatorPage.qml
		SignalStrength.qml
		TelemetryRSSIIndicator.qml
        FlyViewToolBar.qml
        PlanViewToolBar.qml
)
