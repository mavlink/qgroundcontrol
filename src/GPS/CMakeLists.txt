find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(GPS STATIC)

if(NOT QGC_NO_SERIAL_LINK)
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

	target_sources(GPS
		PRIVATE
			definitions.h
			Drivers/src/ashtech.cpp
			Drivers/src/ashtech.h
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
			Drivers/src
		INTERFACE
			${CMAKE_CURRENT_SOURCE_DIR}
	)
endif()
