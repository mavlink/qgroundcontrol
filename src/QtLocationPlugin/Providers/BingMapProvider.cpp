#include "BingMapProvider.h"

QString BingMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return _mapUrl.arg(_getServerNum(x, y, 4)).arg(_mapTypeId, key, _imageFormat, _versionBingMaps, _language);
}
