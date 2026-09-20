if(NOT TARGET QGCGPSNative)
    add_subdirectory(Protocols)
endif()
qt_add_library(
    QGCGPSDriver
    STATIC
    GPSDriver.cc
    GPSNativeData.cc
    GPSNativeData_p.h
    GPSReceiverConfigValidation.cc
    GPSReceiverConfigValidation.h
)
target_sources(QGCGPSDriver PUBLIC FILE_SET HEADERS FILES GPSDriver.h)
target_link_libraries(
    QGCGPSDriver
    PUBLIC QGCGPSReceiver
    PRIVATE QGCGPSTransport QGCLoggingCategory QGCGPSNative
)
