find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(PX4FirmwarePluginQml
	SOURCES
        PX4FlightModeIndicator.qml
        PX4MainStatusIndicatorContentItem.qml
)
