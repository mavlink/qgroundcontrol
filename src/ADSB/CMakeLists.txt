find_package(Qt6 REQUIRED COMPONENTS Core Network Positioning)

qt_add_library(ADSB STATIC
	ADSBVehicle.cc
	ADSBVehicle.h
	ADSBVehicleManager.cc
	ADSBVehicleManager.h
)

target_link_libraries(ADSB
	PUBLIC
		Qt6::Core
		Qt6::Network
		Qt6::Positioning
		comm
		qgc
		QmlControls
)

target_include_directories(ADSB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
