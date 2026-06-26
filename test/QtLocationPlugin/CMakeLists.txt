# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for tile cache database operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MapProviderTest.cc
        MapProviderTest.h
        MockMapProvider.h
        QGCMapEngineManagerArchiveTest.cc
        QGCMapEngineManagerArchiveTest.h
        QGCMapEngineManagerTest.cc
        QGCMapEngineManagerTest.h
        QGCMapTaskTest.cc
        QGCMapTaskTest.h
        QGCCachedTileSetTest.cc
        QGCCachedTileSetTest.h
        QGCCacheWorkerTest.cc
        QGCCacheWorkerTest.h
        QGCTileCacheDatabaseTest.cc
        QGCTileCacheDatabaseTest.h
        QGCTileSetTest.cc
        QGCTileSetTest.h
        QGCTileSetImporterTest.cc
        QGCTileSetImporterTest.h
        QGCTileFallbackTest.cc
        QGCTileFallbackTest.h
        QGCHostCircuitBreakerTest.cc
        QGCHostCircuitBreakerTest.h
        QGeoFileTileCacheQGCTest.cc
        QGeoFileTileCacheQGCTest.h
        QGeoTileFetcherQGCTest.cc
        QGeoTileFetcherQGCTest.h
        QGeoTiledMapReplyQGCTest.cc
        QGeoTiledMapReplyQGCTest.h
        QtLocationTestBase.h
        TileFetchMetricsTest.cc
        TileFetchMetricsTest.h
        UrlFactoryTest.cc
        UrlFactoryTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(MapProviderTest LABELS Unit)
add_qgc_test(QGCCacheWorkerTest LABELS Unit)
add_qgc_test(QGCCachedTileSetTest LABELS Unit)
add_qgc_test(QGCMapEngineManagerArchiveTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(QGCMapEngineManagerTest LABELS Unit)
add_qgc_test(QGCMapTaskTest LABELS Unit)
add_qgc_test(QGCTileCacheDatabaseTest LABELS Unit)
add_qgc_test(QGCTileSetTest LABELS Unit)
add_qgc_test(QGCTileSetImporterTest LABELS Unit)
add_qgc_test(QGCTileFallbackTest LABELS Unit)
add_qgc_test(QGCHostCircuitBreakerTest LABELS Unit)
add_qgc_test(QGeoFileTileCacheQGCTest LABELS Unit)
add_qgc_test(QGeoTileFetcherQGCTest LABELS Unit)
add_qgc_test(QGeoTiledMapReplyQGCTest LABELS Unit)
add_qgc_test(TileFetchMetricsTest LABELS Unit)
add_qgc_test(UrlFactoryTest LABELS Unit)
