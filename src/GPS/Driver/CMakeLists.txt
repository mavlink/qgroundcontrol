# ============================================================================
# GPS Driver Module
# PX4 GPS driver interface, transport layer, and sensor data types
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        GPSProvider.cc
        GPSProvider.h
        GPSTransport.h
        GPSTypes.h
        satellite_info.h
        sensor_gnss_relative.h
        sensor_gps.h
)

if(NOT QGC_NO_SERIAL_LINK)
    target_sources(${CMAKE_PROJECT_NAME}
        PRIVATE
            SerialGPSTransport.cc
            SerialGPSTransport.h
    )
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ============================================================================
# PX4 GPS Drivers Integration
# ============================================================================

CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY PX4/PX4-GPSDrivers
    GIT_TAG 5ed844c374646bfa2b41461c909a3b4df1a17233
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

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PRIVATE ${px4-gpsdrivers_SOURCE_DIR}/src)
