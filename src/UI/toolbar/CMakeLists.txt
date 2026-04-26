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
        SigningIndicator.qml
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
    BASE "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images"
    FILES
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Analyze.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Armed.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Battery.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryYellowGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryYellow.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryOrange.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryCritical.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/BatteryEMERGENCY.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/CameraIcon.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Connect.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Disarmed.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Disconnect.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/EscIndicator.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Gears.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Gps.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/GpsAuthentication.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/GpsInterference.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Hamburger.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/HamburgerThin.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Joystick.png"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Megaphone.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/PaperPlane.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Plan.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Quad.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RC.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidEmergencyBackgroundHighlight.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidEmergencyBackground.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidFlagBackgroundGreen.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidFlagBackgroundRed.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidIconManNoID.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidIconMan.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RidIconText.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/roi.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/RTK.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal0.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal100.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal20.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal40.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal60.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Signal80.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/TelemRSSI.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/TrackingIcon.svg"
        "${CMAKE_SOURCE_DIR}/src/UI/toolbar/Images/Yield.svg"
)

qt_add_resources(${CMAKE_PROJECT_NAME} toolbar_gcscontrol_resources
    PREFIX "/gcscontrolIndicator"
    BASE "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_device.svg"
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_gcs.svg"
        "${CMAKE_SOURCE_DIR}/resources/gcscontrolIndicator/gcscontrol_line.svg"
)
