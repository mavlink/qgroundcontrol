find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(GPS STATIC)

if(QGC_NO_SERIAL_LINK)
    return()
endif()

include(FetchContent)
FetchContent_Declare(gps_drivers
    GIT_REPOSITORY https://github.com/PX4/PX4-GPSDrivers.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR src
    # SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/PX4-GPSDrivers
)
FetchContent_MakeAvailable(gps_drivers)

target_sources(GPS
    PRIVATE
        GPSManager.cc
        GPSManager.h
        GPSProvider.cc
        GPSProvider.h
        RTCMMavlink.cc
        RTCMMavlink.h
        definitions.h
        GPSPositionMessage.h
        satellite_info.h
        sensor_gnss_relative.h
        sensor_gps.h
        ${gps_drivers_SOURCE_DIR}/src/ashtech.cpp
        ${gps_drivers_SOURCE_DIR}/src/ashtech.h
        ${gps_drivers_SOURCE_DIR}/src/base_station.h
        ${gps_drivers_SOURCE_DIR}/src/crc.cpp
        ${gps_drivers_SOURCE_DIR}/src/crc.h
        ${gps_drivers_SOURCE_DIR}/src/emlid_reach.cpp
        ${gps_drivers_SOURCE_DIR}/src/emlid_reach.h
        ${gps_drivers_SOURCE_DIR}/src/femtomes.cpp
        ${gps_drivers_SOURCE_DIR}/src/femtomes.h
        ${gps_drivers_SOURCE_DIR}/src/gps_helper.cpp
        ${gps_drivers_SOURCE_DIR}/src/gps_helper.h
        ${gps_drivers_SOURCE_DIR}/src/mtk.cpp
        ${gps_drivers_SOURCE_DIR}/src/mtk.h
        ${gps_drivers_SOURCE_DIR}/src/nmea.cpp
        ${gps_drivers_SOURCE_DIR}/src/nmea.h
        ${gps_drivers_SOURCE_DIR}/src/rtcm.cpp
        ${gps_drivers_SOURCE_DIR}/src/rtcm.h
        ${gps_drivers_SOURCE_DIR}/src/sbf.cpp
        ${gps_drivers_SOURCE_DIR}/src/sbf.h
        ${gps_drivers_SOURCE_DIR}/src/ubx.cpp
        ${gps_drivers_SOURCE_DIR}/src/ubx.h
        ${gps_drivers_SOURCE_DIR}/src/unicore.cpp
        ${gps_drivers_SOURCE_DIR}/src/unicore.h
)

target_link_libraries(GPS
    PRIVATE
        Settings
        Utilities
        Vehicle
    PUBLIC
        Qt6::Core
        MAVLink
        QGC
)

target_include_directories(GPS
    PRIVATE
        ${gps_drivers_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/PX4-GPSDrivers/include
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
