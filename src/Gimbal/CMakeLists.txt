
add_library(Gimbal
	GimbalController.cc
)

target_link_libraries(Gimbal
	PUBLIC
		qgc
)

target_include_directories(Gimbal
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)

