# ============================================================================
# GPS QML Module
# Shared GPS components, and the QML registration of QGCGPSCore's fix types: the library builds without QML. The GPS
# services register their own types in the application module.
# ============================================================================

qt_add_library(GPSModule STATIC)

set_source_files_properties(
    GPSUnits.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(
    GPSModule
    URI QGroundControl.GPS
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES
        GPSQmlTypes.h
    DEPENDENCIES
        QtPositioning
    QML_FILES
        CorrectionsLogSwitch.qml
        CorrectionsStatus.qml
        GcsPositionStatus.qml
        GPSConstellationPalette.qml
        GPSFactComboField.qml
        GPSFactField.qml
        GPSFactSwitchField.qml
        GPSFactTextField.qml
        GPSField.qml
        GPSReceiverBasePosition.qml
        GPSReceiverBaseSites.qml
        GPSReceiverBaseStation.qml
        GPSReceiverConnectControls.qml
        GPSReceiverConnection.qml
        GPSReceiverConnectionRow.qml
        GPSReceiverRecording.qml
        GPSReceiverSerialPort.qml
        GPSReceiverSettings.qml
        GPSReceiverSettingsGroup.qml
        GPSReceiverStatus.qml
        GPSReceiverSurveyProgress.qml
        GPSSatelliteSignalBars.qml
        GPSSatelliteView.qml
        GPSSkyPlot.qml
        GPSUnits.qml
        GPSWrappingRow.qml
        NTRIPConnectionStatus.qml
        NTRIPMountpointBrowser.qml
        NTRIPMountpointFilter.qml
        NTRIPMountpointList.qml
        NTRIPPasswordField.qml
        RTCMMessageChips.qml
)

target_link_libraries(
    GPSModule
    PRIVATE
        Qt6::Qml
        QGCGPSCore
)

qt_add_resources(
    GPSModule gps_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Images"
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPS.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSAuthentication.svg"
        "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSInterference.svg"
)
