# ============================================================================
# MapTiles Unit Tests
# Tests for the map providers, the tile cache, offline tile sets and credits
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        Cache/QGCCacheWorkerTest.cc
        Cache/QGCCacheWorkerTest.h
        Cache/QGCTileCacheDatabaseTest.cc
        Cache/QGCTileCacheDatabaseTest.h
        Cache/TileCacheTestHelpers.h
        Credits/EsriAttributionTest.cc
        Credits/EsriAttributionTest.h
        Credits/QGCMapCreditsTest.cc
        Credits/QGCMapCreditsTest.h
        OfflineMaps/QGCCachedTileSetTest.cc
        OfflineMaps/QGCCachedTileSetTest.h
        OfflineMaps/QGCMapEngineManagerArchiveTest.cc
        OfflineMaps/QGCMapEngineManagerArchiveTest.h
        OfflineMaps/QGCMapEngineManagerTest.cc
        OfflineMaps/QGCMapEngineManagerTest.h
        Providers/MapProviderTest.cc
        Providers/MapProviderTest.h
        Providers/QGCTileSetTest.cc
        Providers/QGCTileSetTest.h
        Providers/UrlFactoryTest.cc
        Providers/UrlFactoryTest.h
        QGCMapCatalogTest.cc
        QGCMapCatalogTest.h
        QGCTileReplyTest.cc
        QGCTileReplyTest.h
        TileReplyNetworkMock.h
)

# For TileReplyNetworkMock.h, the network stand-in that tests of other folders use too
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
add_qgc_test(UrlFactoryTest LABELS Unit)
