find_package(Qt6 REQUIRED COMPONENTS Core Location Network Qml Sql)

# QGC_NO_GOOGLE_MAPS

qt_add_plugin(QGCLocation STATIC
    CLASS_NAME QGeoServiceProviderFactoryQGC
    OUTPUT_TARGETS QGCLocation_targets
	BingMapProvider.cpp
	BingMapProvider.h
	ElevationMapProvider.cpp
	ElevationMapProvider.h
	EsriMapProvider.cpp
	EsriMapProvider.h
	GenericMapProvider.cpp
	GenericMapProvider.h
	GoogleMapProvider.cpp
	GoogleMapProvider.h
	MapboxMapProvider.cpp
	MapboxMapProvider.h
	MapProvider.cpp
	MapProvider.h
	QGCMapEngine.cpp
	QGCMapEngine.h
	QGCMapEngineData.h
	QGCMapTileSet.cpp
	QGCMapTileSet.h
	QGCMapUrlEngine.cpp
	QGCMapUrlEngine.h
	QGCTileCacheWorker.cpp
	QGCTileCacheWorker.h
	QGCTileSet.h
	QGeoCodeReplyQGC.cpp
	QGeoCodeReplyQGC.h
	QGeoCodingManagerEngineQGC.cpp
	QGeoCodingManagerEngineQGC.h
	QGeoMapReplyQGC.cpp
	QGeoMapReplyQGC.h
	QGeoServiceProviderPluginQGC.cpp
	QGeoServiceProviderPluginQGC.h
	QGeoTiledMappingManagerEngineQGC.cpp
	QGeoTiledMappingManagerEngineQGC.h
	QGeoTileFetcherQGC.cpp
	QGeoTileFetcherQGC.h
	QMLControl/QGCMapEngineManager.cc
	QMLControl/QGCMapEngineManager.h
)

target_compile_definitions(QGCLocation PRIVATE CMAKE_LOCATION_PLUGIN)

target_link_libraries(QGCLocation
	PUBLIC
		Qt6::Core
		Qt6::Location
		Qt6::LocationPrivate
		Qt6::Network
		Qt6::Sql
		qgc
		QmlControls
)

target_include_directories(QGCLocation
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		QMLControl
)
