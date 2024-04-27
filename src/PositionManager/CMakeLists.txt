find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(PositionManager STATIC
	PositionManager.cpp
	PositionManager.h
	SimulatedPosition.cc
	SimulatedPosition.h
)

target_link_libraries(PositionManager
	PRIVATE
		Qt6::PositioningPrivate
		api
		Vehicle
	PUBLIC
		Qt6::Core
		Qt6::Positioning
		qgc
)

target_include_directories(PositionManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
