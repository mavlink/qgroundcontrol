#include "MapboxMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QString MapboxMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString mapBoxToken = getSettingsToken("mapbox", "Mapbox");
    if (!mapBoxToken.isEmpty()) {
        if (_mapTypeId == QStringLiteral("mapbox.custom")) {
            static const QString MapBoxUrlCustom = QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6");
            AppSettings* appSettings = SettingsManager::instance()->appSettings();
            const QString mapBoxAccount = appSettings->mapboxAccount()->rawValue().toString();
            const QString mapBoxStyle = appSettings->mapboxStyle()->rawValue().toString();
            const QString url = MapBoxUrlCustom.arg(mapBoxAccount).arg(mapBoxStyle).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
            return url;
        }

        static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5");
        const QString url = MapBoxUrl.arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        return url;
    }
    return QString();
}
