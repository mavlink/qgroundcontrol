# ============================================================================
# GPS QML Module
# Shared GPS components, and the QML types of QGCGPSCore's value types, which build without QML. The GPS services
# register their own types in the application module.
# ============================================================================

qt_add_library(GPSModule STATIC)

qt_add_qml_module(
    GPSModule
    URI QGroundControl.GPS
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES GPSQmlTypes.h
    DEPENDENCIES QtPositioning
    QML_FILES CorrectionsStatus.qml
              GcsPositionStatus.qml
              GPSField.qml
              GPSNoteLabel.qml
              GPSReceiverSerialPort.qml
              GPSReceiverSettings.qml
              GPSReceiverStatus.qml
              NTRIPConnectionStatus.qml
              NTRIPMountpointBrowser.qml
              NTRIPMountpointList.qml
              NTRIPPasswordField.qml
              RTCMMessageChips.qml
)

target_link_libraries(GPSModule PRIVATE Qt6::Qml QGCGPSCore)

qt_add_resources(
    GPSModule gps_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Images"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPS.svg" "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSAuthentication.svg"
          "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSInterference.svg"
)
