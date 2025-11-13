#include "GenericMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QString CustomURLMapProvider::_getURL(int x, int y, int zoom) const
{
    AppSettings* appSettings = SettingsManager::instance()->appSettings();
    QString url = appSettings->customURL()->rawValue().toString();
    (void) url.replace("{x}", QString::number(x));
    (void) url.replace("{y}", QString::number(y));
    static const QRegularExpression zoomRegExp("\\{(z|zoom)\\}");
    (void) url.replace(zoomRegExp, QString::number(zoom));
    return url;
}

QString CyberJapanMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(_imageFormat);
    return url;
}

QString LINZBasemapMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString linzToken = getSettingsToken("linz", "LINZ Basemap");
    if (linzToken.isEmpty()) {
        return QString();
    }
    const QString url = _mapUrl.arg(zoom).arg(x).arg(y).arg(_imageFormat).arg(linzToken);
    return url;
}

QString OpenAIPMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString apiKey = getSettingsToken("openaip", "OpenAIP");

    const QString url = apiKey.isEmpty()
        ? _mapUrl.arg(zoom).arg(x).arg(y)
        : _mapUrl.arg(zoom).arg(x).arg(y) + QStringLiteral("?apiKey=%1").arg(apiKey);

    return url;
}

QString OpenStreetMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(zoom).arg(x).arg(y);
    return url;
}

QString StatkartMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
    return url;
}

QString EniroMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(zoom).arg(x).arg(((1 << zoom) - 1) - y).arg(_imageFormat);
    return url;
}

QString SvalbardMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(zoom).arg(y).arg(x);
    return url;
}

QString MapQuestMapProvider::_getURL(int x, int y, int zoom) const
{
    const int serverNum = _getServerNum(x, y, kServerCount);
    const QString url = _mapUrl.arg(serverNum).arg(_mapTypeId).arg(zoom).arg(x).arg(y).arg(_imageFormat);
    return url;
}

QString VWorldMapProvider::_getURL(int x, int y, int zoom) const
{
    if ((zoom < kMinZoom) || (zoom > kMaxZoom)) {
        return QString();
    }

    const int gap = zoom - kZoomOffset;
    const int zoom_factor = 1 << gap;

    const int x_min = kXMinBase * zoom_factor;
    const int x_max = (kXMaxBase * zoom_factor) + ((2 * gap) - 1);
    if ((x < x_min) || (x > x_max)) {
        return QString();
    }

    const int y_min = kYMinBase * zoom_factor;
    const int y_max = (kYMaxBase * zoom_factor) + ((2 * gap) - 1);
    if ((y < y_min) || (y > y_max)) {
        return QString();
    }

    const QString VWorldMapToken = getSettingsToken("vworld", "VWorld");
    const QString url = _mapUrl.arg(VWorldMapToken, _mapTypeId).arg(zoom).arg(y).arg(x).arg(_imageFormat);
    return url;
}
