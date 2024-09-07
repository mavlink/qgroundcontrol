find_package(Qt6 REQUIRED COMPONENTS Core Location Network Positioning)

qt_add_library(Terrain STATIC
    Providers/TerrainQueryArduPilot.cc
    Providers/TerrainQueryArduPilot.h
    Providers/TerrainQueryCopernicus.cc
    Providers/TerrainQueryCopernicus.h
    Providers/TerrainTileArduPilot.cc
    Providers/TerrainTileArduPilot.h
    Providers/TerrainTileCopernicus.cc
    Providers/TerrainTileCopernicus.h
    TerrainQuery.cc
    TerrainQuery.h
    TerrainQueryInterface.cc
    TerrainQueryInterface.h
    TerrainTile.cc
    TerrainTile.h
    TerrainTileManager.cc
    TerrainTileManager.h
)

target_link_libraries(Terrain
    PRIVATE
        Qt6::LocationPrivate
        Compression
        QGCLocation
        Utilities
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Positioning
)

target_include_directories(Terrain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
)
