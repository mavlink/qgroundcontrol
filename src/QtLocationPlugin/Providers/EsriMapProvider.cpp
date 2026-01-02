#include "EsriMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QByteArray EsriMapProvider::getToken() const
{
    return SettingsManager::instance()->appSettings()->esriToken()->rawValue().toString().toUtf8();
}

QString EsriMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
}
