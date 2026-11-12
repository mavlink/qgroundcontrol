#include "EsriMapProvider.h"

#include "AppSettings.h"
#include "SettingsManager.h"

QByteArray EsriMapProvider::getToken() const
{
    return SettingsManager::instance()->appSettings()->esriToken()->rawValue().toString().toUtf8();
}

QString EsriMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
}
