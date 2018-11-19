
add_library(Camera
	QGCCameraControl.cc
	QGCCameraIO.cc
	QGCCameraManager.cc
)

target_link_libraries(Camera
	PUBLIC
		qgc
)

target_include_directories(Camera PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

