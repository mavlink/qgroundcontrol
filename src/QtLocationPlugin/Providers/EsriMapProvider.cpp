#include "EsriMapProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QByteArray EsriMapProvider::getToken() const
{
    const QByteArray token = getSettingsToken("esri", "Esri").toUtf8();
    return token;
}

QString EsriMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString url = _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
    return url;
}
