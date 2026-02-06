# ============================================================================
# QtLocationPlugin Unit Tests
# Tests for tile cache database operations
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MapProviderTest.cc
        MapProviderTest.h
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
