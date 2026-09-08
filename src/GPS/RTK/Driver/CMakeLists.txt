# ============================================================================
# GPS Driver Module
# PX4 GPS driver integration + GNSS message structs
# ============================================================================

qt_add_library(
    QGCGPSDriver
    STATIC
    GPSDriver.cc
    GPSDriver.h
    GPSTransport.cc
    GPSTransport.h
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
include(CPMPatchCache)

set(_gpsdrivers_revision 056cfb18e826a688141a8fe5354982273d43215f)
set(_gpsdrivers_patch "${CMAKE_CURRENT_LIST_DIR}/px4-gpsdrivers-utc.patch")
qgc_cpm_patch_cache_key(
    _gpsdrivers_cache_key
    GIT_REPOSITORY
    "https://github.com/PX4/PX4-GPSDrivers.git"
    GIT_TAG
    "${_gpsdrivers_revision}"
    PATCHES
    "${_gpsdrivers_patch}"
)

CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY PX4/PX4-GPSDrivers
    GIT_TAG ${_gpsdrivers_revision}
    CUSTOM_CACHE_KEY "${_gpsdrivers_cache_key}"
    PATCHES "${_gpsdrivers_patch}"
    SOURCE_SUBDIR src
)

# NOTE: Using file(GLOB) for external dependency sources
# CONFIGURE_DEPENDS ensures CMake re-runs if files are added/removed
file(GLOB GPS_DRIVERS_SOURCES
    CONFIGURE_DEPENDS
    "${px4-gpsdrivers_SOURCE_DIR}/src/*.c"
    "${px4-gpsdrivers_SOURCE_DIR}/src/*.cpp"
    "${px4-gpsdrivers_SOURCE_DIR}/src/*.h"
)

add_library(px4-gpsdrivers OBJECT ${GPS_DRIVERS_SOURCES})
target_include_directories(px4-gpsdrivers SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)
target_compile_definitions(px4-gpsdrivers PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_link_libraries(px4-gpsdrivers PRIVATE Qt6::Core)
qgc_disable_dependency_warnings(px4-gpsdrivers)

target_link_libraries(
    QGCGPSDriver
    PUBLIC Qt6::Core
    PRIVATE QGCLogging px4-gpsdrivers
)
target_compile_definitions(QGCGPSDriver PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)
target_include_directories(QGCGPSDriver SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)
