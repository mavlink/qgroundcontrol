# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for the QtLocation service provider plugin
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        QGeoTiledMappingManagerEngineQGCTest.cc
        QGeoTiledMappingManagerEngineQGCTest.h
        QGeoTiledMapReplyQGCTest.cc
        QGeoTiledMapReplyQGCTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(QGeoTiledMappingManagerEngineQGCTest LABELS Unit)
add_qgc_test(QGeoTiledMapReplyQGCTest LABELS Unit)
