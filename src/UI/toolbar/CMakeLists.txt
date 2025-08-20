qt_add_library(ToolbarModule STATIC)

qt_add_qml_module(ToolbarModule
    URI QGroundControl.Toolbar
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        APMBatteryIndicator.qml
        APMFlightModeIndicator.qml
        APMMainStatusIndicatorContentItem.qml
        APMSupportForwardingIndicator.qml
        ArmedIndicator.qml
        EscIndicator.qml
        EscIndicatorPage.qml
        GCSControlIndicator.qml
        GimbalIndicator.qml
        JoystickIndicator.qml
        LinkIndicator.qml
        ModeIndicator.qml
        MultiVehicleSelector.qml
        RCRSSIIndicator.qml
        RemoteIDIndicator.qml
        RTKGPSIndicator.qml
        TelemetryRSSIIndicator.qml
        VehicleGPSIndicator.qml
    NO_PLUGIN
)
