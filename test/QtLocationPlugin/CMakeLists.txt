# ============================================================================
# QtLocationPlugin Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        MapProviderTest.cc
        MapProviderTest.h
        MockMapProvider.h
        QGCCachedTileSetTest.cc
        QGCCachedTileSetTest.h
        QGCMapEngineManagerTest.cc
        QGCMapEngineManagerTest.h
        QGCTileCacheWorkerTest.cc
        QGCTileCacheWorkerTest.h
        TileDownloadIntegrationTest.cc
        TileDownloadIntegrationTest.h
        UrlFactoryTest.cc
        UrlFactoryTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
