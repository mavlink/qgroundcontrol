if(NOT TARGET QGCGPSProtocols)
    add_subdirectory(Protocols)
endif()
qt_add_library(QGCGPSDriver STATIC GPSDriver.cc GPSDecodedData.cc GPSDecodedData_p.h)
target_sources(QGCGPSDriver PUBLIC FILE_SET HEADERS FILES GPSDriver.h)
target_link_libraries(
    QGCGPSDriver
    PUBLIC QGCGPSReceiver
    PRIVATE QGCGPSTransport QGCLoggingCategory QGCGPSProtocols
)
