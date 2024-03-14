/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 *  @file
 *  @author Gus Grubba <gus@auterion.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C)
 * 2012.
 */

//#define DEBUG_GOOGLE_MAPS

#include "QGCMapUrlEngine.h"
#include "QGCLoggingCategory.h"
QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "QGCMapUrlEngineLog")

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkReply>
#include <QtCore5Compat/QRegExp>
#include <QString>
#include <QTimer>

const char* UrlFactory::kCopernicusElevationProviderKey = "Copernicus Elevation";
const char* UrlFactory::kCopernicusElevationProviderNotice = "© Airbus Defence and Space GmbH";

//-----------------------------------------------------------------------------
UrlFactory::UrlFactory() : 
    _timeout(5 * 1000) 
{

    // The internal Qt code for map plugins has the concept of a Map Id. These ids must start at 1 and be sequential.
    // Map Ids are used to identify the map provider to use. Because of this we keep the providers in a simple list
    // such that the index into the list with the map id - 1.

#ifndef QGC_NO_GOOGLE_MAPS
    _providers.append(ProviderPair("Google Street Map", new GoogleStreetMapProvider(this)));
    _providers.append(ProviderPair("Google Satellite", new GoogleSatelliteMapProvider(this)));
    _providers.append(ProviderPair("Google Terrain", new GoogleTerrainMapProvider(this)));
    _providers.append(ProviderPair("Google Hybrid", new GoogleHybridMapProvider(this)));
    _providers.append(ProviderPair("Google Labels", new GoogleLabelsMapProvider(this)));
#endif
    _providers.append(ProviderPair("AMAP(CN)",new GaodeSatMapProvider(this)));
    _providers.append(ProviderPair("Bing Road", new BingRoadMapProvider(this)));
    _providers.append(ProviderPair("Bing Satellite", new BingSatelliteMapProvider(this)));
    _providers.append(ProviderPair("Bing Hybrid", new BingHybridMapProvider(this)));

    _providers.append(ProviderPair("Statkart Topo", new StatkartMapProvider(this)));
    _providers.append(ProviderPair("Statkart Basemap", new StatkartBaseMapProvider(this)));

    _providers.append(ProviderPair("Eniro Topo", new EniroMapProvider(this)));

    // To be add later on Token entry !
    //_providers.append(ProviderPair("Esri World Street", new EsriWorldStreetMapProvider(this)));
    //_providers.append(ProviderPair("Esri World Satellite", new EsriWorldSatelliteMapProvider(this)));
    //_providers.append(ProviderPair("Esri Terrain", new EsriTerrainMapProvider(this)));

    _providers.append(ProviderPair("Mapbox Streets", new MapboxStreetMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Light", new MapboxLightMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Dark", new MapboxDarkMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Satellite", new MapboxSatelliteMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Hybrid", new MapboxHybridMapProvider(this)));
    _providers.append(ProviderPair("Mapbox StreetsBasic", new MapboxStreetsBasicMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Outdoors", new MapboxOutdoorsMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Bright", new MapboxBrightMapProvider(this)));
    _providers.append(ProviderPair("Mapbox Custom", new MapboxCustomMapProvider(this)));

    //_providers.append(ProviderPair("MapQuest Map", new MapQuestMapMapProvider(this)));
    //_providers.append(ProviderPair("MapQuest Sat", new MapQuestSatMapProvider(this)));

    _providers.append(ProviderPair("VWorld Street Map", new VWorldStreetMapProvider(this)));
    _providers.append(ProviderPair("VWorld Satellite Map", new VWorldSatMapProvider(this)));

    _providers.append(ProviderPair(kCopernicusElevationProviderKey, new CopernicusElevationProvider(this)));

    _providers.append(ProviderPair("Japan-GSI Contour", new JapanStdMapProvider(this)));
    _providers.append(ProviderPair("Japan-GSI Seamless", new JapanSeamlessMapProvider(this)));
    _providers.append(ProviderPair("Japan-GSI Anaglyph", new JapanAnaglyphMapProvider(this)));
    _providers.append(ProviderPair("Japan-GSI Slope", new JapanSlopeMapProvider(this)));
    _providers.append(ProviderPair("Japan-GSI Relief", new JapanReliefMapProvider(this)));

    _providers.append(ProviderPair("LINZ Basemap", new LINZBasemapMapProvider(this)));

    _providers.append(ProviderPair("CustomURL Custom", new CustomURLMapProvider(this)));
}

//-----------------------------------------------------------------------------
UrlFactory::~UrlFactory() {}

QString UrlFactory::getImageFormat(int qtMapId, const QByteArray& image) {
    MapProvider* provider = getMapProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getImageFormat(image);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getImageFormat : map id not found:" << qtMapId;
        return "";
    }
}

//-----------------------------------------------------------------------------
QString UrlFactory::getImageFormat(const QString& type, const QByteArray& image) {
    MapProvider* provider =  getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getImageFormat(image);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getImageFormat : type not found:" << type;
        return "";
    }
}
QNetworkRequest UrlFactory::getTileURL(int qtMapId, int x, int y, int zoom, QNetworkAccessManager* networkManager) 
{
    MapProvider* provider = getMapProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->getTileURL(x, y, zoom, networkManager);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getTileURL : map id not found:" << qtMapId;
        return QNetworkRequest(QUrl());
    }
}

//-----------------------------------------------------------------------------
QNetworkRequest UrlFactory::getTileURL(const QString& type, int x, int y, int zoom, QNetworkAccessManager* networkManager) 
{
    MapProvider* provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getTileURL(x, y, zoom, networkManager);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getTileURL : type not found:" << type;
        return QNetworkRequest(QUrl());
    }
}

//-----------------------------------------------------------------------------
quint32 UrlFactory::averageSizeForType(const QString& type) {
    MapProvider* provider = getMapProviderFromProviderType(type);
    if (provider) {
        return provider->getAverageSize();
    } else {
        qCWarning(QGCMapUrlEngineLog) << "UrlFactory::averageSizeForType type not found:" << type;
        return AVERAGE_TILE_SIZE;
    }
}

QString UrlFactory::getProviderTypeFromQtMapId(int qtMapId) {
    if (qtMapId >= 1 && qtMapId <= _providers.count()) {
        return _providers.at(qtMapId - 1).first;
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getProviderTypeFromQtMapId : map id not found:" << qtMapId;
        return _providers.at(0).first;
    }
}

MapProvider* UrlFactory::getMapProviderFromProviderType(const QString& type)
{
    for (qsizetype i=0; i<_providers.count(); i++) {
        if (_providers.at(i).first == type) {
            return _providers.at(i).second;
        }
    }

    return nullptr;
}

MapProvider* UrlFactory::getMapProviderFromQtMapId(int qtMapId)
{
    if (qtMapId >= 1 && qtMapId <= _providers.count()) {
        return _providers.at(qtMapId - 1).second;
    } else {
        return nullptr;
    }
}

int UrlFactory::getQtMapIdFromProviderType(const QString& type)
{
    for (qsizetype i=0; i<_providers.count(); i++) {
        if (_providers.at(i).first == type) {
            return i + 1;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "getQtMapIdFromProviderType : type not found:" << type;
    return 1;
}

int UrlFactory::long2tileX(const QString& mapType, double lon, int z)
{
    MapProvider* provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        return provider->long2tileX(lon, z);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "long2tileX : type not found:" << mapType;
        return 0;
    }
}

int UrlFactory::lat2tileY(const QString& mapType, double lat, int z)
{
    MapProvider* provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        return provider->lat2tileY(lat, z);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "lat2tileY : type not found:" << mapType;
        return 0;
    }
}

QGCTileSet UrlFactory::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, const QString& mapType)
{
    MapProvider* provider = getMapProviderFromProviderType(mapType);
    if (provider) {
        return provider->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
    } else {
        qCWarning(QGCMapUrlEngineLog) << "getTileCount : type not found:" << mapType;
        return QGCTileSet();
    }
}

bool UrlFactory::isElevation(int qtMapId)
{
    MapProvider* provider = getMapProviderFromQtMapId(qtMapId);
    if (provider) {
        return provider->_isElevationProvider();
    } else {
        qCWarning(QGCMapUrlEngineLog) << "isElevation : map id not found:" << qtMapId;
        return false;
    }
}

QStringList UrlFactory::getProviderTypes()
{
    QStringList types;
    for (qsizetype i=0; i<_providers.count(); i++) {
        types.append(_providers.at(i).first);
    }

    return types;
}

int UrlFactory::hashFromProviderType(const QString& type)
{
    return (int)(qHash(type) >> 1);
}

QString UrlFactory::providerTypeFromHash(int hash)
{
    for (qsizetype i=0; i<_providers.count(); i++) {
        if (int(qHash(_providers.at(i).first) >> 1) == hash) {
            return _providers.at(i).first;
        }
    }

    qCWarning(QGCMapUrlEngineLog) << "providerTypeFromTileHash : provider not found from hash" << hash;
    return "";
}
