#include "BingMapProvider.h"
#include "QGCApplication.h"

QString BingMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString serverNum = QString::number(_getServerNum(x, y, 4));
    const QString key = _tileXYToQuadKey(x, y, zoom);

    const QString result = m_mapUrl.arg(serverNum, key, m_versionBingMaps, qgcApp()->getCurrentLanguage().uiLanguages().first());
    return result;
}
