find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(FollowMe STATIC
	FollowMe.cc
	FollowMe.h
)

target_link_libraries(FollowMe
	PUBLIC
		Qt6::Core
		Qt6::Positioning
		comm
		qgc
)

target_include_directories(FollowMe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
