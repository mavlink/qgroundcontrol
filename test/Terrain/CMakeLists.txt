# ============================================================================
# Terrain Unit Tests
# Tests for terrain elevation data and tile management
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ElevationProviderTest.cc
        ElevationProviderTest.h
        TerrainQueryTest.cc
        TerrainQueryTest.h
        TerrainTileTest.cc
        TerrainTileTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

qt_add_resources(${CMAKE_PROJECT_NAME} "TerrainTest"
    PREFIX "/"
    FILES
        # N00E006.hgt
        N00E006.hgt.zip
)
