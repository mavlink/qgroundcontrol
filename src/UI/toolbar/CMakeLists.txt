qt_add_library(ToolbarModule STATIC)

qt_add_qml_module(ToolbarModule
    URI QGroundControl.Toolbar
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ArmedIndicator.qml
        BatteryIndicator.qml
        EscIndicator.qml
        EscIndicatorPage.qml
        FlightModeIndicator.qml
        FlightModeMenuIndicator.qml
        FlyViewToolBar.qml
        FlyViewToolBarIndicators.qml
        GCSControlIndicator.qml
        GimbalIndicator.qml
        JoystickIndicator.qml
        MainStatusIndicator.qml
        MainStatusIndicatorOfflinePage.qml
        ModeIndicator.qml
        MultiVehicleSelector.qml
        ParameterDownloadProgress.qml
        PlanViewToolBar.qml
        RCRSSIIndicator.qml
        RemoteIDIndicator.qml
        RemoteIDIndicatorPage.qml
        SelectViewDropdown.qml
        SignalStrength.qml
        SigningIndicator.qml
        TelemetryRSSIIndicator.qml
        VehicleMessageList.qml
    NO_PLUGIN
)
