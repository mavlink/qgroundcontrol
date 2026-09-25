# Local RTK receiver, worker, Facts, and connection policy.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver QGCGPSReceiverTransports)

target_sources(
    ${CMAKE_PROJECT_NAME}
    PRIVATE GPSProvider.cc
            GPSProvider.h
            GPSRtk.cc
            GPSRtk.h
            GPSRTKFactGroup.cc
            GPSRTKFactGroup.h
            RTKConnectionPolicy.cc
            RTKConnectionPolicy.h
            RTKConnectionTarget.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
