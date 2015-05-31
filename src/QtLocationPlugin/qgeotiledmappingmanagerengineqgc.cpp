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

#include <QtLocation/private/qgeocameracapabilities_p.h>
#include <QtLocation/private/qgeomaptype_p.h>
#if QT_VERSION < 0x050500
#include <QtLocation/private/qgeotiledmapdata_p.h>
#else
#include <QtLocation/private/qgeotiledmap_p.h>
#endif
#include <QDir>
#include <QStandardPaths>

#include "qgeotiledmappingmanagerengineqgc.h"
#include "qgeotilefetcherqgc.h"
#include "OpenPilotMaps.h"

#if QT_VERSION >= 0x050500
QGeoTiledMapQGC::QGeoTiledMapQGC(QGeoTiledMappingManagerEngine *engine, QObject *parent)
    : QGeoTiledMap(engine, parent)
{

}
#endif

QGeoTiledMappingManagerEngineQGC::QGeoTiledMappingManagerEngineQGC(const QVariantMap &parameters, QGeoServiceProvider::Error *error, QString *errorString)
:   QGeoTiledMappingManagerEngine()
{
    QGeoCameraCapabilities cameraCaps;
    cameraCaps.setMinimumZoomLevel(2.0);
    cameraCaps.setMaximumZoomLevel(MAX_MAP_ZOOM);
    cameraCaps.setSupportsBearing(true);
    setCameraCapabilities(cameraCaps);

    setTileSize(QSize(256, 256));

    QList<QGeoMapType> mapTypes;
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         tr("Google Street Map"),   tr("Google street map"),    false, false, OpenPilot::GoogleMap);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   tr("Google Satellite Map"),tr("Google satellite map"), false, false, OpenPilot::GoogleSatellite);
    mapTypes << QGeoMapType(QGeoMapType::TerrainMap,        tr("Google Terrain Map"),  tr("Google terrain map"),   false, false, OpenPilot::GoogleTerrain);
    // TODO:
    // Proper hybrid maps requires collecting two separate bimaps and overlaying them.
    //mapTypes << QGeoMapType(QGeoMapType::HybridMap,       tr("Google Hybrid Map"),   tr("Google hybrid map"),    false, false, OpenPilot::GoogleHybrid);
    // Bing
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         tr("Bing Street Map"),     tr("Bing street map"),      false, false, OpenPilot::BingMap);
    mapTypes << QGeoMapType(QGeoMapType::SatelliteMapDay,   tr("Bing Satellite Map"),  tr("Bing satellite map"),   false, false, OpenPilot::BingSatellite);
    mapTypes << QGeoMapType(QGeoMapType::HybridMap,         tr("Bing Hybrid Map"),     tr("Bing hybrid map"),      false, false, OpenPilot::BingHybrid);
    mapTypes << QGeoMapType(QGeoMapType::StreetMap,         tr("Open Street Map"),     tr("Open Street map"),      false, false, OpenPilot::OpenStreetMap);
    setSupportedMapTypes(mapTypes);

    QGeoTileFetcherQGC *tileFetcher = new QGeoTileFetcherQGC(this);
    if (parameters.contains(QStringLiteral("useragent"))) {
        const QByteArray ua = parameters.value(QStringLiteral("useragent")).toString().toLatin1();
        tileFetcher->setUserAgent(ua);
    } else
        // QGC Default
        tileFetcher->setUserAgent("Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US; rv:1.9.1.7) Gecko/20091221 Firefox/3.5.7");

    setTileFetcher(tileFetcher);

    QString cacheDir;
    if (parameters.contains(QStringLiteral("mapping.cache.directory")))
        cacheDir = parameters.value(QStringLiteral("mapping.cache.directory")).toString();
    else {
        cacheDir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/QGCMapCache");
        if(!QDir::root().mkpath(cacheDir)) {
            qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
            cacheDir = QDir::homePath() + QLatin1String("/.qgcmapscache/");
        }
    }

    if(!QDir::root().mkpath(cacheDir))
    {
        qWarning() << "Could not create mapping disk cache directory: " << cacheDir;
        cacheDir.clear();
    }
    //else {
    //    qDebug() << "Mapping cache directory:" << cacheDir;
    //}

    QGeoTileCache *tileCache = createTileCacheWithDir(cacheDir);

    int cacheLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.disk.size"))) {
      bool ok = false;
      cacheLimit = parameters.value(QStringLiteral("mapping.cache.disk.size")).toString().toInt(&ok);
      if (!ok)
          cacheLimit = 0;
    }
    if(!cacheLimit)
        // QGC Default
        cacheLimit = 1024 * 1024 * 1024;
    tileCache->setMaxDiskUsage(cacheLimit);
    //qDebug() << "Disk caching limit:" << cacheLimit;

    cacheLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.memory.size"))) {
      bool ok = false;
      cacheLimit = parameters.value(QStringLiteral("mapping.cache.memory.size")).toString().toInt(&ok);
      if (!ok)
          cacheLimit = 0;
    }
    if(!cacheLimit)
        // QGC Default
        cacheLimit = 10 * 1024 * 1024;
    tileCache->setMaxMemoryUsage(cacheLimit);
    //qDebug() << "Memory caching limit:" << cacheLimit;

    cacheLimit = 0;
    if (parameters.contains(QStringLiteral("mapping.cache.texture.size"))) {
      bool ok = false;
      cacheLimit = parameters.value(QStringLiteral("mapping.cache.texture.size")).toString().toInt(&ok);
      if (!ok)
          cacheLimit = 0;
    }
    if(!cacheLimit)
        // QGC Default
        cacheLimit = 10 * 1024 * 1024;
    tileCache->setExtraTextureUsage(cacheLimit);

    *error = QGeoServiceProvider::NoError;
    errorString->clear();

#if QT_VERSION >= 0x050500
    if (parameters.contains(QStringLiteral("mapping.copyright")))
        m_customCopyright = parameters.value(QStringLiteral("mapping.copyright")).toString().toLatin1();
#endif
}

QGeoTiledMappingManagerEngineQGC::~QGeoTiledMappingManagerEngineQGC()
{
}

#if QT_VERSION < 0x050500

QGeoMapData *QGeoTiledMappingManagerEngineQGC::createMapData()
{
    return new QGeoTiledMapData(this, 0);
}

#else

QGeoMap *QGeoTiledMappingManagerEngineQGC::createMap()
{
    return new QGeoTiledMapQGC(this);
}

QString QGeoTiledMappingManagerEngineQGC::customCopyright() const
{
    return m_customCopyright;
}

#endif
