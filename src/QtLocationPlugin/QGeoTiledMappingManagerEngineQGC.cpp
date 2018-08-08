/****************************************************************************
**
** Copyright (C) 2013 Aaron McCarthy <mccarthy.aaron@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtLocation module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
** 2015.4.4
** Adapted for use with QGroundControl
**
** Gus Grubba <mavlink@grubba.com>
**
****************************************************************************/

#include "QGCMapEngine.h"
#include "QGeoTiledMappingManagerEngineQGC.h"
#include "QGeoTileFetcherQGC.h"

#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#include <QtLocation/private/qgeotiledmapdata_p.h>
#else
#include <QtLocation/private/qgeotiledmap_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
#include <QtLocation/private/qgeofiletilecache_p.h>
#else
#include <QtLocation/private/qgeotilecache_p.h>
#endif
#endif

#include <QDir>
#include <QStandardPaths>

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
//-----------------------------------------------------------------------------
QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{

}
#endif

//-----------------------------------------------------------------------------
QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
:   QGeoTiledMappingManagerEngine()
{

    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(2.0);
    cameraCaps.setMaximumZoomLevel(MAX_MAP_ZOOM);
    cameraCaps.setSupportsBearing(true);
    setCameraCapabilities(cameraCaps);

    setTileSize(QSize(256, 256));

    // In Qt 5.10 QGeoMapType need QGeoCameraCapabilities as argument
    // E.g: https://github.com/qt/qtlocation/blob/2b230b0a10d898979e9d5193f4da2e408b397fe3/src/plugins/geoservices/osm/qgeotiledmappingmanagerengineosm.cpp#L167
    #if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    #define QGCGEOMAPTYPE(a,b,c,d,e,f)  QGeoMapType(a,b,c,d,e,f,QByteArray("QGroundControl"), cameraCaps)
    #elif QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    #define QGCGEOMAPTYPE(a,b,c,d,e,f)  QGeoMapType(a,b,c,d,e,f,QByteArray("QGroundControl"))
    #else
    #define QGCGEOMAPTYPE(a,b,c,d,e,f)  QGeoMapType(a,b,c,d,e,f)
    #endif

    /*
     * Google and Bing don't seem kosher at all. This was based on original code from OpenPilot and heavily modified to be used in QGC.
     */

    //-- IMPORTANT
    //   Changes here must reflect those in QGCMapEngine.cpp

    QList<QGeoMapType> mapTypes;

#ifndef QGC_NO_GOOGLE_MAPS
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Google Street Map",        "Google street map",            false,  false,  UrlFactory::GoogleMap);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "Google Satellite Map",     "Google satellite map",         false,  false,  UrlFactory::GoogleSatellite);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::TerrainMap,        "Google Terrain Map",       "Google terrain map",           false,  false,  UrlFactory::GoogleTerrain);
#endif

    /* TODO:
     *  Proper google hybrid maps requires collecting two separate bitmaps and overlaying them.
     *
     * mapTypes << QGCGEOMAPTYPE(QGeoMapType::HybridMap,       "Google Hybrid Map",        "Google hybrid map",            false, false, UrlFactory::GoogleHybrid);
     *
     */

    // Bing
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Bing Street Map",          "Bing street map",                  false,  false,  UrlFactory::BingMap);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "Bing Satellite Map",       "Bing satellite map",               false,  false,  UrlFactory::BingSatellite);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::HybridMap,         "Bing Hybrid Map",          "Bing hybrid map",                  false,  false,  UrlFactory::BingHybrid);

    // Statkart
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::TerrainMap,        "Statkart Terrain Map",     "Statkart Terrain Map",             false,  false,  UrlFactory::StatkartTopo);
    // Eniro
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::TerrainMap,        "Eniro Terrain Map",        "Eniro Terrain Map",                false,  false,  UrlFactory::EniroTopo);

    // Esri
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Esri Street Map",          "ArcGIS Online World Street Map",   true,   false,  UrlFactory::EsriWorldStreet);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "Esri Satellite Map",       "ArcGIS Online World Imagery",      true,   false,  UrlFactory::EsriWorldSatellite);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::TerrainMap,        "Esri Terrain Map",         "World Terrain Base",               false,  false,  UrlFactory::EsriTerrain);

    // VWorld
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "VWorld Satellite Map",      "VWorld Satellite Map",               false,  false,  UrlFactory::VWorldSatellite);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "VWorld Street Map",         "VWorld Street Map",                  false,  false,  UrlFactory::VWorldStreet);

    /* See: https://wiki.openstreetmap.org/wiki/Tile_usage_policy
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Open Street Map",          "Open Street map",              false, false, UrlFactory::OpenStreetMap);
    */

    // MapQuest
    /*
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "MapQuest Street Map",      "MapQuest street map",          false,  false,  UrlFactory::MapQuestMap);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "MapQuest Satellite Map",   "MapQuest satellite map",       false,  false,  UrlFactory::MapQuestSat);
    */

    /*
     * These are OK as you need your own token for accessing it. Out-of-the box, QGC does not even offer these unless you enter a proper Mapbox token.
     */

    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Mapbox Street Map",        "Mapbox Street Map",            false,  false,  UrlFactory::MapboxStreets);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::SatelliteMapDay,   "Mapbox Satellite Map",     "Mapbox Satellite Map",         false,  false,  UrlFactory::MapboxSatellite);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox High Contrast Map", "Mapbox High Contrast Map",     false,  false,  UrlFactory::MapboxHighContrast);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Light Map",         "Mapbox Light Map",             false,  false,  UrlFactory::MapboxLight);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Dark Map",          "Mapbox Dark Map",              false,  false,  UrlFactory::MapboxDark);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::HybridMap,         "Mapbox Hybrid Map",        "Mapbox Hybrid Map",            false,  false,  UrlFactory::MapboxHybrid);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Wheat Paste Map",   "Mapbox Wheat Paste Map",       false,  false,  UrlFactory::MapboxWheatPaste);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::StreetMap,         "Mapbox Streets Basic Map", "Mapbox Streets Basic Map",     false,  false,  UrlFactory::MapboxStreetsBasic);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Comic Map",         "Mapbox Comic Map",             false,  false,  UrlFactory::MapboxComic);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Outdoors Map",      "Mapbox Outdoors Map",          false,  false,  UrlFactory::MapboxOutdoors);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CycleMap,          "Mapbox Run, Byke and Hike Map",   "Mapbox Run, Byke and Hike Map",     false,  false,  UrlFactory::MapboxRunBikeHike);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Pencil Map",        "Mapbox Pencil Map",            false,  false,  UrlFactory::MapboxPencil);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Pirates Map",       "Mapbox Pirates Map",           false,  false,  UrlFactory::MapboxPirates);
    mapTypes << QGCGEOMAPTYPE(QGeoMapType::CustomMap,         "Mapbox Emerald Map",       "Mapbox Emerald Map",           false,  false,  UrlFactory::MapboxEmerald);

    setSupportedMapTypes(mapTypes);

    //-- Users (QML code) can define a different user agent
    if (parameters.contains(QStringLiteral("useragent"))) {
        getQGCMapEngine()->setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    _setCache(parameters);
#endif

    setTileFetcher(new QGeoTileFetcherQGC(this));

    *error = QGeoServiceProvider::NoError;
    errorString->clear();
}

//-----------------------------------------------------------------------------
QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
}

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)

//-----------------------------------------------------------------------------
QGeoMapData *QGeoTiledMappingManagerEngineQGC::createMapData()
{
    return new QGeoTiledMapData(this, 0);
}

#else

//-----------------------------------------------------------------------------
QGeoMap*
QGeoTiledMappingManagerEngineQGC::createMap()
{
    return new QGeoTiledMapQGC(this);
}

#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
//-----------------------------------------------------------------------------
void
QGeoTiledMappingManagerEngineQGC::_setCache(const QVariantMap &parameters)
{
    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory")))
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    else {
        cacheDir = getQGCMapEngine()->getCachePath();
        if(!QFileInfo(cacheDir).exists()) {
            if(!QDir::root().mkpath(cacheDir)) {
                qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
                cacheDir = QDir::homePath() + QStringLiteral("/.qgcmapscache/");
            }
        }
    }
    if(!QFileInfo(cacheDir).exists()) {
        if(!QDir::root().mkpath(cacheDir)) {
            qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
            cacheDir.clear();
        }
    }
    //-- Memory Cache
    uint32_t memLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.memory.size"))) {
      bool ok = false;
      memLimit = parameters.value(QStringLiteral("mapping.cache.memory.size")).toString().toUInt(&ok);
      if (!ok)
          memLimit = 0;
    }
    if(!memLimit)
    {
        //-- Value saved in MB
        memLimit = getQGCMapEngine()->getMaxMemCache() * (1024 * 1024);
    }
    //-- It won't work with less than 1M of memory cache
    if(memLimit < 1024 * 1024)
        memLimit = 1024 * 1024;
    //-- On the other hand, Qt uses signed 32-bit integers. Limit to 1G to round it down (you don't need more than that).
    if(memLimit > 1024 * 1024 * 1024)
        memLimit = 1024 * 1024 * 1024;
    //-- Disable Qt's disk cache (sort of)
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QAbstractGeoTileCache *pTileCache = new QGeoFileTileCache(cacheDir);
    setTileCache(pTileCache);
#else
    QGeoTileCache* pTileCache = createTileCacheWithDir(cacheDir);
#endif
    if(pTileCache)
    {
        //-- We're basically telling it to use 100k of disk for cache. It doesn't like
        //   values smaller than that and I could not find a way to make it NOT cache.
        //   We handle our own disk caching elsewhere.
        pTileCache->setMaxDiskUsage(1024 * 100);
        pTileCache->setMaxMemoryUsage(memLimit);
    }
}
#endif
