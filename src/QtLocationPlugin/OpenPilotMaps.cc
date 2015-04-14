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
 *  @brief  QGC Open Pilot Mapping Tools
 *  @author Gus Grubba <mavlink@grubba.com>
 *  Original work: The OpenPilot Team, http://www.openpilot.org Copyright (C) 2012.
 */

#include <QRegExp>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

#include "OpenPilotMaps.h"

namespace OpenPilot {

const QString ProviderStrings::kLevelsForSigPacSpainMap[] =
    { "0",         "1",         "2",         "3",       "4",
      "MTNSIGPAC",
      "MTN2000",   "MTN2000",   "MTN2000",   "MTN2000", "MTN2000",
      "MTN200",    "MTN200",    "MTN200",
      "MTN25",     "MTN25",
      "ORTOFOTOS", "ORTOFOTOS", "ORTOFOTOS", "ORTOFOTOS" };

ProviderStrings::ProviderStrings()
{
    // Google version strings
    VersionGoogleMap            = "m@296306248";
    VersionGoogleSatellite      = "s@168";
    VersionGoogleLabels         = "h@296000000";
    VersionGoogleTerrain        = "t@132,r@296000000";
    SecGoogleWord               = "Galileo";

    // Google (China) version strings
    VersionGoogleMapChina       = "m@132";
    VersionGoogleSatelliteChina = "s@71";
    VersionGoogleLabelsChina    = "h@132";
    VersionGoogleTerrainChina   = "t@125,r@132";

    // Google (Korea) version strings
    VersionGoogleMapKorea       = "kr1.12";
    VersionGoogleSatelliteKorea = "66";
    VersionGoogleLabelsKorea    = "kr1t.12";

    /// <summary>
    /// Google Maps API generated using http://greatmaps.codeplex.com/
    /// from http://code.google.com/intl/en-us/apis/maps/signup.html
    /// </summary>
    GoogleMapsAPIKey      = "ABQIAAAAWaQgWiEBF3lW97ifKnAczhRAzBk5Igf8Z5n2W3hNnMT0j2TikxTLtVIGU7hCLLHMAuAMt-BO5UrEWA";

    // Yahoo version strings
    VersionYahooMap             = "4.3";
    VersionYahooSatellite       = "1.9";
    VersionYahooLabels          = "4.3";

    // BingMaps
    VersionBingMaps             = "563";

    // YandexMap
    VersionYandexMap            = "2.16.0";

    /// <summary>
    /// Bing Maps Customer Identification, more info here
    /// http://msdn.microsoft.com/en-us/library/bb924353.aspx
    /// </summary>
    BingMapsClientToken = "";
}

UrlFactory::UrlFactory(QNetworkAccessManager *network)
    : _timeout(5 * 1000)
    , _googleVersionRetrieved(false)
    , _network(network)
    , _googleReply(NULL)
{
}

UrlFactory::~UrlFactory()
{
    if(_googleReply)
        _googleReply->deleteLater();
}

QString UrlFactory::_tileXYToQuadKey(const int& tileX, const int& tileY, const int& levelOfDetail) const
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

int UrlFactory::_getServerNum(const QPoint &pos, const int &max) const
{
    return (pos.x() + 2 * pos.y()) % max;
}

void UrlFactory::_networkReplyError(QNetworkReply::NetworkError error)
{
    qWarning() << "Could not connect to google maps. Error:" << error;
    if(_googleReply)
    {
        _googleReply->deleteLater();
        _googleReply = NULL;
    }
}

void UrlFactory::_replyDestroyed()
{
    _googleReply = NULL;
}

void UrlFactory::_googleVersionCompleted()
{
    if (!_googleReply || (_googleReply->error() != QNetworkReply::NoError)) {
        qDebug() << "Error collecting Google maps version info";
        return;
    }
    QString html = QString(_googleReply->readAll());
    QRegExp reg("\"*http://mts0.google.com/vt/lyrs=m@(\\d*)", Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        VersionGoogleMap = QString("m@%1").arg(gc[1]);
        VersionGoogleMapChina = VersionGoogleMap;
        VersionGoogleMapKorea = VersionGoogleMap;
    }
    reg = QRegExp("\"*http://mts0.google.com/vt/lyrs=h@(\\d*)", Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        VersionGoogleLabels = QString("h@%1").arg(gc[1]);
        VersionGoogleLabelsChina = VersionGoogleLabels;
        VersionGoogleLabelsKorea = VersionGoogleLabels;
    }
    reg = QRegExp("\"*http://khms0.google.com/kh/v=(\\d*)", Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        VersionGoogleSatellite = "s@" + gc[1];
        VersionGoogleSatelliteKorea = VersionGoogleSatellite;
        VersionGoogleSatelliteChina = VersionGoogleSatellite;
    }
    reg = QRegExp("\"*http://mts0.google.com/vt/lyrs=t@(\\d*),r@(\\d*)", Qt::CaseInsensitive);
    if (reg.indexIn(html) != -1) {
        QStringList gc = reg.capturedTexts();
        VersionGoogleTerrain = QString("t@%1,r@%2").arg(gc[1]).arg(gc[2]);
        VersionGoogleTerrainChina = VersionGoogleTerrain;
        VersionGoogleTerrainChina = VersionGoogleTerrain;
    }
    _googleReply->deleteLater();
    _googleReply = NULL;
}

void UrlFactory::_tryCorrectGoogleVersions()
{
    QMutexLocker locker(&_googleVersionMutex);
    if (_googleVersionRetrieved) {
        return;
    }
    _googleVersionRetrieved = true;
    if(_network)
    {
        QNetworkRequest qheader;
        QNetworkProxy proxy = _network->proxy();
        QNetworkProxy tProxy;
        tProxy.setType(QNetworkProxy::NoProxy);
        _network->setProxy(tProxy);
        QString url = "http://maps.google.com/maps?output=classic";
        qheader.setUrl(QUrl(url));
        QByteArray userAgent = "Mozilla/5.0 (Windows; U; Windows NT 6.0; en-US; rv:1.9.1.7) Gecko/20091221 Firefox/3.5.7";
        qheader.setRawHeader("User-Agent", userAgent);
        _googleReply = _network->get(qheader);
        connect(_googleReply, SIGNAL(finished()), this, SLOT(_googleVersionCompleted()));
        connect(_googleReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(_networkReplyError(QNetworkReply::NetworkError)));
        connect(_googleReply, SIGNAL(destroyed()), this, SLOT(_replyDestroyed()));
        _network->setProxy(proxy);
    }
}

QString UrlFactory::makeImageUrl(const MapType &type, const QPoint& pos, const int &zoom, const QString &language)
{
    switch (type) {
    case GoogleMap:
    {
        // http://mt1.google.com/vt/lyrs=m
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10&scale=2").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleMap).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleSatellite:
    {
        // http://mt1.google.com/vt/lyrs=s
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10&scale=2").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleSatellite).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleLabels:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleLabels).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleTerrain:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        return QString("http://%1%2.google.com/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10&scale=2").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleTerrain).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleMapChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2.101&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga
        return QString("http://%1%2.google.cn/%3/lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleMapChina).arg("zh-CN").arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleSatelliteChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://khm0.google.cn/kh/v=46&x=12&y=6&z=4&s=Ga
        return QString("http://%1%2.google.cn/%3/lyrs=%4&gl=cn&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleSatelliteChina).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleLabelsChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2t.110&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga
        return QString("http://%1%2.google.cn/%3/imgtp=png32&lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsChina).arg("zh-CN").arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleTerrainChina:
    {
        QString server  = "mt";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://mt0.google.cn/vt/v=w2p.110&hl=zh-CN&gl=cn&x=12&y=6&z=4&s=Ga
        return QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&gl=cn&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleTerrainChina).arg("zh-CN").arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleMapKorea:
    {
        QString server  = "mts";
        QString request = "vt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://mts0.google.com/vt/lyrs=m@224000000&hl=ko&gl=KR&src=app&x=107&y=50&z=7&s=Gal
        // http://mts0.google.com/mt/v=kr1.11&hl=ko&x=109&y=49&z=7&s=
        QString ret = QString("http://%1%2.google.com/%3/lyrs=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleMapKorea).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
        return ret;
    }
    break;
    case GoogleSatelliteKorea:
    {
        QString server  = "khms";
        QString request = "kh";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://khm1.google.co.kr/kh/v=54&x=109&y=49&z=7&s=
        return QString("http://%1%2.google.co.kr/%3/v=%4&x=%5%6&y=%7&z=%8&s=%9").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleSatelliteKorea).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case GoogleLabelsKorea:
    {
        QString server  = "mts";
        QString request = "mt";
        QString sec1    = ""; // after &x=...
        QString sec2    = ""; // after &zoom=...
        _getSecGoogleWords(pos, sec1, sec2);
        _tryCorrectGoogleVersions();
        // http://mts1.gmaptiles.co.kr/mt/v=kr1t.11&hl=lt&x=109&y=50&z=7&s=G
        return QString("http://%1%2.gmaptiles.co.kr/%3/v=%4&hl=%5&x=%6%7&y=%8&z=%9&s=%10").arg(server).arg(_getServerNum(pos, 4)).arg(request).arg(VersionGoogleLabelsKorea).arg(language).arg(pos.x()).arg(sec1).arg(pos.y()).arg(zoom).arg(sec2);
    }
    break;
    case YahooMap:
    {
        return QString("http://maps%1.yimg.com/hx/tl?v=%2&.intl=%3&x=%4&y=%5&z=%6&r=1").arg(((_getServerNum(pos, 2)) + 1)).arg(VersionYahooMap).arg(language).arg(pos.x()).arg((((1 << zoom) >> 1) - 1 - pos.y())).arg((zoom + 1));
    }
    case YahooSatellite:
    {
        return QString("http://maps%1.yimg.com/ae/ximg?v=%2&t=a&s=256&.intl=%3&x=%4&y=%5&z=%6&r=1").arg("3").arg(VersionYahooSatellite).arg(language).arg(pos.x()).arg(((1 << zoom) >> 1) - 1 - pos.y()).arg(zoom + 1);
    }
    break;
    case YahooLabels:
    {
        return QString("http://maps%1.yimg.com/hx/tl?v=%2&t=h&.intl=%3&x=%4&y=%5&z=%6&r=1").arg("1").arg(VersionYahooLabels).arg(language).arg(pos.x()).arg(((1 << zoom) >> 1) - 1 - pos.y()).arg(zoom + 1);
    }
    break;
    case OpenStreetMap:
    {
        char letter = "abc"[_getServerNum(pos, 3)];
        return QString("http://%1.tile.openstreetmap.org/%2/%3/%4.png").arg(letter).arg(zoom).arg(pos.x()).arg(pos.y());
    }
    break;
    case OpenStreetOsm:
    {
        char letter = "abc"[_getServerNum(pos, 3)];
        return QString("http://%1.tah.openstreetmap.org/Tiles/tile/%2/%3/%4.png").arg(letter).arg(zoom).arg(pos.x()).arg(pos.y());
    }
    break;
    case OpenStreetMapSurfer:
    {
        // http://tiles1.mapsurfer.net/tms_r.ashx?x=37378&y=20826&z=16
        return QString("http://tiles1.mapsurfer.net/tms_r.ashx?x=%1&y=%2&z=%3").arg(pos.x()).arg(pos.y()).arg(zoom);
    }
    break;
    case OpenStreetMapSurferTerrain:
    {
        // http://tiles2.mapsurfer.net/tms_t.ashx?x=9346&y=5209&z=14
        return QString("http://tiles2.mapsurfer.net/tms_t.ashx?x=%1&y=%2&z=%3").arg(pos.x()).arg(pos.y()).arg(zoom);
    }
    break;
    case BingMap:
    {
        QString key = _tileXYToQuadKey(pos.x(), pos.y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/r%2.png?g=%3&mkt=%4%5").arg(_getServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }
    break;
    case BingSatellite:
    {
        QString key = _tileXYToQuadKey(pos.x(), pos.y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/a%2.jpeg?g=%3&mkt=%4%5").arg(_getServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }
    break;
    case BingHybrid:
    {
        QString key = _tileXYToQuadKey(pos.x(), pos.y(), zoom);
        return QString("http://ecn.t%1.tiles.virtualearth.net/tiles/h%2.jpeg?g=%3&mkt=%4%5").arg(_getServerNum(pos, 4)).arg(key).arg(VersionBingMaps).arg(language).arg(!(BingMapsClientToken.isNull() | BingMapsClientToken.isEmpty()) ? "&token=" + BingMapsClientToken : QString(""));
    }
    case ArcGIS_Map:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_StreetMap_World_2D/MapServer/tile/0/0/0.jpg
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_StreetMap_World_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.y()).arg(pos.x());
    }
    break;
    case ArcGIS_Satellite:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_Imagery_World_2D/MapServer/tile/1/0/1.jpg
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_Imagery_World_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.y()).arg(pos.x());
    }
    break;
    case ArcGIS_ShadedRelief:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_ShadedRelief_World_2D/MapServer/tile/1/0/1.jpg
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/ESRI_ShadedRelief_World_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.y()).arg(pos.x());
    }
    break;
    case ArcGIS_Terrain:
    {
        // http://server.arcgisonline.com/ArcGIS/rest/services/NGS_Topo_US_2D/MapServer/tile/4/3/15
        return QString("http://server.arcgisonline.com/ArcGIS/rest/services/NGS_Topo_US_2D/MapServer/tile/%1/%2/%3").arg(zoom).arg(pos.y()).arg(pos.x());
    }
    break;
    case ArcGIS_MapsLT_OrtoFoto:
    {
        // http://www.maps.lt/ortofoto/mapslt_ortofoto_vector_512/map/_alllayers/L02/R0000001b/C00000028.jpg
        // http://arcgis.maps.lt/ArcGIS/rest/services/mapslt_ortofoto/MapServer/tile/0/9/13
        // return string.Format("http://www.maps.lt/ortofoto/mapslt_ortofoto_vector_512/map/_alllayers/L{0:00}/R{1:x8}/C{2:x8}.jpg", zoom, pos.y(), pos.x());
        // http://dc1.maps.lt/cache/mapslt_ortofoto_512/map/_alllayers/L03/R0000001c/C00000029.jpg
        // return string.Format("http://arcgis.maps.lt/ArcGIS/rest/services/mapslt_ortofoto/MapServer/tile/{0}/{1}/{2}", zoom, pos.y(), pos.x());
        // http://dc1.maps.lt/cache/mapslt_ortofoto_512/map/_alllayers/L03/R0000001d/C0000002a.jpg
        // TODO verificar
        return QString("http://dc1.maps.lt/cache/mapslt_ortofoto/map/_alllayers/L%1/R%2/C%3.jpg").arg(zoom, 2, 10, (QChar)'0').arg(pos.y(), 8, 16, (QChar)'0').arg(pos.x(), 8, 16, (QChar)'0');
    }
    break;
    case ArcGIS_MapsLT_Map:
    {
        // http://www.maps.lt/ortofoto/mapslt_ortofoto_vector_512/map/_alllayers/L02/R0000001b/C00000028.jpg
        // http://arcgis.maps.lt/ArcGIS/rest/services/mapslt_ortofoto/MapServer/tile/0/9/13
        // return string.Format("http://www.maps.lt/ortofoto/mapslt_ortofoto_vector_512/map/_alllayers/L{0:00}/R{1:x8}/C{2:x8}.jpg", zoom, pos.y(), pos.x());
        // http://arcgis.maps.lt/ArcGIS/rest/services/mapslt/MapServer/tile/7/1162/1684.png
        // http://dc1.maps.lt/cache/mapslt_512/map/_alllayers/L03/R0000001b/C00000029.png
        // TODO verificar
        // http://dc1.maps.lt/cache/mapslt/map/_alllayers/L02/R0000001c/C00000029.png
        return QString("http://dc1.maps.lt/cache/mapslt/map/_alllayers/L%1/R%2/C%3.png").arg(zoom, 2, 10, (QChar)'0').arg(pos.y(), 8, 16, (QChar)'0').arg(pos.x(), 8, 16, (QChar)'0');
    }
    break;
    case ArcGIS_MapsLT_Map_Labels:
    {
        // http://arcgis.maps.lt/ArcGIS/rest/services/mapslt_ortofoto_overlay/MapServer/tile/0/9/13
        // return string.Format("http://arcgis.maps.lt/ArcGIS/rest/services/mapslt_ortofoto_overlay/MapServer/tile/{0}/{1}/{2}", zoom, pos.y(), pos.x());
        // http://dc1.maps.lt/cache/mapslt_ortofoto_overlay_512/map/_alllayers/L03/R0000001d/C00000029.png
        // TODO verificar
        return QString("http://dc1.maps.lt/cache/mapslt_ortofoto_overlay/map/_alllayers/L%1/R%2/C%3.png").arg(zoom, 2, 10, (QChar)'0').arg(pos.y(), 8, 16, (QChar)'0').arg(pos.x(), 8, 16, (QChar)'0');
    }
    break;
    case PergoTurkeyMap:
    {
        // http://{domain}/{layerName}/{zoomLevel}/{first3LetterOfTileX}/{second3LetterOfTileX}/{third3LetterOfTileX}/{first3LetterOfTileY}/{second3LetterOfTileY}/{third3LetterOfTileXY}.png
        // http://map3.pergo.com.tr/tile/00/000/000/001/000/000/000.png
        // That means: Zoom Level: 0 TileX: 1 TileY: 0
        // http://domain/tile/14/000/019/371/000/011/825.png
        // That means: Zoom Level: 14 TileX: 19371 TileY:11825
        // string x = pos.x().ToString("000000000").Insert(3, "/").Insert(7, "/"); // - 000/000/001
        // string y = pos.y().ToString("000000000").Insert(3, "/").Insert(7, "/"); // - 000/000/000
        QString x = QString("%1").arg(QString::number(pos.x()), 9, (QChar)'0');
        x.insert(3, "/").insert(7, "/");
        QString y = QString("%1").arg(QString::number(pos.y()), 9, (QChar)'0');
        y.insert(3, "/").insert(7, "/");
        // "http://map03.pergo.com.tr/tile/2/000/000/003/000/000/002.png"
        return QString("http://map%1.pergo.com.tr/tile/%2/%3/%4.png").arg(_getServerNum(pos, 4)).arg(zoom, 2, 10, (QChar)'0').arg(x).arg(y);
    }
    break;
    case SigPacSpainMap:
    {
        return QString("http://sigpac.mapa.es/kmlserver/raster/%1@3785/%2.%3.%4.img").arg(kLevelsForSigPacSpainMap[zoom]).arg(zoom).arg(pos.x()).arg((2 << (zoom - 1)) - pos.y() - 1);
    }
    break;
    case YandexMapRu:
    {
        QString server = "vec";
        // http://vec01.maps.yandex.ru/tiles?l=map&v=2.10.2&x=1494&y=650&z=11
        return QString("http://%1").arg(server) + QString("0%2.maps.yandex.ru/tiles?l=map&v=%3&x=%4&y=%5&z=%6").arg(_getServerNum(pos, 4) + 1).arg(VersionYandexMap).arg(pos.x()).arg(pos.y()).arg(zoom);
    }
    break;
    default:
        qWarning("Unknown map id %d\n", type);
        break;
    }
    return QString::null;
}

void UrlFactory::_getSecGoogleWords(const QPoint &pos, QString &sec1, QString &sec2)
{
    sec1 = ""; // after &x=...
    sec2 = ""; // after &zoom=...
    int seclen = ((pos.x() * 3) + pos.y()) % 8;
    sec2 = SecGoogleWord.left(seclen);
    if (pos.y() >= 10000 && pos.y() < 100000) {
        sec1 = "&s=";
    }
}

}
