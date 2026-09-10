add_subdirectory(Transport)
qgc_add_gps_component(QGCGPSNative COMPONENT Native NATIVE_DEFINITIONS_HEADER
                      "${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverPlatform.h"
)
qgc_add_gps_component(
    QGCGPSDriver
    COMPONENT
    Driver
    LOGGING_TARGET
    QGCLogging
    NATIVE_DEFINITIONS_HEADER
    "${CMAKE_CURRENT_SOURCE_DIR}/GPSDriverPlatform.h"
)
