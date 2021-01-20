#include "MapboxMapProvider.h"
#include "QGCApplication.h"
#include "QGCMapEngine.h"
#include "SettingsManager.h"

static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5");
static const QString MapBoxUrlCustom = QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6");

MapboxMapProvider::MapboxMapProvider(const QString &mapName, const quint32 averageSize, const QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QStringLiteral("https://www.mapbox.com/"), QStringLiteral("jpg"), averageSize, mapType, parent)
    , _mapboxName(mapName)
{
}

QString MapboxMapProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    const QString mapBoxToken = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString();
    if (!mapBoxToken.isEmpty()) {
        if (_mapboxName == QString("mapbox.custom")) {
            const QString mapBoxAccount = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxAccount()->rawValue().toString();
            const QString mapBoxStyle = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxStyle()->rawValue().toString();
            return MapBoxUrlCustom.arg(mapBoxAccount).arg(mapBoxStyle).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        }
        return MapBoxUrl.arg(_mapboxName).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
    }
    return QString();
}
