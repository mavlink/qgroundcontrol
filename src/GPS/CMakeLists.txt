find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(GPS STATIC)

if(QGC_NO_SERIAL_LINK)
	return()
endif()

find_package(Qt6 REQUIRED COMPONENTS QmlIntegration)

# include(FetchContent)
# FetchContent_Declare(gps_drivers
#     GIT_REPOSITORY https://github.com/PX4/PX4-GPSDrivers.git
#     GIT_TAG main
#     GIT_SHALLOW TRUE
# )
# FetchContent_GetProperties(gps_drivers)
# if(NOT gps_drivers_POPULATED)
# 	FetchContent_Populate(gps_drivers)
# 	add_subdirectory(${gps_drivers_SOURCE_DIR} ${gps_drivers_BINARY_DIR} EXCLUDE_FROM_ALL)
# endif()

qt_add_library(GPSDrivers STATIC
	definitions.h
	Drivers/src/ashtech.cpp
	Drivers/src/ashtech.h
	Drivers/src/base_station.h
	Drivers/src/crc.cpp
	Drivers/src/crc.h
	Drivers/src/emlid_reach.cpp
	Drivers/src/emlid_reach.h
	Drivers/src/femtomes.cpp
	Drivers/src/femtomes.h
	Drivers/src/gps_helper.cpp
	Drivers/src/gps_helper.h
	Drivers/src/mtk.cpp
	Drivers/src/mtk.h
	Drivers/src/rtcm.cpp
	Drivers/src/rtcm.h
	Drivers/src/sbf.cpp
	Drivers/src/sbf.h
	Drivers/src/ubx.cpp
	Drivers/src/ubx.h
	Drivers/src/unicore.cpp
	Drivers/src/unicore.h
)

target_link_libraries(GPSDrivers
	PUBLIC
		Qt6::Core
		Utilities
)

target_sources(GPS
	PRIVATE
		GPSManager.cc
		GPSManager.h
		GPSProvider.cc
		GPSProvider.h
		GPSRtk.cc
		GPSRtk.h
		RTCMMavlink.cc
		RTCMMavlink.h
		satellite_info.h
		sensor_gnss_relative.h
		sensor_gps.h
)

target_link_libraries(GPS
	PRIVATE
		GPSDrivers
		MAVLink
		Settings
		Utilities
		Vehicle
	PUBLIC
		Qt6::Core
		Qt6::QmlIntegration
)

target_include_directories(GPS
	PRIVATE
		Drivers/src
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)
