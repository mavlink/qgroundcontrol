qt_add_library(QGCGPSDriver STATIC GPSDriver.cc GPSReceiverConfigValidation.cc GPSReceiverConfigValidation.h)
target_sources(QGCGPSDriver PUBLIC FILE_SET HEADERS FILES GPSDriver.h)
target_link_libraries(
    QGCGPSDriver
    PUBLIC QGCGPSDriverReports QGCGPSReceiverConfig
    PRIVATE QGCGPSTransport QGCLoggingCategory
)

add_subdirectory(Px4)
qgc_add_px4_backend(QGCGPSDriver)
