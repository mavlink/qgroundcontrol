
qt_add_library(FollowMe STATIC
	FollowMe.cc
)

target_link_libraries(FollowMe
	PUBLIC
		qgc
)

target_include_directories(FollowMe
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)

