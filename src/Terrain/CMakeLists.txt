find_package(Qt6 REQUIRED COMPONENTS Core Location Network Positioning)

qt_add_library(Terrain STATIC
    TerrainQuery.cc
    TerrainQuery.h
    TerrainQueryAirMap.cc
    TerrainQueryAirMap.h
    TerrainTile.cc
    TerrainTile.h
    TerrainTileManager.cc
    TerrainTileManager.h
)

target_link_libraries(Terrain
    PRIVATE
        Qt6::LocationPrivate
        QGCLocation
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Positioning
)

target_include_directories(Terrain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
