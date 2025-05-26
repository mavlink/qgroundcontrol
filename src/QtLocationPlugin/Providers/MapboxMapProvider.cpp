/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapboxMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QString MapboxMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString mapBoxToken = SettingsManager::instance()->appSettings()->mapboxToken()->rawValue().toString();
    if (!mapBoxToken.isEmpty()) {
        if (_mapTypeId == QStringLiteral("mapbox.custom")) {
            static const QString MapBoxUrlCustom = QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6");
            const QString mapBoxAccount = SettingsManager::instance()->appSettings()->mapboxAccount()->rawValue().toString();
            const QString mapBoxStyle = SettingsManager::instance()->appSettings()->mapboxStyle()->rawValue().toString();
            return MapBoxUrlCustom.arg(mapBoxAccount).arg(mapBoxStyle).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        }

        static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5");
        return MapBoxUrl.arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
    }
    return QString();
}
