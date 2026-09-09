# Local RTK receiver presentation and Facts.

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSRtkState.cc GPSRtkState.h GPSRTKFactGroup.cc GPSRTKFactGroup.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Network)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
