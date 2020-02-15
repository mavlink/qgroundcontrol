################################################################################
#
# (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
#
# QGroundControl is licensed according to the terms in the file
# COPYING.md in the root of the source code directory.
#
################################################################################

QT  += location-private positioning-private network

contains(QT_VERSION, 5.5.1) {
    message(Using Local QtLocation headers for Qt 5.5.1)
    INCLUDEPATH += \
        $$PWD/qtlocation/include \
} else {
    message(Using Default QtLocation headers)
    INCLUDEPATH += $$QT.location.includes
}

HEADERS += \
    $$PWD/QGCMapEngine.h \
    $$PWD/QGCMapEngineData.h \
    $$PWD/QGCMapTileSet.h \
    $$PWD/QGCMapUrlEngine.h \
    $$PWD/QGCTileCacheWorker.h \
    $$PWD/QGeoCodeReplyQGC.h \
    $$PWD/QGeoCodingManagerEngineQGC.h \
    $$PWD/QGeoMapReplyQGC.h \
    $$PWD/QGeoServiceProviderPluginQGC.h \
    $$PWD/QGeoTileFetcherQGC.h \
    $$PWD/QGeoTiledMappingManagerEngineQGC.h \
    $$PWD/MapProvider.h \
    $$PWD/ElevationMapProvider.h \
    $$PWD/GoogleMapProvider.h \
    $$PWD/BingMapProvider.h \
    $$PWD/GenericMapProvider.h \
    $$PWD/EsriMapProvider.h \
    $$PWD/MapboxMapProvider.h \
    $$PWD/QGCTileSet.h \


SOURCES += \
    $$PWD/QGCMapEngine.cpp \
    $$PWD/QGCMapTileSet.cpp \
    $$PWD/QGCMapUrlEngine.cpp \
    $$PWD/QGCTileCacheWorker.cpp \
    $$PWD/QGeoCodeReplyQGC.cpp \
    $$PWD/QGeoCodingManagerEngineQGC.cpp \
    $$PWD/QGeoMapReplyQGC.cpp \
    $$PWD/QGeoServiceProviderPluginQGC.cpp \
    $$PWD/QGeoTileFetcherQGC.cpp \
    $$PWD/QGeoTiledMappingManagerEngineQGC.cpp \
    $$PWD/MapProvider.cpp \
    $$PWD/ElevationMapProvider.cpp \
    $$PWD/GoogleMapProvider.cpp \
    $$PWD/BingMapProvider.cpp \
    $$PWD/GenericMapProvider.cpp \
    $$PWD/EsriMapProvider.cpp \
    $$PWD/MapboxMapProvider.cpp \

OTHER_FILES += \
    $$PWD/qgc_maps_plugin.json
