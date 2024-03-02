
qt_add_library(Camera STATIC
	MavlinkCameraControl.cc
	VehicleCameraControl.cc
	QGCCameraIO.cc
	QGCCameraManager.cc
	SimulatedCameraControl.cc
)

target_link_libraries(Camera
	PUBLIC
		qgc
)

target_include_directories(Camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

