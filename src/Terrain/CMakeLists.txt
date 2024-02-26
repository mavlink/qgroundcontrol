
qt_add_library(Terrain STATIC
	TerrainQuery.cc
)

target_link_libraries(Terrain
	PRIVATE
		qgc
)

target_include_directories(Terrain
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
	)

