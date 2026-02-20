# ============================================================================
# Terrain Module
# Provides terrain elevation data and queries
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        Providers/TerrainQueryArduPilot.cc
        Providers/TerrainQueryArduPilot.h
        Providers/TerrainQueryCopernicus.cc
        Providers/TerrainQueryCopernicus.h
        Providers/TerrainQueryOpenElevation.cc
        Providers/TerrainQueryOpenElevation.h
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

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
)
