# Local base station state and Facts.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSBaseStationState.cc GPSBaseStationState.h GPSBaseStationFactGroup.cc
                                             GPSBaseStationFactGroup.h GPSBaseReferenceSave.cc GPSBaseReferenceSave.h
)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Network)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSBaseStationFact.json
)
