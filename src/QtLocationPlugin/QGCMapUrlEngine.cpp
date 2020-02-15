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

#include "QGCLoggingCategory.h"
QGC_LOGGING_CATEGORY(QGCMapUrlEngineLog, "QGCMapUrlEngineLog")

#include "AppSettings.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"


#include <QByteArray>
#include <QEventLoop>
#include <QNetworkReply>
#include <QRegExp>
#include <QString>
#include <QTimer>

//-----------------------------------------------------------------------------
UrlFactory::UrlFactory() : _timeout(5 * 1000) {

    // Warning : in _providersTable, keys needs to follow this format :
    // "Provider Type"
#ifndef QGC_NO_GOOGLE_MAPS
    _providersTable["Google Street Map"] = new GoogleStreetMapProvider(this);
    _providersTable["Google Satellite"]  = new GoogleSatelliteMapProvider(this);
    _providersTable["Google Terrain"]    = new GoogleTerrainMapProvider(this);
    _providersTable["Google Hybrid"]    = new GoogleHybridMapProvider(this);
    _providersTable["Google Labels"]     = new GoogleTerrainMapProvider(this);
#endif

    _providersTable["Bing Road"]      = new BingRoadMapProvider(this);
    _providersTable["Bing Satellite"] = new BingSatelliteMapProvider(this);
    _providersTable["Bing Hybrid"]    = new BingHybridMapProvider(this);

    _providersTable["Statkart Topo"] = new StatkartMapProvider(this);

    _providersTable["Eniro Topo"] = new EniroMapProvider(this);

    // To be add later on Token entry !
    //_providersTable["Esri World Street"] = new EsriWorldStreetMapProvider(this);
    //_providersTable["Esri World Satellite"] = new EsriWorldSatelliteMapProvider(this);
    //_providersTable["Esri Terrain"] = new EsriTerrainMapProvider(this);

    _providersTable["Mapbox Streets"]      = new MapboxStreetMapProvider(this);
    _providersTable["Mapbox Light"]        = new MapboxLightMapProvider(this);
    _providersTable["Mapbox Dark"]         = new MapboxDarkMapProvider(this);
    _providersTable["Mapbox Satellite"]    = new MapboxSatelliteMapProvider(this);
    _providersTable["Mapbox Hybrid"]       = new MapboxHybridMapProvider(this);
    _providersTable["Mapbox StreetsBasic"] = new MapboxStreetsBasicMapProvider(this);
    _providersTable["Mapbox Outdoors"]     = new MapboxOutdoorsMapProvider(this);
    _providersTable["Mapbox RunBikeHike"]  = new MapboxRunBikeHikeMapProvider(this);
    _providersTable["Mapbox HighContrast"] = new MapboxHighContrastMapProvider(this);

    //_providersTable["MapQuest Map"] = new MapQuestMapMapProvider(this);
    //_providersTable["MapQuest Sat"] = new MapQuestSatMapProvider(this);
    
    _providersTable["VWorld Street Map"] = new VWorldStreetMapProvider(this);
    _providersTable["VWorld Satellite Map"] = new VWorldSatMapProvider(this);

    _providersTable["Airmap Elevation"] = new AirmapElevationProvider(this);
}

void UrlFactory::registerProvider(QString name, MapProvider* provider) {
    _providersTable[name] = provider;
}

//-----------------------------------------------------------------------------
UrlFactory::~UrlFactory() {}

QString UrlFactory::getImageFormat(int id, const QByteArray& image) {
    QString type = getTypeFromId(id);
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[getTypeFromId(id)]->getImageFormat(image);
    } else {
        qCDebug(QGCMapUrlEngineLog) << "getImageFormat : Map not registered :" << type;
        return "";
    }
}

//-----------------------------------------------------------------------------
QString UrlFactory::getImageFormat(QString type, const QByteArray& image) {
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getImageFormat(image);
    } else {
        qCDebug(QGCMapUrlEngineLog) << "getImageFormat : Map not registered :" << type;
        return "";
    }
}
QNetworkRequest UrlFactory::getTileURL(int id, int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {

    QString type = getTypeFromId(id);
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getTileURL(x, y, zoom, networkManager);
    }

    qCDebug(QGCMapUrlEngineLog) << "getTileURL : map not registered :" << type;
    return QNetworkRequest(QUrl());
}

//-----------------------------------------------------------------------------
QNetworkRequest UrlFactory::getTileURL(QString type, int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getTileURL(x, y, zoom, networkManager);
    }
    qCDebug(QGCMapUrlEngineLog) << "getTileURL : map not registered :" << type;
    return QNetworkRequest(QUrl());
}

//-----------------------------------------------------------------------------
quint32 UrlFactory::averageSizeForType(QString type) {
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getAverageSize();
    } 
    qCDebug(QGCMapUrlEngineLog) << "UrlFactory::averageSizeForType " << type
        << " Not registered";

    //    case AirmapElevation:
    //        return AVERAGE_AIRMAP_ELEV_SIZE;
    //    default:
    //        break;
    //    }
    return AVERAGE_TILE_SIZE;
}

QString UrlFactory::getTypeFromId(int id) {

    QHashIterator<QString, MapProvider*> i(_providersTable);

    while (i.hasNext()) {
        i.next();
        if ((int)(qHash(i.key())>>1) == id) {
            return i.key();
        }
    }
    qCDebug(QGCMapUrlEngineLog) << "getTypeFromId : id not found" << id;
    return "";
}

// Todo : qHash produce a uint bigger than max(int)
// There is still a low probability for this to
// generate similar hash for different types
int UrlFactory::getIdFromType(QString type) { return (int)(qHash(type)>>1); }

//-----------------------------------------------------------------------------
int
UrlFactory::long2tileX(QString mapType, double lon, int z)
{
    return _providersTable[mapType]->long2tileX(lon, z);
}

//-----------------------------------------------------------------------------
int
UrlFactory::lat2tileY(QString mapType, double lat, int z)
{
    return _providersTable[mapType]->lat2tileY(lat, z);
}


//-----------------------------------------------------------------------------
QGCTileSet
UrlFactory::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon, double bottomRightLat, QString mapType)
{
	return _providersTable[mapType]->getTileCount(zoom, topleftLon, topleftLat, bottomRightLon, bottomRightLat);
}

bool UrlFactory::isElevation(int mapId){
    return _providersTable[getTypeFromId(mapId)]->_isElevationProvider();
}
