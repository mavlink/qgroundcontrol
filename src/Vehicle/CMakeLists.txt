
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		SendMavCommandTest.cc
	)
endif()

add_library(Vehicle
	ADSBVehicle.cc
	GPSRTKFactGroup.cc
	MAVLinkLogManager.cc
	MultiVehicleManager.cc
	Vehicle.cc
	${EXTRA_SRC}
)

target_link_libraries(Vehicle
        PRIVATE
            ui
	PUBLIC
		qgc
)

target_include_directories(Vehicle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
