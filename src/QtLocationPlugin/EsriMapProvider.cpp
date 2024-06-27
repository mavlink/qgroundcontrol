/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EsriMapProvider.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

#include <QtNetwork/QNetworkRequest>

QNetworkRequest EsriMapProvider::getTileURL(int x, int y, int zoom) const
{
    QNetworkRequest request;
    const QString url = _getURL(x, y, zoom);
    if (url.isEmpty()) {
        return request;
    }
    request.setUrl(QUrl(url));
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    const QByteArray token = qgcApp()->toolbox()->settingsManager()->appSettings()->esriToken()->rawValue().toString().toLatin1();
    request.setRawHeader(QByteArrayLiteral("User-Agent"), QByteArrayLiteral("Qt Location based application"));
    request.setRawHeader(QByteArrayLiteral("User-Token"), token);
    return request;
}

QString EsriMapProvider::_getURL(int x, int y, int zoom) const
{
    return _mapUrl.arg(_mapTypeId).arg(zoom).arg(y).arg(x);
}
