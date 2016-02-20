/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 *  @file
 *  @author Gus Grubba <mavlink@grubba.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 */

#include "QGCMapEngine.h"

#include <QRegExp>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QString>
#include <QByteArray>

//-----------------------------------------------------------------------------
UrlFactory::UrlFactory()
    : _timeout(5 * 1000)
    , _googleVersionRetrieved(false)
    , _googleReply(NULL)
{
    QStringList langs = QLocale::system().uiLanguages();
    if (langs.length() > 0) {
        _language = langs[0];
    }
    // Google version strings
    _versionGoogleMap            = "m@336";
    _versionGoogleSatellite      = "194";
    _versionGoogleLabels         = "h@336";
    _versionGoogleTerrain        = "t@132,r@336";
    _secGoogleWord               = "Galileo";
    // BingMaps
    _versionBingMaps             = "563";
}

//-----------------------------------------------------------------------------
UrlFactory::~UrlFactory()
{
    if(_googleReply)
        _googleReply->deleteLater();
}


//-----------------------------------------------------------------------------
QString
UrlFactory::getImageFormat(MapType type, const QByteArray& image)
{
    QString format;
    if(image.size() > 2)
    {
        if((char)image[0] == (char)0xff && (char)image[1] == (char)0xd8)
            format = "jpg";
        else if((char)image[0] == (char)0x89 && (char)image[1] == (char)0x50)
            format = "png";
        else {
            switch (type) {
                case GoogleMap:
                case GoogleLabels:
                case GoogleTerrain:
                case GoogleHybrid:
                case BingMap:
                case OpenStreetMap:
                    format = "png";
                    break;
                case MapQuestMap:
                case MapQuestSat:
                case MapBoxStreets:
                case MapBoxLight:
                case MapBoxDark:
                case MapBoxSatellite:
                case MapBoxHybrid:
                case MapBoxWheatPaste:
                case MapBoxStreetsBasic:
                case MapBoxComic:
                case MapBoxOutdoors:
                case MapBoxRunBikeHike:
                case MapBoxPencil:
                case MapBoxPirates:
                case MapBoxEmerald:
                case MapBoxHighContrast:
                case GoogleSatellite:
                case BingSatellite:
                case BingHybrid:
                    format = "jpg";
                    break;
                default:
                    qWarning("UrlFactory::getImageFormat() Unknown map id %d", type);
                    break;
            }
        }
    }
    return format;
}

//-----------------------------------------------------------------------------
QNetworkRequest
UrlFactory::getTileURL(MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager)
{
    //-- Build URL
    QNetworkRequest request;
    QString url = _getURL(type, x, y, zoom, networkManager);
    if(url.isEmpty())
        return request;
    request.setUrl(QUrl(url));
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("User-Agent", _userAgent);
    switch (type) {
        case GoogleMap:
        case GoogleSatellite:
        case GoogleLabels:
        case GoogleTerrain:
        case GoogleHybrid:
            request.setRawHeader("Referrer", "http://maps.google.com/");
            break;
        case BingHybrid:
        case BingMap:
        case BingSatellite:
            request.setRawHeader("Referrer", "http://www.bing.com/maps/");
            break;
        /*
        case OpenStreetMapSurfer:
        case OpenStreetMapSurferTerrain:
            request.setRawHeader("Referrer", "http://www.mapsurfer.net/");
            break;
        case OpenStreetMap:
        case OpenStreetOsm:
            request.setRawHeader("Referrer", "https://www.openstreetmap.org/");
            break;
        */
        default:
            break;
    }
    return request;
}

//-----------------------------------------------------------------------------
void
UrlFactory::_getSecGoogleWords(int x, int y, QString &sec1, QString &sec2)
{
    sec1 = ""; // after &x=...
    sec2 = ""; // after &zoom=...
    int seclen = ((x * 3) + y) % 8;
    sec2 = _secGoogleWord.left(seclen);
    if (y >= 10000 && y < 100000) {
        sec1 = "&s=";
    }
}

//-----------------------------------------------------------------------------
QString
UrlFactory::_getURL(MapType type, int x, int y, int zoom, QNetworkAccessManager* networkManager)
{
    switch (type) {
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

    case MapBoxStreets:
    case MapBoxLight:
    case MapBoxDark:
    case MapBoxSatellite:
    case MapBoxHybrid:
    case MapBoxWheatPaste:
    case MapBoxStreetsBasic:
    case MapBoxComic:
    case MapBoxOutdoors:
    case MapBoxRunBikeHike:
    case MapBoxPencil:
    case MapBoxPirates:
    case MapBoxEmerald:
    case MapBoxHighContrast:
    {
        QString mapBoxToken = getQGCMapEngine()->getMapBoxToken();
        if(!mapBoxToken.isEmpty()) {
            QString server = "https://api.mapbox.com/v4/";
            switch(type) {
                case MapBoxStreets:
                    server += "mapbox.streets";
                    break;
                case MapBoxLight:
                    server += "mapbox.light";
                    break;
                case MapBoxDark:
                    server += "mapbox.dark";
                    break;
                case MapBoxSatellite:
                    server += "mapbox.satellite";
                    break;
                case MapBoxHybrid:
                    server += "mapbox.streets-satellite";
                    break;
                case MapBoxWheatPaste:
                    server += "mapbox.wheatpaste";
                    break;
                case MapBoxStreetsBasic:
                    server += "mapbox.streets-basic";
                    break;
                case MapBoxComic:
                    server += "mapbox.comic";
                    break;
                case MapBoxOutdoors:
                    server += "mapbox.outdoors";
                    break;
                case MapBoxRunBikeHike:
                    server += "mapbox.run-bike-hike";
                    break;
                case MapBoxPencil:
                    server += "mapbox.pencil";
                    break;
                case MapBoxPirates:
                    server += "mapbox.pirates";
                    break;
                case MapBoxEmerald:
                    server += "mapbox.emerald";
                    break;
                case MapBoxHighContrast:
                    server += "mapbox.high-contrast";
                    break;
                default:
                    return QString::null;
            }
            server += QString("/%1/%2/%3.jpg80?access_token=%4").arg(zoom).arg(x).arg(y).arg(mapBoxToken);
            return server;
        }
    }
    break;

    default:
        qWarning("Unknown map id %d\n", type);
        break;
    }
    return QString::null;
}

//-----------------------------------------------------------------------------
QString
UrlFactory::_tileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
    QString quadKey;
    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        int mask   = 1 << (i - 1);
        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit++;
            digit++;
        }
        quadKey.append(digit);
    }
    return quadKey;
}

//-----------------------------------------------------------------------------
int
UrlFactory::_getServerNum(int x, int y, int max)
{
    return (x + 2 * y) % max;
}

//-----------------------------------------------------------------------------
void
UrlFactory::_networkReplyError(QNetworkReply::NetworkError error)
{
    qWarning() << "Could not connect to google maps. Error:" << error;
    if(_googleReply)
    {
        _googleReply->deleteLater();
        _googleReply = NULL;
    }
}

//-----------------------------------------------------------------------------
void
UrlFactory::_replyDestroyed()
{
    _googleReply = NULL;
}

//-----------------------------------------------------------------------------
void
UrlFactory::_googleVersionCompleted()
{
    if (!_googleReply || (_googleReply->error() != QNetworkReply::NoError)) {
        qDebug() << "Error collecting Google maps version info";
        return;
    }
    QString html = QString(_googleReply->readAll());
    QRegExp reg("\"*http://mt0.google.com/vt/lyrs=m@(\\d*)",Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        _versionGoogleMap = QString("m@%1").arg(gc[1]);
    }
    reg = QRegExp("\"*http://mt0.google.com/vt/lyrs=h@(\\d*)",Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        _versionGoogleLabels = QString("h@%1").arg(gc[1]);
    }
    reg = QRegExp("\"*http://khm\\D?\\d.google.com/kh/v=(\\d*)",Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        _versionGoogleSatellite = gc[1];
    }
    reg = QRegExp("\"*http://mt0.google.com/vt/lyrs=t@(\\d*),r@(\\d*)",Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        _versionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
    }
    _googleReply->deleteLater();
    _googleReply = NULL;
}

//-----------------------------------------------------------------------------
void
UrlFactory::_tryCorrectGoogleVersions(QNetworkAccessManager*  networkManager)
{
    QMutexLocker locker(&_googleVersionMutex);
    if (_googleVersionRetrieved) {
        return;
    }
    _googleVersionRetrieved = true;
    if(networkManager)
    {
        QNetworkRequest qheader;
        QNetworkProxy proxy = networkManager->proxy();
        QNetworkProxy tProxy;
        tProxy.setType(QNetworkProxy::NoProxy);
        networkManager->setProxy(tProxy);
        QString url = "http://maps.google.com/maps";
        qheader.setUrl(QUrl(url));
        QByteArray ua;
        ua.append(getQGCMapEngine()->userAgent());
        qheader.setRawHeader("User-Agent", ua);
        _googleReply = networkManager->get(qheader);
        connect(_googleReply, &QNetworkReply::finished, this, &UrlFactory::_googleVersionCompleted);
        connect(_googleReply, &QNetworkReply::destroyed, this, &UrlFactory::_replyDestroyed);
        connect(_googleReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                this, &UrlFactory::_networkReplyError);
        networkManager->setProxy(proxy);
    }
}

#define AVERAGE_GOOGLE_STREET_MAP   4913
#define AVERAGE_GOOGLE_TERRAIN_MAP  19391
#define AVERAGE_BING_STREET_MAP     1297
#define AVERAGE_BING_SAT_MAP        19597
#define AVERAGE_GOOGLE_SAT_MAP      56887
#define AVERAGE_MAPBOX_SAT_MAP      15739
#define AVERAGE_MAPBOX_STREET_MAP   5648
#define AVERAGE_TILE_SIZE           13652

//-----------------------------------------------------------------------------
quint32
UrlFactory::averageSizeForType(MapType type)
{
    switch (type) {
    case GoogleMap:
        return AVERAGE_GOOGLE_STREET_MAP;
    case BingMap:
        return AVERAGE_BING_STREET_MAP;
    case GoogleSatellite:
        return AVERAGE_GOOGLE_SAT_MAP;
    case MapBoxSatellite:
        return AVERAGE_MAPBOX_SAT_MAP;
    case BingHybrid:
    case BingSatellite:
        return AVERAGE_BING_SAT_MAP;
    case GoogleTerrain:
        return AVERAGE_GOOGLE_TERRAIN_MAP;
    case MapBoxStreets:
    case MapBoxStreetsBasic:
    case MapBoxRunBikeHike:
        return AVERAGE_MAPBOX_STREET_MAP;
    case GoogleLabels:
    case MapBoxDark:
    case MapBoxLight:
    case MapBoxOutdoors:
    case MapBoxPencil:
    case OpenStreetMap:
    case GoogleHybrid:
    case MapBoxComic:
    case MapBoxEmerald:
    case MapBoxHighContrast:
    case MapBoxHybrid:
    case MapBoxPirates:
    case MapBoxWheatPaste:
    default:
        break;
    }
    return AVERAGE_TILE_SIZE;
}
