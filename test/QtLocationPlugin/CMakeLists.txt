# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for tile cache database operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MapProviderTest.cc
        MapProviderTest.h
        QGCMapEngineManagerArchiveTest.cc
        QGCMapEngineManagerArchiveTest.h
        QGCCachedTileSetTest.cc
        QGCCachedTileSetTest.h
        QGCCacheWorkerTest.cc
        QGCCacheWorkerTest.h
        QGCTileCacheDatabaseTest.cc
        QGCTileCacheDatabaseTest.h
        QGCTileSetTest.cc
        QGCTileSetTest.h
        UrlFactoryTest.cc
        UrlFactoryTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_qgc_test(MapProviderTest LABELS Unit)
add_qgc_test(QGCCacheWorkerTest LABELS Unit)
add_qgc_test(QGCCachedTileSetTest LABELS Unit)
add_qgc_test(QGCMapEngineManagerArchiveTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(QGCTileCacheDatabaseTest LABELS Unit)
add_qgc_test(QGCTileSetTest LABELS Unit)
add_qgc_test(UrlFactoryTest LABELS Unit)
