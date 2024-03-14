find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(UiPreferencesQml
	SOURCES
		BluetoothSettings.qml
		GeneralSettings.qml
		HelpSettings.qml
		LinkSettings.qml
		LogReplaySettings.qml
		PX4LogTransferSettings.qml
		SerialSettings.qml
		TcpSettings.qml
		UdpSettings.qml
        ADSBServerSettings.qml
        FlyViewSettings.qml
        MapSettings.qml
        OfflineMapInfo.qml
        PlanViewSettings.qml
        TelemetrySettings.qml
        VideoSettings.qml
)
