find_package(Qt6 REQUIRED COMPONENTS Core Location Network Positioning)

qt_add_library(Terrain STATIC
	TerrainQuery.cc
	TerrainQuery.h
    TerrainTile.cc
    TerrainTile.h
)

target_link_libraries(Terrain
	PRIVATE
		Qt6::LocationPrivate
		qgc
	PUBLIC
		Qt6::Core
		Qt6::Network
		Qt6::Positioning
		Utilities
)

target_include_directories(Terrain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
