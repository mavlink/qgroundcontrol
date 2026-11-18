# ============================================================================
# Map Tiles Library
# Map tile providers and the tile cache. Must not depend on QtLocation: it
# stays when the QtLocation plugin (the folder above) is removed.
# ============================================================================

qt_add_library(
    QGCMapTiles STATIC
    Cache/QGCCacheTile.h
    Cache/QGCCacheWorker.cc
    Cache/QGCCacheWorker.h
    Cache/QGCMapTask.h
    Cache/QGCMapTasks.cc
    Cache/QGCMapTasks.h
    Cache/QGCTile.h
    Cache/QGCTileCacheDatabase.cc
    Cache/QGCTileCacheDatabase.h
    Cache/QGCTileCacheDatabaseImportExport.cc
    Cache/QGCTileCacheDatabaseSchema.cc
    Cache/QGCTileCacheDatabaseSets.cc
    Cache/QGCTileCacheTypes.h
    Credits/EsriAttribution.cc
    Credits/EsriAttribution.h
    Credits/QGCMapCredits.cc
    Credits/QGCMapCredits.h
    OfflineMaps/QGCCachedTileSet.cc
    OfflineMaps/QGCCachedTileSet.h
    OfflineMaps/QGCMapEngineManager.cc
    OfflineMaps/QGCMapEngineManager.h
    OfflineMaps/QGCTileSetPlan.cc
    OfflineMaps/QGCTileSetPlan.h
    Providers/ElevationMapProvider.cc
    Providers/ElevationMapProvider.h
    Providers/MapProvider.cc
    Providers/MapProvider.h
    Providers/MapProviderTable.cc
    Providers/MapProviderTable.h
    Providers/QGCTileSet.h
    Providers/UrlFactory.cc
    Providers/UrlFactory.h
    QGCMapCatalog.cc
    QGCMapCatalog.h
    QGCMapEngine.cc
    QGCMapEngine.h
    QGCTileReply.cc
    QGCTileReply.h
)

if(IOS)
    target_compile_definitions(QGCMapTiles PRIVATE QGC_NO_GOOGLE_MAPS)
endif()

target_link_libraries(
    QGCMapTiles
    PRIVATE
        Qt6::Qml
        Qt6::Sql
        QGCCompression
        QGCDatabase
        QGCFileSystem
        QGCFormat
        QGCLogging
        QGCNetwork
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::QmlIntegration
)

target_include_directories(
    QGCMapTiles
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Cache
        ${CMAKE_CURRENT_SOURCE_DIR}/Credits
        ${CMAKE_CURRENT_SOURCE_DIR}/OfflineMaps
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../FactSystem
        ${CMAKE_CURRENT_SOURCE_DIR}/../../QmlControls
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Terrain
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Terrain/Providers
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Settings
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities
)

# Qt serves the headers of all its modules from one include root, so <QtLocation/...> resolves without
# linking Qt6::Location. Shadow the header that every QtLocation header includes, so using one here fails.
file(
    GENERATE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/QGCMapTiles_no_qtlocation/QtLocation/qlocationglobal.h"
    CONTENT "#error \"QGCMapTiles must not include QtLocation headers\"\n"
)
target_include_directories(QGCMapTiles BEFORE PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/QGCMapTiles_no_qtlocation")

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE QGCMapTiles)

target_precompile_headers(
    QGCMapTiles
    PRIVATE
        <QtCore/QDir>
        <QtCore/QLoggingCategory>
        <QtCore/QObject>
        <QtCore/QString>
        <QtCore/QStringList>
        <QtCore/QVariant>
        <QtNetwork/QNetworkRequest>
        <QtQml/QQmlEngine>
)
