#include "GoogleMapProvider.h"
#include "QGCApplication.h"

QString GoogleMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString result = QString(m_mapUrl).arg(m_version).arg(qgcApp()->getCurrentLanguage().uiLanguages().first()).arg(x).arg(y).arg(zoom);
    return result;
}