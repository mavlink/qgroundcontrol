#include "MapboxMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

QString MapboxMapProvider::_getURL(int x, int y, int zoom) const
{
    QString result = QString();

    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    const QString mapBoxToken = appSettings->mapboxToken()->rawValue().toString();
    if (!mapBoxToken.isEmpty()) {
        if (m_mapboxName == QString("mapbox.custom")) {
            static const QString MapBoxUrlCustom = QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6");
            const QString mapBoxAccount = appSettings->mapboxAccount()->rawValue().toString();
            const QString mapBoxStyle = appSettings->mapboxStyle()->rawValue().toString();
            result = MapBoxUrlCustom.arg(mapBoxAccount, mapBoxStyle).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        } else {
            static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5");
            result = MapBoxUrl.arg(m_mapboxName).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        }
    }

    return result;
}
