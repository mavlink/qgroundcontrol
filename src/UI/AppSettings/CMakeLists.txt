qt_add_library(AppSettingsModule STATIC)

qt_add_qml_module(AppSettingsModule
    URI QGroundControl.AppSettings
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES
        ADSBServerSettings.qml
        BluetoothSettings.qml
        DebugWindow.qml
        FlyViewSettings.qml
        GeneralSettings.qml
        AboutSettings.qml
        HelpSettings.qml
        AboutTool.qml
        LinkSettings.qml
        LogReplaySettings.qml
        MapSettings.qml
        MockLink.qml
        MockLinkSettings.qml
        PlanViewSettings.qml
        PX4LogTransferSettings.qml
        QmlTest.qml
        RemoteIDSettings.qml
        ChangePassword.qml
        SerialSettings.qml
        SettingsPage.qml
        SettingsPagesModel.qml
        TcpSettings.qml
        TelemetrySettings.qml
        UdpSettings.qml
        VideoSettings.qml
    NO_PLUGIN
)
