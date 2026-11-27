# ============================================================================
# Qt Location Plugin
# QtLocation service provider that shows the map tiles in a QtLocation map.
# The tiles come from the library in MapTiles, which does not use QtLocation.
# ============================================================================

add_subdirectory(MapTiles)

qt_add_plugin(
    QGCLocation
    STATIC
    CLASS_NAME
    QGCGeoServiceProviderFactory
    OUTPUT_TARGETS
    QGCLocation_targets
    QGCGeoFileTileCache.cc
    QGCGeoFileTileCache.h
    QGCGeoServiceProviderPlugin.cc
    QGCGeoServiceProviderPlugin.h
    QGCGeoTiledMappingManagerEngine.cc
    QGCGeoTiledMappingManagerEngine.h
    QGCGeoTiledMap.cc
    QGCGeoTiledMap.h
    QGCGeoTiledMapReply.cc
    QGCGeoTiledMapReply.h
    QGCGeoTileFetcher.cc
    QGCGeoTileFetcher.h
    QtLocationMapId.cc
    QtLocationMapId.h
)

target_link_libraries(
    QGCLocation
    PRIVATE
        QGCLogging
        QGCMapTiles
        QGCNetwork
        QGCTiming
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Network
)

target_include_directories(QGCLocation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLocation)
