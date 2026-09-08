# Local RTK receiver, worker, Facts, and connection policy.
add_subdirectory(Driver)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSProvider.cc GPSProvider.h GPSRtk.cc GPSRtk.h GPSRTKFactGroup.cc
                                             GPSRTKFactGroup.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKAutoConnect.cc RTKAutoConnect.h SerialGPSTransport.cc
                                                 SerialGPSTransport.h
    )
endif()

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
