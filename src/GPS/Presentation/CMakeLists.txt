target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSQmlTypes.h
            GPSPositionFactGroup.cc
            GPSPositionFactGroup.h
            GPSIntegrityFactGroup.cc
            GPSIntegrityFactGroup.h
            GPSReceiver.cc
            GPSReceiver.h
            GPSReceiverFactGroup.cc
            GPSReceiverFactGroup.h
            GPSReceiverSettingsPresentation.cc
            GPSReceiverSettingsPresentation.h
            GPSBaseStationState.cc
            GPSBaseStationState.h
            GPSBaseStationFactGroup.cc
            GPSBaseStationFactGroup.h
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
set_source_files_properties(GPSPositionFact.json GPSIntegrityFact.json PROPERTIES QT_RESOURCE_ALIAS GPSFact.json)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_position
    PREFIX "/json/GPS/Position"
    FILES GPSPositionFact.json
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_integrity
    PREFIX "/json/GPS/Integrity"
    FILES GPSIntegrityFact.json
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_receiver
    PREFIX "/json/Vehicle"
    FILES GPSReceiverFact.json
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSBaseStationFact.json
)
qt_add_library(GPSNTRIPModule STATIC)
qt_add_qml_module(
    GPSNTRIPModule
    URI QGroundControl.GPS.NTRIP
    VERSION 1.0
    RESOURCE_PREFIX /qml
    QML_FILES NTRIPConnectionStatus.qml NTRIPConnectionStatusRow.qml NTRIPMountpointList.qml
    NO_PLUGIN
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPSNTRIPModule)
