#include "EsriMapProvider.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

QNetworkRequest
EsriMapProvider::getTileURL(int x, int y, int zoom,
                            QNetworkAccessManager* networkManager) {
    //-- Build URL
    QNetworkRequest request;
    QString         url = _getURL(x, y, zoom, networkManager);
    if (url.isEmpty()) {
        return request;
    }
    request.setUrl(QUrl(url));
    request.setRawHeader("Accept", "*/*");
    QByteArray token = qgcApp()
                           ->toolbox()
                           ->settingsManager()
                           ->appSettings()
                           ->esriToken()
                           ->rawValue()
                           .toString()
                           .toLatin1();
    request.setRawHeader("User-Agent",
                         QByteArrayLiteral("Qt Location based application"));
    request.setRawHeader("User-Token", token);
    return request;
}

QString
EsriWorldStreetMapProvider::_getURL(int x, int y, int zoom,
                                    QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://services.arcgisonline.com/ArcGIS/rest/services/"
                   "World_Street_Map/MapServer/tile/%1/%2/%3")
        .arg(zoom)
        .arg(y)
        .arg(x);
}

QString
EsriWorldSatelliteMapProvider::_getURL(int x, int y, int zoom,
                                       QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://server.arcgisonline.com/ArcGIS/rest/"
                   "services/World_Imagery/MapServer/tile/%1/%2/%3")
        .arg(zoom)
        .arg(y)
        .arg(x);
}

QString EsriTerrainMapProvider::_getURL(int x, int y, int zoom,
                                        QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    return QString("http://server.arcgisonline.com/ArcGIS/rest/services/"
                   "World_Terrain_Base/MapServer/tile/%1/%2/%3")
        .arg(zoom)
        .arg(y)
        .arg(x);
}
