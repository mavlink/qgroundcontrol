#include "BingMapProvider.h"

QString BingMapProvider::_getURL(int x, int y, int zoom) const
{
    const int serverNum = _getServerNum(x, y, kServerCount);
    const QString key = _tileXYToQuadKey(x, y, zoom);
    const QString url = _mapUrl.arg(serverNum).arg(_mapTypeId, key, _imageFormat, _versionBingMaps, _language);
    return url;
}
