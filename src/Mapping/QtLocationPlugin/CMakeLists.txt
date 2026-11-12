# ============================================================================
# Qt Location Plugin
# QtLocation service provider that shows the map tiles in a QtLocation map
# ============================================================================

qt_add_plugin(
    QGCLocation
    STATIC
    CLASS_NAME
    QGeoServiceProviderFactoryQGC
    OUTPUT_TARGETS
    QGCLocation_targets
    QGeoFileTileCacheQGC.cc
    QGeoFileTileCacheQGC.h
    QGeoMapIdQGC.cc
    QGeoMapIdQGC.h
    QGeoServiceProviderPluginQGC.cc
    QGeoServiceProviderPluginQGC.h
    QGeoTiledMappingManagerEngineQGC.cc
    QGeoTiledMappingManagerEngineQGC.h
    QGeoTiledMapQGC.cc
    QGeoTiledMapQGC.h
    QGeoTiledMapReplyQGC.cc
    QGeoTiledMapReplyQGC.h
    QGeoTileFetcherQGC.cc
    QGeoTileFetcherQGC.h
)

target_link_libraries(
    QGCLocation
    PRIVATE
        QGCFileSystem
        QGCLogging
        QGCMapTiles
        QGCNetwork
    PUBLIC
        Qt6::Core
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Network
)

target_include_directories(
    QGCLocation
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../FactSystem
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Settings
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCLocation)
