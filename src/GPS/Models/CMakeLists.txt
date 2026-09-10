qt_add_library(QGCGPSModels STATIC GPSSatelliteModel.cc GPSSatelliteModel.h GPSRelativePositionModel.cc
               GPSRelativePositionModel.h
)
target_include_directories(QGCGPSModels PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSModels
    PUBLIC QGCGPSCore
    PRIVATE QGCLogging
)
qt_extract_metatypes(QGCGPSModels)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSModels)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSPositionFactGroup.cc GPSPositionFactGroup.h GPSIntegrityFactGroup.cc
                                             GPSIntegrityFactGroup.h
)
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
