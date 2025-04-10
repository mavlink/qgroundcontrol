qt_add_plugin(QGCLocation
    STATIC
    CLASS_NAME QGeoServiceProviderFactoryQGC
    OUTPUT_TARGETS QGCLocation_targets
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/BingMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/BingMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/ElevationMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/ElevationMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/EsriMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/EsriMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/GenericMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/GenericMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/GoogleMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/GoogleMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/MapboxMapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/MapboxMapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/MapProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Providers/MapProvider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCachedTileSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCachedTileSet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCCacheTile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapTasks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapUrlEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCMapUrlEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCTile.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCTileCacheWorker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCTileCacheWorker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGCTileSet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoFileTileCacheQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoFileTileCacheQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoMapReplyQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoMapReplyQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoServiceProviderPluginQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoServiceProviderPluginQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTiledMappingManagerEngineQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTiledMappingManagerEngineQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTiledMapQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTiledMapQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTileFetcherQGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/QGeoTileFetcherQGC.h
    ${CMAKE_CURRENT_SOURCE_DIR}/QMLControl/QGCMapEngineManager.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/QMLControl/QGCMapEngineManager.h
)

target_link_libraries(QGCLocation
    PRIVATE
        Qt6::Positioning
        Qt6::Sql
    PUBLIC
        Qt6::Core
        Qt6::Location
        Qt6::LocationPrivate
        Qt6::Network
)

target_include_directories(QGCLocation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
        ${CMAKE_CURRENT_SOURCE_DIR}/QMLControl
        ${QGC_DIRECTORIES}
)

set_source_files_properties(QMLControl/OfflineMapEditor.qml PROPERTIES QT_RESOURCE_ALIAS OfflineMapEditor.qml)

set(QGC_LIBRARIES
    ${QGC_LIBRARIES}
    QGCLocation
    CACHE INTERNAL "" FORCE
)

# qt_add_qml_module(QGCLocation
#     URI QGroundControl.QGCLocation
#     VERSION 1.0
#     QML_FILES
#         QMLControl/OfflineMapEditor.qml
#     OUTPUT_TARGETS QGCLocation_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#       QtQuick
#       QtQuick.Controls
#       QtQuick.Layouts
#       QtQuick.Dialogs
#       QtQuick.Controls
#       QtLocation
#       QtPositioning

#       QGroundControl
#       QGroundControl.Controls
#       QGroundControl.ScreenTools
#       QGroundControl.Palette
#       QGroundControl.FlightMap
#       QGroundControl.QGCMapEngineManager
#       QGroundControl.FactSystem
#       QGroundControl.FactControls
# )
