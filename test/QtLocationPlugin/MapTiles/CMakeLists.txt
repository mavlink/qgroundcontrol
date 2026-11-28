# ============================================================================
# MapTiles Unit Tests
# Tests for the map providers, the tile cache, the fetching of tiles, offline
# tile sets and credits
# ============================================================================

target_sources(
    QGCUnitTests
    PRIVATE
        Cache/CacheWorkerTest.cc
        Cache/CacheWorkerTest.h
        Cache/TileCacheDatabaseTest.cc
        Cache/TileCacheDatabaseTest.h
        Cache/TileCacheTest.cc
        Cache/TileCacheTest.h
        Cache/TileCacheTestHelpers.h
        Credits/EsriAttributionTest.cc
        Credits/EsriAttributionTest.h
        Credits/MapCreditsTest.cc
        Credits/MapCreditsTest.h
        Fetch/TileReplyNetworkMock.h
        Fetch/TileReplyTest.cc
        Fetch/TileReplyTest.h
        MapEngineTest.cc
        MapEngineTest.h
        OfflineMaps/CachedTileSetTest.cc
        OfflineMaps/CachedTileSetTest.h
        OfflineMaps/MapEngineManagerTest.cc
        OfflineMaps/MapEngineManagerTest.h
        OfflineMaps/TileSetDownloaderTest.cc
        OfflineMaps/TileSetDownloaderTest.h
        OfflineMaps/TileSetTestHelpers.h
        Providers/CopernicusTileFormatTest.cc
        Providers/CopernicusTileFormatTest.h
        Providers/MapCatalogTest.cc
        Providers/MapCatalogTest.h
        Providers/MapProviderRegistryTest.cc
        Providers/MapProviderRegistryTest.h
        Providers/MapProviderTest.cc
        Providers/MapProviderTest.h
        Providers/TileRegionTest.cc
        Providers/TileRegionTest.h
)

# For what tests of other folders use too: TileCacheTestHelpers.h, and TileReplyNetworkMock.h with the network stand-in
target_include_directories(
    QGCUnitTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Cache
        ${CMAKE_CURRENT_SOURCE_DIR}/Fetch
)

add_qgc_test(CacheWorkerTest LABELS Unit)
add_qgc_test(CachedTileSetTest LABELS Unit)
add_qgc_test(CopernicusTileFormatTest LABELS Unit)
add_qgc_test(EsriAttributionTest LABELS Unit)
add_qgc_test(MapCatalogTest LABELS Unit)
add_qgc_test(MapCreditsTest LABELS Unit)
add_qgc_test(MapEngineManagerTest LABELS Unit)
add_qgc_test(MapEngineTest LABELS Unit)
add_qgc_test(MapProviderRegistryTest LABELS Unit)
add_qgc_test(MapProviderTest LABELS Unit)
add_qgc_test(TileCacheDatabaseTest LABELS Unit)
add_qgc_test(TileCacheTest LABELS Unit)
add_qgc_test(TileRegionTest LABELS Unit)
add_qgc_test(TileReplyTest LABELS Unit)
add_qgc_test(TileSetDownloaderTest LABELS Unit)
