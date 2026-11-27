# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for the QtLocation plugin. Those of the tile library are in MapTiles.
# ============================================================================

add_subdirectory(MapTiles)

target_sources(
    QGCUnitTests
    PRIVATE
        QGCGeoTiledMappingManagerEngineTest.cc
        QGCGeoTiledMappingManagerEngineTest.h
        QGCGeoTiledMapReplyTest.cc
        QGCGeoTiledMapReplyTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGCGeoTiledMappingManagerEngineTest LABELS Unit)
add_qgc_test(QGCGeoTiledMapReplyTest LABELS Unit)
