/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GenericMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

QString CustomURLMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QRegularExpression regExpZoom = QRegularExpression("\\{(z|zoom)\\}");
    QString url = qgcApp()->toolbox()->settingsManager()->appSettings()->customURL()->rawValue().toString();
    return url.replace("{x}", QString::number(x)).replace("{y}", QString::number(y)).replace(regExpZoom, QString::number(zoom));
}

QString JapanStdMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString JapanStdMapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/std/%1/%2/%3.png");
    return JapanStdMapUrl.arg(zoom).arg(x).arg(y);
}

QString JapanSeamlessMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString JapanSeamlessMapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/seamlessphoto/%1/%2/%3.jpg");
    return JapanSeamlessMapUrl.arg(zoom).arg(x).arg(y);
}

QString JapanAnaglyphMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString JapanAnaglyphMapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/anaglyphmap_color/%1/%2/%3.png");
    return JapanAnaglyphMapUrl.arg(zoom).arg(x).arg(y);
}

QString JapanSlopeMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString JapanSlopeMapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/slopemap/%1/%2/%3.png");
    return JapanSlopeMapUrl.arg(zoom).arg(x).arg(y);
}

QString JapanReliefMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString JapanReliefMapUrl = QStringLiteral("https://cyberjapandata.gsi.go.jp/xyz/relief/%1/%2/%3.png");
    return JapanReliefMapUrl.arg(zoom).arg(x).arg(y);
}

QString LINZBasemapMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString LINZBasemapMapUrl = QStringLiteral("https://basemaps.linz.govt.nz/v1/tiles/aerial/EPSG:3857/%1/%2/%3.png?api=d01ev80nqcjxddfvc6amyvkk1ka");
    return LINZBasemapMapUrl.arg(zoom).arg(x).arg(y);
}

QString StatkartMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString StatkartMapUrl = QStringLiteral("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom=%1&x=%2&y=%3");
    return StatkartMapUrl.arg(zoom).arg(x).arg(y);
}

QString StatkartBaseMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString StatkartBaseMapUrl = QStringLiteral("http://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=norgeskart_bakgrunn&zoom=%1&x=%2&y=%3");
    return StatkartBaseMapUrl.arg(zoom).arg(x).arg(y);
}


QString EniroMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString EniroMapUrl = QStringLiteral("http://map.eniro.com/geowebcache/service/tms1.0.0/map/%1/%2/%3.png");
    return EniroMapUrl.arg(zoom).arg(x).arg((1 << zoom) - 1 - y);
}


QString MapQuestMapMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString MapQuestMapUrl = QStringLiteral("http://otile%1.mqcdn.com/tiles/1.0.0/map/%2/%3/%4.jpg");
    return MapQuestMapUrl.arg(_getServerNum(x, y, 4)).arg(zoom).arg(x).arg(y);
}


QString MapQuestSatMapProvider::_getURL(int x, int y, int zoom) const
{
    static const QString MapQuestSatUrl = QStringLiteral("http://otile%1.mqcdn.com/tiles/1.0.0/sat/%2/%3/%4.jpg");
    return MapQuestSatUrl.arg(_getServerNum(x, y, 4)).arg(zoom).arg(x).arg(y);
}

QString VWorldMapProvider::_getURL(int x, int y, int zoom) const
{
    const int gap   = zoom - 6;
    const int x_min = int(53 * pow(2, gap));
    const int x_max = int(55 * pow(2, gap) + (2 * gap - 1));
    const int y_min = int(22 * pow(2, gap));
    const int y_max = int(26 * pow(2, gap) + (2 * gap - 1));

    const QString VWorldMapToken = qgcApp()->toolbox()->settingsManager()->appSettings()->vworldToken()->rawValue().toString();

    QString result;
    if (zoom > 19) {
        result = QString();
    } else if ((zoom > 5) && (x >= x_min) && (x <= x_max) && (y >= y_min) && (y <= y_max)) {
        QString mapTypeCode;
        if (m_mapStyle == QGeoMapType::StreetMap) {
            mapTypeCode = "Base";
        } else if (m_mapStyle == QGeoMapType::SatelliteMapDay) {
            mapTypeCode = "Satellite";
        }
        result = QString("http://api.vworld.kr/req/wmts/1.0.0/%1/%2/%3/%4/%5.%6").arg(VWorldMapToken, mapTypeCode).arg(zoom).arg(y).arg(x).arg(m_imageFormat);
    } else {
        const QString key = _tileXYToQuadKey(x, y, zoom);
        QString mapTypeCode;
        if (m_mapStyle == QGeoMapType::StreetMap) {
            mapTypeCode = "r";
        } else if (m_mapStyle == QGeoMapType::SatelliteMapDay) {
            mapTypeCode = "a";
        }
        result = QString(QStringLiteral("http://ecn.t%1.tiles.virtualearth.net/tiles/%2%3.%4?g=%5&mkt=%6"))
            .arg(QString::number(_getServerNum(x, y, 4)), mapTypeCode, key, m_imageFormat, m_versionBingMaps, qgcApp()->getCurrentLanguage().uiLanguages().first());
    }
    return result;
}
