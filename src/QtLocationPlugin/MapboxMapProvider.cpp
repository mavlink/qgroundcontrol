#include "MapboxMapProvider.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

MapboxMapProvider::MapboxMapProvider(QString mapName, quint32 averageSize,
                                     QGeoMapType::MapStyle mapType,
                                     QObject*              parent)
    : MapProvider(QString("https://www.mapbox.com/"), QString("jpg"),
                  averageSize, mapType, parent), mapboxName(mapName) {
}

QString
MapboxMapProvider::_getURL(int x, int y, int zoom,
                                  QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager);
    QString mapBoxToken = qgcApp()
                              ->toolbox()
                              ->settingsManager()
                              ->appSettings()
                              ->mapboxToken()
                              ->rawValue()
                              .toString();
    if (!mapBoxToken.isEmpty()) {
        QString server = "https://api.mapbox.com/v4/";
        server += mapboxName;
        server += QString("/%1/%2/%3.jpg80?access_token=%4")
                      .arg(zoom)
                      .arg(x)
                      .arg(y)
                      .arg(mapBoxToken);
        return server;
    }
    return QString("");
}
