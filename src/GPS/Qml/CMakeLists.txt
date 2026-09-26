# ============================================================================
# GPS QML Module
# QML types of the GPS libraries, which build without QML, and shared GPS components.
# ============================================================================

qt_add_library(GPSModule STATIC)

set_source_files_properties(GPSFormat.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# Keep the NTRIP components at their historical qrc:/qml/QGroundControl/GPS/NTRIP/ URLs.
foreach(qml_file IN ITEMS NTRIPConnectionStatus.qml NTRIPMountpointList.qml RTCMMessageChips.qml)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS NTRIP/${qml_file})
endforeach()

qt_add_qml_module(
    GPSModule
    URI QGroundControl.GPS
    VERSION 1.0
    RESOURCE_PREFIX /qml
    SOURCES GPSQmlTypes.h
    QML_FILES GPSFormat.qml NTRIPConnectionStatus.qml NTRIPMountpointList.qml RTCMMessageChips.qml
)

target_link_libraries(
    GPSModule
    PRIVATE Qt6::Qml
            QGCGPSCorrections
            QGCGPSCorrectionService
            QGCGPSNTRIP
            QGCGPSNTRIPService
            QGCGPSPositioning
            QGCGPSReceiver
            QGCGPSReceiverConfig
            QGCGPSRTCM
)

qt_add_resources(
    GPSModule gps_qmlimages
    PREFIX "/qmlimages"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/Images"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPS.svg" "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSAuthentication.svg"
          "${CMAKE_CURRENT_SOURCE_DIR}/Images/GPSInterference.svg" "${CMAKE_CURRENT_SOURCE_DIR}/Images/RTK.svg"
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPSModule)
