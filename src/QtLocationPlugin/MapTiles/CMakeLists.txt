# ============================================================================
# Map Tiles Library
# Map tile providers, the tile cache and the fetching of tiles. Must not depend on QtLocation: it
# stays when the QtLocation plugin (the folder above) is removed.
# ============================================================================

qt_add_library(
    QGCMapTiles STATIC
    Cache/CacheTask.h
    Cache/CacheTasks.cc
    Cache/CacheTasks.h
    Cache/CacheTile.h
    Cache/CacheWorker.cc
    Cache/CacheWorker.h
    Cache/DownloadTile.h
    Cache/TileCache.cc
    Cache/TileCache.h
    Cache/TileCacheDatabase.cc
    Cache/TileCacheDatabase.h
    Cache/TileCacheDatabaseImportExport.cc
    Cache/TileCacheDatabaseSchema.cc
    Cache/TileCacheDatabaseSets.cc
    Cache/TileCacheTypes.h
    Credits/EsriAttribution.cc
    Credits/EsriAttribution.h
    Credits/MapCredits.cc
    Credits/MapCredits.h
    Fetch/TileReply.cc
    Fetch/TileReply.h
    Fetch/TileServerPauses.cc
    Fetch/TileServerPauses.h
    MapEngine.cc
    MapEngine.h
    MapTilesApp.cc
    MapTilesApp.h
    MapTilesSettings.cc
    MapTilesSettings.h
    OfflineMaps/CachedTileSet.cc
    OfflineMaps/CachedTileSet.h
    OfflineMaps/MapEngineManager.cc
    OfflineMaps/MapEngineManager.h
    OfflineMaps/TileSetDownloader.cc
    OfflineMaps/TileSetDownloader.h
    OfflineMaps/TileSetPlan.cc
    OfflineMaps/TileSetPlan.h
    OfflineMaps/TileSetTasks.cc
    OfflineMaps/TileSetTasks.h
    Providers/CopernicusTileFormat.cc
    Providers/CopernicusTileFormat.h
    Providers/ElevationMapProvider.cc
    Providers/ElevationMapProvider.h
    Providers/MapCatalog.cc
    Providers/MapCatalog.h
    Providers/MapProvider.cc
    Providers/MapProvider.h
    Providers/MapProviderRegistry.cc
    Providers/MapProviderRegistry.h
    Providers/MapProviderTable.cc
    Providers/MapProviderTable.h
    Providers/TileRegion.h
    Providers/WebMercator.h
)

# For the enum of MapEngineManager, which the offline map settings name
qt_add_qml_module(
    QGCMapTiles
    URI QGroundControl.MapEngineManager
    VERSION 1.0
    RESOURCE_PREFIX /qml
)

if(IOS)
    target_compile_definitions(QGCMapTiles PRIVATE QGC_NO_GOOGLE_MAPS)
endif()

if(WIN32)
    # The min and max macros of <windows.h>, which a header can pull in, break std::min and std::max
    target_compile_definitions(QGCMapTiles PRIVATE NOMINMAX)
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
        QGCParsing
        QGCTiming
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
        ${CMAKE_CURRENT_SOURCE_DIR}/Fetch
        ${CMAKE_CURRENT_SOURCE_DIR}/OfflineMaps
        ${CMAKE_CURRENT_SOURCE_DIR}/Providers
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../FactSystem
        ${CMAKE_CURRENT_SOURCE_DIR}/../../QmlControls
)

# The settings of the application are read in one file, and its messages and its test mode are asked for in
# another: each alone gets the headers for it. A source with include directories of its own stays out of a unity
# build.
set_source_files_properties(
    MapTilesSettings.cc
    PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../../Settings"
)
set_source_files_properties(
    MapTilesApp.cc
    PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities"
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
