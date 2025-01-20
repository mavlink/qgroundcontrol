find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(GPS STATIC)

if(QGC_NO_SERIAL_LINK)
    return()
endif()

target_sources(GPS
    PRIVATE
        GPSManager.cc
        GPSManager.h
        GPSProvider.cc
        GPSProvider.h
        GPSRtk.cc
        GPSRtk.h
        GPSRTKFactGroup.cc
        GPSRTKFactGroup.h
        RTCMMavlink.cc
        RTCMMavlink.h
        satellite_info.h
        sensor_gnss_relative.h
        sensor_gps.h
)

target_link_libraries(GPS
    PRIVATE
        Comms
        MAVLink
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        FactSystem
        GPSDrivers
)

target_include_directories(GPS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#===========================================================================#

message(STATUS "Building GPS Drivers")

include(CPM)
CPMAddPackage(
    NAME px4-gpsdrivers
    GITHUB_REPOSITORY PX4/PX4-GPSDrivers
    GIT_TAG main
    SOURCE_SUBDIR src
)

file(GLOB GPS_DRIVERS_SOURCES "${px4-gpsdrivers_SOURCE_DIR}/src/*")
qt_add_library(GPSDrivers STATIC
    definitions.h
    ${GPS_DRIVERS_SOURCES}
)

target_link_libraries(GPSDrivers PUBLIC Qt6::Core)

target_compile_definitions(GPSDrivers PUBLIC GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

target_include_directories(GPSDrivers PUBLIC ${px4-gpsdrivers_SOURCE_DIR}/src)

target_link_libraries(GPS PUBLIC GPSDrivers)
