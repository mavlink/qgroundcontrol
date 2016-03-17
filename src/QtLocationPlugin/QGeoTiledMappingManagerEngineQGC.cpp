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
#if QT_VERSION < 0x050500
#include <QtLocation/private/qgeotiledmapdata_p.h>
#else
#include <QtLocation/private/qgeotiledmap_p.h>
#if QT_VERSION >= 0x050600
#include <QtLocation/private/qgeofiletilecache_p.h>
#else
#include <QtLocation/private/qgeotilecache_p.h>
#endif
#endif

#include <QDir>
#include <QStandardPaths>

#if QT_VERSION >= 0x050500
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

    /*
     * Most of these don't seem kosher at all. This was based on original code from OpenPilot and heavily modified to be used in QGC.
     */


    //-- IMPORTANT
    //   Changes here must reflect those in QGCMapEngine.cpp

    QList<QGeoMapType> mapTypes;
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "Google Street Map",        "Google street map",            false,  false,  UrlFactory::GoogleMap);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   "Google Satellite Map",     "Google satellite map",         false,  false,  UrlFactory::GoogleSatellite);
    mapTypes << QGeoMapType(QGeoMapType::TerrainMap,        "Google Terrain Map",       "Google terrain map",           false,  false,  UrlFactory::GoogleTerrain);

    /* TODO:
     *  Proper google hybrid maps requires collecting two separate bimaps and overlaying them.
     *
     * mapTypes << QGeoMapType(QGeoMapType::HybridMap,       "Google Hybrid Map",        "Google hybrid map",            false, false, UrlFactory::GoogleHybrid);
     *
     */

    // Bing
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "Bing Street Map",          "Bing street map",              false,  false,  UrlFactory::BingMap);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   "Bing Satellite Map",       "Bing satellite map",           false,  false,  UrlFactory::BingSatellite);
    mapTypes << QGeoMapType(QGeoMapType::HybridMap,         "Bing Hybrid Map",          "Bing hybrid map",              false,  false,  UrlFactory::BingHybrid);

    /* See: https://wiki.openstreetmap.org/wiki/Tile_usage_policy
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "Open Street Map",          "Open Street map",              false, false, UrlFactory::OpenStreetMap);
    */

    // MapQuest
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "MapQuest Street Map",      "MapQuest street map",          false,  false,  UrlFactory::MapQuestMap);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   "MapQuest Satellite Map",   "MapQuest satellite map",       false,  false,  UrlFactory::MapQuestSat);

    /*
     * These are OK as you need your own token for accessing it. Out-of-the box, QGC does not even offer these unless you enter a proper MapBox token.
     */

    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "MapBox Street Map",        "MapBox Street Map",            false,  false,  UrlFactory::MapBoxStreets);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   "MapBox Satellite Map",     "MapBox Satellite Map",         false,  false,  UrlFactory::MapBoxSatellite);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox High Contrast Map", "MapBox High Contrast Map",     false,  false,  UrlFactory::MapBoxHighContrast);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Light Map",         "MapBox Light Map",             false,  false,  UrlFactory::MapBoxLight);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Dark Map",          "MapBox Dark Map",              false,  false,  UrlFactory::MapBoxDark);
    mapTypes << QGeoMapType(QGeoMapType::HybridMap,         "MapBox Hybrid Map",        "MapBox Hybrid Map",            false,  false,  UrlFactory::MapBoxHybrid);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Wheat Paste Map",   "MapBox Wheat Paste Map",       false,  false,  UrlFactory::MapBoxWheatPaste);
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         "MapBox Streets Basic Map", "MapBox Streets Basic Map",     false,  false,  UrlFactory::MapBoxStreetsBasic);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Comic Map",         "MapBox Comic Map",             false,  false,  UrlFactory::MapBoxComic);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Outdoors Map",      "MapBox Outdoors Map",          false,  false,  UrlFactory::MapBoxOutdoors);
    mapTypes << QGeoMapType(QGeoMapType::CycleMap,          "MapBox Run, Byke and Hike Map",   "MapBox Run, Byke and Hike Map",     false,  false,  UrlFactory::MapBoxRunBikeHike);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Pencil Map",        "MapBox Pencil Map",            false,  false,  UrlFactory::MapBoxPencil);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Pirates Map",       "MapBox Pirates Map",           false,  false,  UrlFactory::MapBoxPirates);
    mapTypes << QGeoMapType(QGeoMapType::CustomMap,         "MapBox Emerald Map",       "MapBox Emerald Map",           false,  false,  UrlFactory::MapBoxEmerald);

    setSupportedMapTypes(mapTypes);

    //-- Users (QML code) can define a different user agent
    if (parameters.contains(QStringLiteral("useragent"))) {
        getQGCMapEngine()->setUserAgent(parameters.value(QStringLiteral("useragent")).toString().toLatin1());
    }

#if QT_VERSION >= 0x050500
    _setCache(parameters);
#endif

    setTileFetcher(new QGeoTileFetcherQGC(this));

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

#if QT_VERSION >= 0x050500
    if (parameters.contains(QStringLiteral("mapping.copyright")))
        m_customCopyright = parameters.value(QStringLiteral("mapping.copyright")).toString().toLatin1();
#endif
}

//-----------------------------------------------------------------------------
QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
}

#if QT_VERSION < 0x050500

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

//-----------------------------------------------------------------------------
QString
QGeoTiledMappingManagerEngineQGC::customCopyright() const
{
    return m_customCopyright;
}

#endif

#if QT_VERSION >= 0x050500
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
                cacheDir = QDir::homePath() + QLatin1String("/.qgcmapscache/");
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
    //-- Disable Qt's disk cache (set memory cache otherwise Qtlocation won't work)
#if QT_VERSION >= 0x050600
    QAbstractGeoTileCache *pTileCache = new QGeoFileTileCache(cacheDir);
    setTileCache(pTileCache);
#else
    QGeoTileCache* pTileCache = createTileCacheWithDir(cacheDir);
#endif
    if(pTileCache)
    {
        //-- We're basically telling it to use 100k of disk for cache. It doesn't like
        //   values smaller than that and I could not find a way to make it NOT cache.
        pTileCache->setMaxDiskUsage(1024 * 100);
        pTileCache->setMaxMemoryUsage(memLimit);
    }
}
#endif
