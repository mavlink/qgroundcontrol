
add_library(QtLocationPlugin
	BingMapProvider.cpp
	ElevationMapProvider.cpp
	EsriMapProvider.cpp
	GenericMapProvider.cpp
	GoogleMapProvider.cpp
	MapboxMapProvider.cpp
	MapProvider.cpp
	QGCMapEngine.cpp
	QGCMapTileSet.cpp
	QGCMapUrlEngine.cpp
	QGCTileCacheWorker.cpp
	QGeoCodeReplyQGC.cpp
	QGeoCodingManagerEngineQGC.cpp
	QGeoMapReplyQGC.cpp
	QGeoServiceProviderPluginQGC.cpp
	QGeoTiledMappingManagerEngineQGC.cpp
	QGeoTileFetcherQGC.cpp

	QMLControl/QGCMapEngineManager.cc

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	QGCMapEngineData.h
)

target_link_libraries(QtLocationPlugin
	PUBLIC
		Qt5::Location
		Qt5::Sql

		qgc
)

target_include_directories(QtLocationPlugin
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		QMLControl
)
