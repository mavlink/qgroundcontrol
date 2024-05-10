find_package(Qt6 REQUIRED COMPONENTS Core Location Network Positioning Qml Sql)

qt_add_plugin(QGCLocation STATIC
    CLASS_NAME QGeoServiceProviderFactoryQGC
    OUTPUT_TARGETS QGCLocation_targets
	MapProviders/BingMapProvider.cpp
	MapProviders/BingMapProvider.h
	MapProviders/ElevationMapProvider.cpp
	MapProviders/ElevationMapProvider.h
	MapProviders/EsriMapProvider.cpp
	MapProviders/EsriMapProvider.h
	MapProviders/GenericMapProvider.cpp
	MapProviders/GenericMapProvider.h
	MapProviders/GoogleMapProvider.cpp
	MapProviders/GoogleMapProvider.h
	MapProviders/MapboxMapProvider.cpp
	MapProviders/MapboxMapProvider.h
	MapProviders/MapProvider.cpp
	MapProviders/MapProvider.h
	QGCCacheTile.h
	QGCMapEngine.cpp
	QGCMapEngine.h
	QGCMapEngineData.h
	QGCCachedTileSet.cpp
	QGCCachedTileSet.h
	QGCMapUrlEngine.cpp
	QGCMapUrlEngine.h
	QGCTile.h
	QGCTileCacheWorker.cpp
	QGCTileCacheWorker.h
	QGCTileSet.h
	QGeoFileTileCacheQGC.cpp
	QGeoFileTileCacheQGC.h
	QGeoMapReplyQGC.cpp
	QGeoMapReplyQGC.h
	QGeoServiceProviderPluginQGC.cpp
	QGeoServiceProviderPluginQGC.h
	QGeoTiledMappingManagerEngineQGC.cpp
	QGeoTiledMappingManagerEngineQGC.h
	QGeoTiledMapQGC.cpp
	QGeoTiledMapQGC.h
	QGeoTileFetcherQGC.cpp
	QGeoTileFetcherQGC.h
	QMLControl/QGCMapEngineManager.cc
	QMLControl/QGCMapEngineManager.h
)

target_compile_definitions(QGCLocation PRIVATE CMAKE_LOCATION_PLUGIN)
if(IOS)
	target_compile_definitions(QGCLocation PRIVATE QGC_NO_GOOGLE_MAPS)
endif()

target_link_libraries(QGCLocation
	PRIVATE
		Qt6::Positioning
		Settings
		Utilities
	PUBLIC
		Qt6::Core
		Qt6::Location
		Qt6::LocationPrivate
		Qt6::Network
		Qt6::Sql
		QGC
		QmlControls
)

target_include_directories(QGCLocation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		QMLControl
		MapProviders
)

set_source_files_properties(QMLControl/OfflineMap.qml
    PROPERTIES
        QT_RESOURCE_ALIAS OfflineMap.qml
)

set_source_files_properties(QMLControl/OfflineMapEditor.qml
    PROPERTIES
        QT_RESOURCE_ALIAS OfflineMapEditor.qml
)

# set(QGCLOCATION_RESOURCES)
# qt_add_resources(QGCLOCATION_RESOURCES MapProviders/maps.json)
# target_sources(QGCLocation PRIVATE ${QGCLOCATION_RESOURCES})

# qt_add_qml_module(QGCLocation
#     URI QGroundControl.QGCLocation
#     VERSION 1.0
#     QML_FILES
#         QMLControl/OfflineMap.qml
#         QMLControl/OfflineMapEditor.qml
#     OUTPUT_TARGETS QGCLocation_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
# 		QtQuick
# 		QtQuick.Controls
# 		QtQuick.Layouts
# 		QtQuick.Dialogs
# 		QtQuick.Controls
# 		QtLocation
# 		QtPositioning

# 		QGroundControl
# 		QGroundControl.Controls
# 		QGroundControl.ScreenTools
# 		QGroundControl.Palette
# 		QGroundControl.FlightMap
# 		QGroundControl.QGCMapEngineManager
# 		QGroundControl.FactSystem
# 		QGroundControl.FactControls
# )
