find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(GPS STATIC)

if(QGC_NO_SERIAL_LINK)
    return()
endif()

include(FetchContent)
FetchContent_Declare(PX4-GPSDrivers
    GIT_REPOSITORY https://github.com/PX4/PX4-GPSDrivers.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR src
)
FetchContent_MakeAvailable(PX4-GPSDrivers)

qt_add_library(GPSDrivers STATIC
    definitions.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/ashtech.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/ashtech.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/base_station.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/crc.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/crc.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/emlid_reach.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/emlid_reach.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/femtomes.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/femtomes.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/gps_helper.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/gps_helper.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/mtk.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/mtk.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/nmea.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/nmea.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/rtcm.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/rtcm.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/sbf.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/sbf.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/ubx.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/ubx.h
    ${px4-gpsdrivers_SOURCE_DIR}/src/unicore.cpp
    ${px4-gpsdrivers_SOURCE_DIR}/src/unicore.h
)

target_link_libraries(GPSDrivers
    PUBLIC
        Qt6::Core
        Utilities
)

target_include_directories(GPSDrivers PUBLIC ${px4-gpsdrivers_SOURCE_DIR})

target_compile_definitions(GPSDrivers PUBLIC GPS_DEFINITIONS_HEADER=<${CMAKE_CURRENT_SOURCE_DIR}/definitions.h>)

target_sources(GPS
    PRIVATE
        GPSManager.cc
        GPSManager.h
        GPSPositionMessage.h
        GPSProvider.cc
        GPSProvider.h
        RTCMMavlink.cc
        RTCMMavlink.h
        satellite_info.h
        sensor_gnss_relative.h
        sensor_gps.h
)

target_link_libraries(GPS
    PRIVATE
        Comms
        GPSDrivers
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        MAVLink
        QGC
)

target_include_directories(GPS PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
