#include "TianDiTuProvider.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#include <QtCore/QUrl>

QString TianDiTuProvider::_getURL(int x, int y, int zoom) const
{
    //"https://t%1.tianditu.gov.cn/DataServer?tk=%2&T=%3&x=%4&y=%5&l=%6"
    const QString tiandituToken =
        QString::fromUtf8(QUrl::toPercentEncoding(factString(appSettings()->tiandituToken())));
    if (!tiandituToken.isEmpty()) {
        return _mapUrl
            .arg(_getServerNum(x, y, kServerCount))
            .arg(tiandituToken)
            .arg(_mapType)
            .arg(x)
            .arg(y)
            .arg(zoom)
            ;
    }
    return QString();
}
