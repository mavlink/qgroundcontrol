find_package(Qt6 REQUIRED COMPONENTS Core Location Network Positioning)

target_sources(QGC
    PRIVATE
        Providers/TerrainQueryCopernicus.cc
        Providers/TerrainQueryCopernicus.h
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

target_link_libraries(QGC
    PRIVATE
        Qt6::LocationPrivate
        QGCLocation
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::Positioning
)

target_include_directories(QGC
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
)
