/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EsriMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

EsriMapProvider::EsriMapProvider(const quint32 averageSize, const QGeoMapType::MapStyle mapType, QObject *parent)
    : MapProvider(QString(), QString(), averageSize, mapType, parent) {}

QNetworkRequest EsriMapProvider::getTileURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    //-- Build URL
    QNetworkRequest request;
    const QString url = _getURL(x, y, zoom, networkManager);
    if (url.isEmpty()) {
        return request;
    }
    request.setUrl(QUrl(url));
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    const QByteArray token = qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().toLatin1();
    request.setRawHeader(QByteArrayLiteral("User-Agent"), QByteArrayLiteral("Qt Location based application"));
    request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    return request;
}

static const QString WorldStreetMapUrl = QStringLiteral("http://services.arcgisonline.com/ArcGIS/rest/services/World_Street_Map/MapServer/tile/%1/%2/%3");

QString EsriWorldStreetMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    return WorldStreetMapUrl.arg(zoom).arg(y).arg(x);
}

static const QString WorldSatelliteMapUrl = QStringLiteral("http://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/%1/%2/%3");

QString EsriWorldSatelliteMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    return WorldSatelliteMapUrl.arg(zoom).arg(y).arg(x);
}

static const QString TerrainMapUrl = QStringLiteral("http://server.arcgisonline.com/ArcGIS/rest/services/World_Terrain_Base/MapServer/tile/%1/%2/%3");

QString EsriTerrainMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    return TerrainMapUrl.arg(zoom).arg(y).arg(x);
}
