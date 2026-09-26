qt_add_library(
    QGCGPSDriver
    STATIC
    GPSDriver.cc
    GPSDecodedData.cc
    GPSDecodedData_p.h
    GPSSatelliteObservation.cc
    GPSSatelliteObservation.h
    GPSSatelliteState.h
)
target_sources(QGCGPSDriver PUBLIC FILE_SET HEADERS FILES GPSDriver.h)
target_link_libraries(
    QGCGPSDriver
    PUBLIC QGCGPSCommon QGCGPSProtocolContracts QGCGPSReceiverConfig
    PRIVATE QGCClock QGCGPSTransport QGCLoggingCategory QGCGPSProtocols
)
