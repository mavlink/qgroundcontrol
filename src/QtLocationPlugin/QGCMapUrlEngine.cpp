/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 *  @file
 *  @author Gus Grubba <mavlink@grubba.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C)
 * 2012.
 */

//#define DEBUG_GOOGLE_MAPS

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

    // BingMaps
    //_versionBingMaps             = "563";
#ifndef QGC_NO_GOOGLE_MAPS
    _providersTable["Google Street Map"] = new GoogleStreetMapProvider(this);
    _providersTable["Google Satellite"]   = new GoogleSatelliteMapProvider(this);
    _providersTable["Google Terrain"]   = new GoogleTerrainMapProvider(this);
#endif
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
        return "";
    }
}

//-----------------------------------------------------------------------------
QString UrlFactory::getImageFormat(QString type, const QByteArray& image) {
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getImageFormat(image);
    } else {
        return "";
    }
    // QString format;
    // if(image.size() > 2)
    //{
    //    if (image.startsWith(reinterpret_cast<const char*>(pngSignature)))
    //        format = "png";
    //    else if (image.startsWith(reinterpret_cast<const
    //    char*>(jpegSignature)))
    //        format = "jpg";
    //    else if (image.startsWith(reinterpret_cast<const
    //    char*>(gifSignature)))
    //        format = "gif";
    //    else {
    //        switch (type) {
    //            case GoogleMap:
    //            case GoogleLabels:
    //            case GoogleTerrain:
    //            case GoogleHybrid:
    //            case BingMap:
    //            case StatkartTopo:
    //                format = "png";
    //                break;
    //            case EniroTopo:
    //                format = "png";
    //                break;
    //            /*
    //            case MapQuestMap:
    //            case MapQuestSat:
    //            case OpenStreetMap:
    //            */
    //            case MapboxStreets:
    //            case MapboxLight:
    //            case MapboxDark:
    //            case MapboxSatellite:
    //            case MapboxHybrid:
    //            case MapboxWheatPaste:
    //            case MapboxStreetsBasic:
    //            case MapboxComic:
    //            case MapboxOutdoors:
    //            case MapboxRunBikeHike:
    //            case MapboxPencil:
    //            case MapboxPirates:
    //            case MapboxEmerald:
    //            case MapboxHighContrast:
    //            case GoogleSatellite:
    //            case BingSatellite:
    //            case BingHybrid:
    //                format = "jpg";
    //                break;
    //            case AirmapElevation:
    //                format = "bin";
    //                break;
    //            case VWorldStreet :
    //                format = "png";
    //                break;
    //            case VWorldSatellite :
    //                format = "jpg";
    //                break;
    //            default:
    //                qWarning("UrlFactory::getImageFormat() Unknown map id %d",
    //                type); break;
    //        }
    //    }
    //}
    // return format;
}
QNetworkRequest UrlFactory::getTileURL(int id, int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {

    QString type = getTypeFromId(id);
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getTileURL(x, y, zoom, networkManager);
    }
    return QNetworkRequest(QUrl());
}

//-----------------------------------------------------------------------------
QNetworkRequest UrlFactory::getTileURL(QString type, int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {
    return _providersTable[type]->getTileURL(x, y, zoom, networkManager);
    ////-- Build URL
    // QNetworkRequest request;
    // QString url = _getURL(type, x, y, zoom, networkManager);
    // if(url.isEmpty()) {
    //    return request;
    //}
    // request.setUrl(QUrl(url));
    // request.setRawHeader("Accept", "*/*");
    // switch (type) {
    //    //    case GoogleMap:
    //    //    case GoogleSatellite:
    //    //    case GoogleLabels:
    //    //    case GoogleTerrain:
    //    //    case GoogleHybrid:
    //    //        request.setRawHeader("Referrer",
    //    "https://www.google.com/maps/preview");
    //    //        break;
    //    case BingHybrid:
    //    case BingMap:
    //    case BingSatellite:
    //        request.setRawHeader("Referrer", "https://www.bing.com/maps/");
    //        break;
    //    case StatkartTopo:
    //        request.setRawHeader("Referrer", "https://www.norgeskart.no/");
    //        break;
    //    case EniroTopo:
    //        request.setRawHeader("Referrer", "https://www.eniro.se/");
    //        break;
    //    /*
    //    case OpenStreetMapSurfer:
    //    case OpenStreetMapSurferTerrain:
    //        request.setRawHeader("Referrer", "http://www.mapsurfer.net/");
    //        break;
    //    case OpenStreetMap:
    //    case OpenStreetOsm:
    //        request.setRawHeader("Referrer",
    //        "https://www.openstreetmap.org/"); break;
    //    */

    //    case EsriWorldStreet:
    //    case EsriWorldSatellite:
    //    case EsriTerrain: {
    //            QByteArray token =
    //            qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().toLatin1();
    //            request.setRawHeader("User-Agent", QByteArrayLiteral("Qt
    //            Location based application"));
    //            request.setRawHeader("User-Token", token);
    //        }
    //        return request;

    //    case AirmapElevation:
    //        request.setRawHeader("Referrer", "https://api.airmap.com/");
    //        break;

    //    default:
    //        break;
    //}
    // request.setRawHeader("User-Agent", _userAgent);
    // return request;
}

//-----------------------------------------------------------------------------
#if 0
QString
UrlFactory::_getURL(QString type, int x, int y, int zoom, QNetworkAccessManager* networkManager)
{
    switch (type) {
    Q_UNUSED(networkManager);
    case GoogleMap:
    {
        // http://mt1.google.com/vt/lyrs=m
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(x, y, sec1, sec2);
        _tryCorrectGoogleVersions(networkManager);
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(x, y, 4)).arg(request).arg(_versionGoogleMap).arg(_language).arg(x).arg(sec1).arg(y).arg(zoom).arg(sec2);
    }
    break;
    case GoogleSatellite:
    {
        // http://mt1.google.com/vt/lyrs=s
        QString server  = "khm";
        QString request = "kh";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(x, y, sec1, sec2);
        _tryCorrectGoogleVersions(networkManager);
        return QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(x, y, 4)).arg(request).arg(_versionGoogleSatellite).arg(_language).arg(x).arg(sec1).arg(y).arg(zoom).arg(sec2);
    }
    break;
    case GoogleLabels:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(x, y, sec1, sec2);
        _tryCorrectGoogleVersions(networkManager);
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(x, y, 4)).arg(request).arg(_versionGoogleLabels).arg(_language).arg(x).arg(sec1).arg(y).arg(zoom).arg(sec2);
    }
    break;
    case GoogleTerrain:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(x, y, sec1, sec2);
        _tryCorrectGoogleVersions(networkManager);
        return QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(x, y, 4)).arg(request).arg(_versionGoogleTerrain).arg(_language).arg(x).arg(sec1).arg(y).arg(zoom).arg(sec2);
    }
    break;
    case StatkartTopo:
    {
        return QString("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom=%1&x=%2&y=%3").arg(zoom).arg(x).arg(y);
    }
    break;
    case EniroTopo:
    {
    	return QString("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.png").arg(zoom).arg(x).arg((1<<zoom)-1-y);
    }
    break;
    /*
    case OpenStreetMap:
    {
        char letter = "abc"[_getServerNum(x, y, 3)];
        return QString("https://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(letter).arg(zoom).arg(x).arg(y);
    }
    break;
    case OpenStreetOsm:
    {
        char letter = "abc"[_getServerNum(x, y, 3)];
        return QString("http://%1.tah.openstreetmap.org/Tiles/tile/%2/%3/%4.png").arg(letter).arg(zoom).arg(x).arg(y);
    }
    break;
    case OpenStreetMapSurfer:
    {
        // http://tiles1.mapsurfer.net/tms_r.ashx?x=37378&y=20826&z=16
        return QString("http://tiles1.mapsurfer.net/tms_r.ashx?x=%1&y=%2&z=%3").arg(x).arg(y).arg(zoom);
    }
    break;
    case OpenStreetMapSurferTerrain:
    {
        // http://tiles2.mapsurfer.net/tms_t.ashx?x=9346&y=5209&z=14
        return QString("http://tiles2.mapsurfer.net/tms_t.ashx?x=%1&y=%2&z=%3").arg(x).arg(y).arg(zoom);
    }
    break;
    */
    case BingMap:
    {
        QString key = _tileXYToQuadKey(x, y, zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4").arg(_getServerNum(x, y, 4)).arg(key).arg(_versionBingMaps).arg(_language);
    }
    break;
    case BingSatellite:
    {
        QString key = _tileXYToQuadKey(x, y, zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4").arg(_getServerNum(x, y, 4)).arg(key).arg(_versionBingMaps).arg(_language);
    }
    break;
    case BingHybrid:
    {
        QString key = _tileXYToQuadKey(x, y, zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4").arg(_getServerNum(x, y, 4)).arg(key).arg(_versionBingMaps).arg(_language);
    }
    /*
    case MapQuestMap:
    {
        char letter = "1234"[_getServerNum(x, y, 4)];
        return QString("http://otile%1.mqcdn.com/tiles/1.0.0/map/%2/%3/%4.jpg").arg(letter).arg(zoom).arg(x).arg(y);
    }
    break;
    case MapQuestSat:
    {
        char letter = "1234"[_getServerNum(x, y, 4)];
        return QString("http://otile%1.mqcdn.com/tiles/1.0.0/sat/%2/%3/%4.jpg").arg(letter).arg(zoom).arg(x).arg(y);
    }
    break;
    */
    case EsriWorldStreet:
        return QString("http://services.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/%1/%2/%3").arg(zoom).arg(y).arg(x);
    case EsriWorldSatellite:
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3").arg(zoom).arg(y).arg(x);
    case EsriTerrain:
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/%1/%2/%3").arg(zoom).arg(y).arg(x);

    case MapboxStreets:
    case MapboxLight:
    case MapboxDark:
    case MapboxSatellite:
    case MapboxHybrid:
    case MapboxWheatPaste:
    case MapboxStreetsBasic:
    case MapboxComic:
    case MapboxOutdoors:
    case MapboxRunBikeHike:
    case MapboxPencil:
    case MapboxPirates:
    case MapboxEmerald:
    case MapboxHighContrast:
    {
        QString mapBoxToken = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString();
        if(!mapBoxToken.isEmpty()) {
            QString server = "https://api.mapbox.com/v4/";
            switch(type) {
                case MapboxStreets:
                    server += "mapbox.streets";
                    break;
                case MapboxLight:
                    server += "mapbox.light";
                    break;
                case MapboxDark:
                    server += "mapbox.dark";
                    break;
                case MapboxSatellite:
                    server += "mapbox.satellite";
                    break;
                case MapboxHybrid:
                    server += "mapbox.streets-satellite";
                    break;
                case MapboxWheatPaste:
                    server += "mapbox.wheatpaste";
                    break;
                case MapboxStreetsBasic:
                    server += "mapbox.streets-basic";
                    break;
                case MapboxComic:
                    server += "mapbox.comic";
                    break;
                case MapboxOutdoors:
                    server += "mapbox.outdoors";
                    break;
                case MapboxRunBikeHike:
                    server += "mapbox.run-bike-hike";
                    break;
                case MapboxPencil:
                    server += "mapbox.pencil";
                    break;
                case MapboxPirates:
                    server += "mapbox.pirates";
                    break;
                case MapboxEmerald:
                    server += "mapbox.emerald";
                    break;
                case MapboxHighContrast:
                    server += "mapbox.high-contrast";
                    break;
                default:
                    return {};
            }
            server += QString("/%1/%2/%3.jpg80?access_token=%4").arg(zoom).arg(x).arg(y).arg(mapBoxToken);
            return server;
        }
    }
    break;
    case AirmapElevation:
    {
        return QString("https://api.airmap.com/elevation/v1/ele/carpet?points=%1,%2,%3,%4").arg(static_cast<double>(y)*QGCMapEngine::srtm1TileSize - 90.0).arg(
                                                                                                static_cast<double>(x)*QGCMapEngine::srtm1TileSize - 180.0).arg(
                                                                                                static_cast<double>(y + 1)*QGCMapEngine::srtm1TileSize - 90.0).arg(
                                                                                                static_cast<double>(x + 1)*QGCMapEngine::srtm1TileSize - 180.0);
    }
    break;

    case VWorldStreet :
    {
        int gap = zoom - 6;
        int x_min = 53 * pow(2, gap);
        int x_max = 55 * pow(2, gap) + (2*gap - 1);
        int y_min = 22 * pow(2, gap);
        int y_max = 26 * pow(2, gap) + (2*gap - 1);

        if ( zoom > 19 ) {
            return {};
        }
        else if ( zoom > 5 && x >= x_min && x <= x_max && y >= y_min && y <= y_max ) {
            return QString("http://xdworld.vworld.kr:8080/2d/Base/service/%1/%2/%3.png").arg(zoom).arg(x).arg(y);
        }
        else {
            QString key = _tileXYToQuadKey(x, y, zoom);
            return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4").arg(_getServerNum(x, y, 4)).arg(key).arg(_versionBingMaps).arg(_language);
        }


    }
        break;

    case VWorldSatellite :
    {
        int gap = zoom - 6;
        int x_min = 53 * pow(2, gap);
        int x_max = 55 * pow(2, gap) + (2*gap - 1);
        int y_min = 22 * pow(2, gap);
        int y_max = 26 * pow(2, gap) + (2*gap - 1);

        if ( zoom > 19 ) {
            return {};
        }
        else if ( zoom > 5 && x >= x_min && x <= x_max && y >= y_min && y <= y_max ) {
            return QString("http://xdworld.vworld.kr:8080/2d/Satellite/service/%1/%2/%3.jpeg").arg(zoom).arg(x).arg(y);
        }
        else {
            QString key = _tileXYToQuadKey(x, y, zoom);
            return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4").arg(_getServerNum(x, y, 4)).arg(key).arg(_versionBingMaps).arg(_language);
        }
    }
        break;
    default:
        qWarning("Unknown map id %d\n", type);
        break;
    }
    return {};
}


//-----------------------------------------------------------------------------

#endif

//-----------------------------------------------------------------------------
quint32 UrlFactory::averageSizeForType(QString type) {
    qDebug() << "UrlFactory::averageSizeForType for" << type;
    if (_providersTable.find(type) != _providersTable.end()) {
        return _providersTable[type]->getAverageSize();
    } else {
        qDebug() << "UrlFactory::averageSizeForType " << type
                 << " Not registered";
    }

    //    switch (type) {
    //    case GoogleMap:
    //        return AVERAGE_GOOGLE_STREET_MAP;
    //    case BingMap:
    //        return AVERAGE_BING_STREET_MAP;
    //    case GoogleSatellite:
    //        return AVERAGE_GOOGLE_SAT_MAP;
    //    case MapboxSatellite:
    //        return AVERAGE_MAPBOX_SAT_MAP;
    //    case BingHybrid:
    //    case BingSatellite:
    //        return AVERAGE_BING_SAT_MAP;
    //    case GoogleTerrain:
    //        return AVERAGE_GOOGLE_TERRAIN_MAP;
    //    case MapboxStreets:
    //    case MapboxStreetsBasic:
    //    case MapboxRunBikeHike:
    //        return AVERAGE_MAPBOX_STREET_MAP;
    //    case AirmapElevation:
    //        return AVERAGE_AIRMAP_ELEV_SIZE;
    //    case GoogleLabels:
    //    case MapboxDark:
    //    case MapboxLight:
    //    case MapboxOutdoors:
    //    case MapboxPencil:
    //    case OpenStreetMap:
    //    case GoogleHybrid:
    //    case MapboxComic:
    //    case MapboxEmerald:
    //    case MapboxHighContrast:
    //    case MapboxHybrid:
    //    case MapboxPirates:
    //    case MapboxWheatPaste:
    //    default:
    //        break;
    //    }
    //    return AVERAGE_TILE_SIZE;
}

QString UrlFactory::getTypeFromId(int id) {

    QHashIterator<QString, MapProvider*> i(_providersTable);

    while (i.hasNext()) {
        i.next();
        if (abs(qHash(i.key())) == id) {
            return i.key();
        }
    }
    return "";
}

// Todo : qHash produce a uint bigger than max(int)
// There is still a low probability for abs to
// generate similar hash for different types
int UrlFactory::getIdFromType(QString type) { return abs(qHash(type)); }
