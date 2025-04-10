set(QGC_SOURCES
    ${QGC_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/TerrainQueryCopernicus.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/TerrainQueryCopernicus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/TerrainTileCopernicus.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/TerrainTileCopernicus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainQuery.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainQuery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainQueryInterface.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainQueryInterface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTile.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTileManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/TerrainTileManager.h
    CACHE INTERNAL "" FORCE
)

set(QGC_DIRECTORIES
    ${QGC_DIRECTORIES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers
    CACHE INTERNAL "" FORCE
)
