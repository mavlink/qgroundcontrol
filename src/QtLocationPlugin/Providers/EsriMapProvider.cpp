#include "EsriMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QByteArray EsriMapProvider::getToken() const
{
    return factString(appSettings()->esriToken()).toUtf8();
}

QString EsriMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
}
