qt_add_library(
    QGCGPSReceiver
    STATIC
    GPSSatelliteModel.cc
    GPSSatelliteModel.h
    GPSRelativePositionModel.cc
    GPSRelativePositionModel.h
    GPSConnectionConfig.cc
    GPSConnectionConfig.h
    GPSReceiverProfile.cc
    GPSReceiverProfile.h
    GPSReceiverTransportFactory.cc
    GPSReceiverTransportFactory.h
    GPSByteStream.cc
    GPSByteStream.h
    GPSProvider.cc
    GPSProvider.h
    GPSReceiverPositionSource.cc
    GPSReceiverPositionSource.h
    GPSReceiverMailbox.cc
    GPSReceiverMailbox.h
    GPSReceiverSession.cc
    GPSReceiverSession.h
)

target_include_directories(QGCGPSReceiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    QGCGPSReceiver
    PUBLIC QGCGPSDriver QGCGPSCore QGCGPSCorrections QGCGPSRecording
    PRIVATE QGCLogging
)

# Discovery integrates the application serial-port registry; worker sessions remain in the standalone library.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverAutoConnect.cc GPSReceiverAutoConnect.h)
if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiverAutoConnectSerial.cc)
endif()

# Application presentation uses Facts; the receiver worker library stays independent of the UI.
target_sources(${CMAKE_PROJECT_NAME} PRIVATE GPSReceiver.cc GPSReceiver.h GPSReceiverFactGroup.cc
                                             GPSReceiverFactGroup.h
)
qt_add_resources(
    ${CMAKE_PROJECT_NAME} json_gps_receiver
    PREFIX "/json/Vehicle"
    FILES GPSReceiverFact.json
)
