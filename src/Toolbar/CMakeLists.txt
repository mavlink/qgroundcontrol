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
        GPSIndicator.qml
        GPSIndicatorPage.qml
        GPSResilienceIndicator.qml
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
        RTKGPSIndicator.qml
        SelectViewDropdown.qml
        SignalStrength.qml
        TelemetryRSSIIndicator.qml
        VehicleGPSIndicator.qml
        VehicleMessageList.qml
    NO_PLUGIN
)

qt_add_resources(${CMAKE_PROJECT_NAME} toolbar_resources
    PREFIX "/res"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/FlyingPaperPlane.svg"
        "${CMAKE_SOURCE_DIR}/resources/GearWithPaperPlane.svg"
        "${CMAKE_SOURCE_DIR}/resources/OpenDoor.svg"
        "${CMAKE_SOURCE_DIR}/resources/VehicleMessages.png"
)

qt_add_resources(${CMAKE_PROJECT_NAME} toolbar_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_SOURCE_DIR}/src/Toolbar/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Analyze.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Armed.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Battery.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryYellowGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryYellow.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryOrange.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryCritical.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/BatteryEMERGENCY.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/CameraIcon.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Connect.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Disarmed.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Disconnect.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/EscIndicator.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Gears.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Gps.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/GpsAuthentication.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/GpsInterference.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Hamburger.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/HamburgerThin.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Joystick.png"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Megaphone.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/PaperPlane.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Plan.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Quad.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RC.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidEmergencyBackgroundHighlight.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidEmergencyBackground.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidFlagBackgroundGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidFlagBackgroundRed.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidIconManNoID.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidIconMan.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RidIconText.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/roi.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/RTK.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal0.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal100.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal20.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal40.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal60.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Signal80.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/TelemRSSI.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/TrackingIcon.svg"
        "${CMAKE_SOURCE_DIR}/src/Toolbar/Images/Yield.svg"
)

qt_add_resources(${CMAKE_PROJECT_NAME} toolbar_gcscontrol_resources
    PREFIX "/gcscontrolIndicator"
    BASE "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_device.svg"
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_gcs.svg"
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_line.svg"
)
