
set(EXTRA_SRC)
if(BUILD_TESTING)
	list(APPEND EXTRA_SRC
		SendMavCommandTest.cc
		SendMavCommandTest.h
	)
endif()

add_library(Vehicle
	GPSRTKFactGroup.cc
	GPSRTKFactGroup.h
	MAVLinkLogManager.cc
	MAVLinkLogManager.h
	MultiVehicleManager.cc
	MultiVehicleManager.h
	TrajectoryPoints.cc
	TrajectoryPoints.h
	Vehicle.cc
	Vehicle.h
	VehicleObjectAvoidance.cc
	VehicleObjectAvoidance.h

	${EXTRA_SRC}
)

target_link_libraries(Vehicle
	PRIVATE
		ui
	PUBLIC
		qgc
)

target_include_directories(Vehicle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
