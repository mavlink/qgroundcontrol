# ============================================================================
# QtLocationPlugin Unit Tests
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        QGCCachedTileSetTest.cc
        QGCCachedTileSetTest.h
        UrlFactoryTest.cc
        UrlFactoryTest.h
        TileDownloadIntegrationTest.cc
        TileDownloadIntegrationTest.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
