
add_library(ADSB
	ADSBVehicle.cc
	ADSBVehicle.h
	ADSBVehicleManager.cc
	ADSBVehicleManager.h
)

target_link_libraries(ADSB
	PUBLIC
		qgc
)

target_include_directories(ADSB
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)


