#include "BingMapProvider.h"

QString BingMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return _mapUrl.arg(_getServerNum(x, y, kServerCount)).arg(_mapTypeId, key, _imageFormat, _versionBingMaps, _language);
}
