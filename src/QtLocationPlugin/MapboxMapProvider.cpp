#include "MapboxMapProvider.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/v4/%1/%2/%3/%4.jpg80?access_token=%5");

MapboxMapProvider::MapboxMapProvider(const QString &mapName, const quint32 averageSize, const QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QStringLiteral("https://www.mapbox.com/"), QStringLiteral("jpg"), averageSize, mapType, parent)
    , _mapboxName(mapName)
{
}

QString MapboxMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    const QString mapBoxToken = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString();
    if (!mapBoxToken.isEmpty()) {
        return MapBoxUrl.arg(_mapboxName).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
    }
    return QString();
}
