# ============================================================================
# GPS Driver Module
# PX4 GPS driver integration + GNSS message structs
# ============================================================================

qt_add_library(
    QGCGPSDriver
    STATIC
    GPSDriver.cc
    GPSDriver.h
    GPSDriverData.cc
    GPSDriverData.h
    GPSReceiverCapabilities.cc
    GPSReceiverCapabilities.h
    GPSTransport.cc
    GPSTransport.h
    TcpGPSTransport.cc
    TcpGPSTransport.h
    UdpGPSTransport.cc
    UdpGPSTransport.h
    GPSType.h
    definitions.h
    satellite_info.h
    sensor_gnss_relative.h
    sensor_gps.h
)

target_include_directories(QGCGPSDriver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# PX4 GPS Drivers Integration
# ----------------------------------------------------------------------------
# Build the vendored receiver sources, including the local u-blox NMEA changes.
file(GLOB GPS_DRIVERS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/PX4/*.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/PX4/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/PX4/*.h"
)

add_library(px4-gpsdrivers OBJECT ${GPS_DRIVERS_SOURCES})
target_include_directories(px4-gpsdrivers SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PX4)
target_compile_definitions(px4-gpsdrivers PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_link_libraries(px4-gpsdrivers PRIVATE Qt6::Core)
qgc_disable_dependency_warnings(px4-gpsdrivers)

target_link_libraries(
    QGCGPSDriver
    PUBLIC Qt6::Core QGCGPSCore
    PRIVATE Qt6::Network QGCLogging px4-gpsdrivers
)
target_compile_definitions(QGCGPSDriver PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_include_directories(QGCGPSDriver SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/PX4)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(QGCGPSDriver PRIVATE SerialGPSTransport.cc SerialGPSTransport.h)
    if(ANDROID)
        target_link_libraries(QGCGPSDriver PRIVATE QGCAndroidSerialPort)
    else()
        target_link_libraries(QGCGPSDriver PRIVATE Qt6::SerialPort)
    endif()
endif()
