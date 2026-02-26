# ============================================================================
# GPS Module
# GPS/GNSS positioning and RTK support
# ============================================================================

# GPS requires serial link support
if(QGC_NO_SERIAL_LINK)
    return()
endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSManager.cc
        GPSManager.h
        GPSProvider.cc
        GPSProvider.h
        GPSRtk.cc
        GPSRtk.h
        GPSRTKFactGroup.cc
        GPSRTKFactGroup.h
        NTRIP.cc
        NTRIP.h
        RTCMMavlink.cc
        RTCMMavlink.h
        satellite_info.h
        sensor_gnss_relative.h
        sensor_gps.h
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# PX4 GPS Drivers Integration
# ============================================================================

CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY PX4/PX4-GPSDrivers
    GIT_TAG caf5158061bd10e79c9f042abb62c86bc6f3e7a7
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

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        definitions.h
        $<TARGET_OBJECTS:px4-gpsdrivers>
)

# ----------------------------------------------------------------------------
# GPS/RTK Configuration Resources
# ----------------------------------------------------------------------------
qt_add_resources(${CMAKE_PROJECT_NAME} json_gps_drivers
    PREFIX "/json/Vehicle"
    FILES GPSRTKFact.json
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)
