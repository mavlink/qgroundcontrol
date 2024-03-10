find_package(Qt6 REQUIRED COMPONENTS Core Qml)

add_custom_target(APMFirmwarePluginQml
    SOURCES
        APMBatteryIndicator.qml
        APMFlightModeIndicator.qml
        APMMainStatusIndicatorContentItem.qml
        APMSensorParams.qml
)
