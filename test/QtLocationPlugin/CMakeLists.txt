# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for the map providers, the tile cache, offline tile sets and the QtLocation plugin
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        EsriAttributionTest.cc
        EsriAttributionTest.h
        MapProviderTest.cc
        MapProviderTest.h
        QGCMapCatalogTest.cc
        QGCMapCatalogTest.h
        QGCMapCreditsTest.cc
        QGCMapCreditsTest.h
        QGCMapEngineManagerArchiveTest.cc
        QGCMapEngineManagerArchiveTest.h
        QGCMapEngineManagerTest.cc
        QGCMapEngineManagerTest.h
        QGCCachedTileSetTest.cc
        QGCCachedTileSetTest.h
        QGCCacheWorkerTest.cc
        QGCCacheWorkerTest.h
        QGCTileCacheDatabaseTest.cc
        QGCTileCacheDatabaseTest.h
        QGCTileReplyTest.cc
        QGCTileReplyTest.h
        QGCTileSetTest.cc
        QGCTileSetTest.h
        QGeoTiledMappingManagerEngineQGCTest.cc
        QGeoTiledMappingManagerEngineQGCTest.h
        QGeoTiledMapReplyQGCTest.cc
        QGeoTiledMapReplyQGCTest.h
        TileCacheTestHelpers.h
        TileReplyNetworkMock.h
        UrlFactoryTest.cc
        UrlFactoryTest.h
)

target_include_directories(QGCUnitTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(EsriAttributionTest LABELS Unit)
add_qgc_test(MapProviderTest LABELS Unit)
add_qgc_test(QGCCacheWorkerTest LABELS Unit)
add_qgc_test(QGCCachedTileSetTest LABELS Unit)
add_qgc_test(QGCMapCatalogTest LABELS Unit)
add_qgc_test(QGCMapCreditsTest LABELS Unit)
add_qgc_test(QGCMapEngineManagerArchiveTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(QGCMapEngineManagerTest LABELS Unit)
add_qgc_test(QGCTileCacheDatabaseTest LABELS Unit)
add_qgc_test(QGCTileReplyTest LABELS Unit)
add_qgc_test(QGCTileSetTest LABELS Unit)
add_qgc_test(QGeoTiledMappingManagerEngineQGCTest LABELS Unit)
add_qgc_test(QGeoTiledMapReplyQGCTest LABELS Unit)
add_qgc_test(UrlFactoryTest LABELS Unit)
