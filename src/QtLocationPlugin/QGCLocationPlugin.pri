################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

QT  += location-private positioning-private network

INCLUDEPATH += $$QT.location.includes MapProviders

HEADERS += \
    $$PWD/QGCMapEngine.h \
    $$PWD/QGCMapEngineData.h \
    $$PWD/QGCCachedTileSet.h \
    $$PWD/QGCMapUrlEngine.h \
    $$PWD/QGCTileCacheWorker.h \
    $$PWD/QGeoMapReplyQGC.h \
    $$PWD/QGeoServiceProviderPluginQGC.h \
    $$PWD/QGeoTileFetcherQGC.h \
    $$PWD/QGeoTiledMappingManagerEngineQGC.h \
    $$PWD/MapProviders/MapProvider.h \
    $$PWD/MapProviders/ElevationMapProvider.h \
    $$PWD/MapProviders/GoogleMapProvider.h \
    $$PWD/MapProviders/BingMapProvider.h \
    $$PWD/MapProviders/GenericMapProvider.h \
    $$PWD/MapProviders/EsriMapProvider.h \
    $$PWD/MapProviders/MapboxMapProvider.h \
    $$PWD/QGCTileSet.h \
    $$PWD/QGeoTiledMapQGC.h \
    $$PWD/QGeoFileTileCacheQGC.h


SOURCES += \
    $$PWD/QGCMapEngine.cpp \
    $$PWD/QGCCachedTileSet.cpp \
    $$PWD/QGCMapUrlEngine.cpp \
    $$PWD/QGCTileCacheWorker.cpp \
    $$PWD/QGeoMapReplyQGC.cpp \
    $$PWD/QGeoServiceProviderPluginQGC.cpp \
    $$PWD/QGeoTileFetcherQGC.cpp \
    $$PWD/QGeoTiledMappingManagerEngineQGC.cpp \
    $$PWD/MapProviders/MapProvider.cpp \
    $$PWD/MapProviders/ElevationMapProvider.cpp \
    $$PWD/MapProviders/GoogleMapProvider.cpp \
    $$PWD/MapProviders/BingMapProvider.cpp \
    $$PWD/MapProviders/GenericMapProvider.cpp \
    $$PWD/MapProviders/EsriMapProvider.cpp \
    $$PWD/MapProviders/MapboxMapProvider.cpp \
    $$PWD/QGeoTiledMapQGC.cpp \
    $$PWD/QGeoFileTileCacheQGC.cpp

OTHER_FILES += \
    $$PWD/qgc_maps_plugin.json
