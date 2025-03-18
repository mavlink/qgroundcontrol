find_package(Qt6 REQUIRED COMPONENTS Core)

if(QGC_NO_SERIAL_LINK)
    return()
endif()

target_sources(QGC
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

target_link_libraries(QGC
    PUBLIC
        Qt6::Core
)

target_include_directories(QGC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

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
target_sources(QGC
    PRIVATE
        definitions.h
        ${GPS_DRIVERS_SOURCES}
)

target_link_libraries(QGC PUBLIC Qt6::Core)

target_compile_definitions(QGC PUBLIC GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

target_include_directories(QGC PUBLIC ${px4-gpsdrivers_SOURCE_DIR}/src)
