#include "MapboxMapProvider.h"
#include "QGCMapUrlEngine.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QUrl>

QString MapboxMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString mapBoxToken = QString::fromUtf8(QUrl::toPercentEncoding(factString(appSettings()->mapboxToken())));
    if (!mapBoxToken.isEmpty()) {
        if (_mapTypeId == QStringLiteral("mapbox.custom")) {
            const bool retina = UrlFactory::useRetinaTiles();
            const QString tileSize = retina ? QStringLiteral("512") : QStringLiteral("256");
            const QString retinaSuffix = retina ? QStringLiteral("@2x") : QString();
            const QString MapBoxUrlCustom =
                QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/%3/%4/%5/%6%7?access_token=%8");
            const QString mapBoxAccount = factString(appSettings()->mapboxAccount());
            const QString mapBoxStyle = factString(appSettings()->mapboxStyle());
            return MapBoxUrlCustom.arg(mapBoxAccount, mapBoxStyle, tileSize)
                .arg(zoom)
                .arg(x)
                .arg(y)
                .arg(retinaSuffix, mapBoxToken);
        }

        const QString retinaSuffix = UrlFactory::useRetinaTiles() ? QStringLiteral("@2x") : QString();
        const QString MapBoxUrl =
            QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4%5?access_token=%6");
        return MapBoxUrl.arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(retinaSuffix, mapBoxToken);
    }
    return QString();
}
