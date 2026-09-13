target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSPositionQmlTypes.h
            GPSPositionFactGroup.cc
            GPSPositionFactGroup.h
            GPSIntegrityFactGroup.cc
            GPSIntegrityFactGroup.h
            GPSReceiverFactGroup.cc
            GPSReceiverFactGroup.h
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
    ${CMAKE_PROJECT_NAME} json_gps_base_station
    PREFIX "/json/Vehicle"
    FILES GPSBaseStationFact.json
)
