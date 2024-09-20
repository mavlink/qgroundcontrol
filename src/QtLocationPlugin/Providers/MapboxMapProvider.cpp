/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapboxMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

QString MapboxMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString mapBoxToken = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxToken()->rawValue().toString();
    if (!mapBoxToken.isEmpty()) {
        if (_mapTypeId == QStringLiteral("mapbox.custom")) {
            static const QString MapBoxUrlCustom = QStringLiteral("https://api.mapbox.com/styles/v1/%1/%2/tiles/256/%3/%4/%5?access_token=%6");
            const QString mapBoxAccount = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxAccount()->rawValue().toString();
            const QString mapBoxStyle = qgcApp()->toolbox()->settingsManager()->appSettings()->mapboxStyle()->rawValue().toString();
            return MapBoxUrlCustom.arg(mapBoxAccount).arg(mapBoxStyle).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
        }

        static const QString MapBoxUrl = QStringLiteral("https://api.mapbox.com/styles/v1/mapbox/%1/tiles/%2/%3/%4?access_token=%5");
        return MapBoxUrl.arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(mapBoxToken);
    }
    return QString();
}
