# ============================================================================
# GPS Driver Module
# PX4 GPS driver integration + GNSS message structs
# ============================================================================

qt_add_library(
    QGCGPSDriver
    STATIC
    GPSDriver.cc
    GPSDriver.h
    GPSNMEAPreparation.cc
    GPSNMEAPreparation.h
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
# Pin PX4/PX4-GPSDrivers PR #238 until its receiver-mode fixes are merged.
CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY HTRamsey/PX4-GPSDrivers
    GIT_TAG b5a128be2b278122ea91dd884e1df0f460915926
    SOURCE_SUBDIR src
)

# NOTE: Using file(GLOB) for external dependency sources
# CONFIGURE_DEPENDS ensures CMake re-runs if files are added/removed
file(GLOB GPS_DRIVERS_SOURCES CONFIGURE_DEPENDS "${px4-gpsdrivers_SOURCE_DIR}/src/*.c"
     "${px4-gpsdrivers_SOURCE_DIR}/src/*.cpp" "${px4-gpsdrivers_SOURCE_DIR}/src/*.h"
)

add_library(px4-gpsdrivers OBJECT ${GPS_DRIVERS_SOURCES})
target_include_directories(px4-gpsdrivers SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)
target_compile_definitions(px4-gpsdrivers PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_link_libraries(px4-gpsdrivers PRIVATE Qt6::Core)
qgc_disable_dependency_warnings(px4-gpsdrivers)

target_link_libraries(
    QGCGPSDriver
    PUBLIC Qt6::Core
    PRIVATE Qt6::Network QGCLogging px4-gpsdrivers
)
target_compile_definitions(QGCGPSDriver PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_include_directories(QGCGPSDriver SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(QGCGPSDriver PRIVATE SerialGPSTransport.cc SerialGPSTransport.h)
    if(ANDROID)
        target_link_libraries(QGCGPSDriver PRIVATE QGCAndroidSerialPort)
    else()
        target_link_libraries(QGCGPSDriver PRIVATE Qt6::SerialPort)
    endif()
endif()
