# Local RTK receiver, worker, Facts, and connection policy.
add_subdirectory(Driver)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCGPSDriver)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSProvider.cc GPSProvider.h GPSRtk.cc GPSRtk.h GPSRTKFactGroup.cc
                                             GPSRTKFactGroup.h
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE TcpGPSTransport.cc TcpGPSTransport.h RTKAutoConnect.cc RTKAutoConnect.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE UdpGPSTransport.cc UdpGPSTransport.h)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKPositionSource.cc RTKPositionSource.h)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Network)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE RTKAutoConnectSerial.cc SerialGPSTransport.cc SerialGPSTransport.h)
endif()

qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)
