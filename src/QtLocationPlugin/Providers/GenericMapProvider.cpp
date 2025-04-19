/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GenericMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QString CustomURLMapProvider::_getURL(int x, int y, int zoom) const
{
    QString url = SettingsManager::instance()->appSettings()->customURL()->rawValue().toString();
    (void) url.replace("{x}", QString::number(x));
    (void) url.replace("{y}", QString::number(y));
    static const QRegularExpression zoomRegExp("\\{(z|zoom)\\}");
    (void) url.replace(zoomRegExp, QString::number(zoom));
    return url;
}

QString CyberJapanMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_mapName).arg(zoom).arg(x).arg(y).arg(_imageFormat);
}

QString LINZBasemapMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(zoom).arg(x).arg(y).arg(_imageFormat);
}

QString StatkartMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(zoom).arg(y).arg(x);
}

QString EniroMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(zoom).arg(x).arg((1 << zoom) - 1 - y).arg(_imageFormat);
}

QString SvalbardMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(zoom).arg(y).arg(x);
}

QString MapQuestMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_getServerNum(x, y, 4)).arg(_mapName).arg(zoom).arg(x).arg(y).arg(_imageFormat);
}

QString VWorldMapProvider::_getURL(int x, int y, int zoom) const
{
    if ((zoom < 5) || (zoom > 19)) {
        return QString();
    }

    const int gap = zoom - 6;

    const int x_min = 53 * pow(2, gap);
    const int x_max = (55 * pow(2, gap)) + (2 * gap - 1);
    if ((x < x_min) || (x > x_max)) {
        return QString();
    }

    const int y_min = 22 * pow(2, gap);
    const int y_max = (26 * pow(2, gap)) + (2 * gap - 1);
    if ((y < y_min) || (y > y_max)) {
        return QString();
    }

    const QString VWorldMapToken = SettingsManager::instance()->appSettings()->vworldToken()->rawValue().toString();
    return _mapUrl.arg(VWorldMapToken, _mapName).arg(zoom).arg(y).arg(x).arg(_imageFormat);
}
