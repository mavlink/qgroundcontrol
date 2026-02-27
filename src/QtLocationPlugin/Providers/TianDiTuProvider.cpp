#include "TianDiTuProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

QString TianDiTuProvider::_getURL(int x, int y, int zoom) const
{
    //"https://t%1.tianditu.gov.cn/DataServer?tk=%2&T=%3&x=%4&y=%5&l=%6"
    const QString tiandituToken = SettingsManager::instance()->appSettings()->tiandituToken()->rawValue().toString();
    if (!tiandituToken.isEmpty()) {
        return _mapUrl
            .arg(_getServerNum(x, y, 8))
            .arg(tiandituToken)
            .arg(_mapType)
            .arg(x)
            .arg(y)
            .arg(zoom)
            ;
    }
    return QString();
}
