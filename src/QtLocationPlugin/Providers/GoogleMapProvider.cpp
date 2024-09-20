/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "GoogleMapProvider.h"

void GoogleMapProvider::_getSecGoogleWords(int x, int y, QString& sec1, QString& sec2) const
{
    sec1 = QStringLiteral(""); // after &x=...
    sec2 = QStringLiteral(""); // after &zoom=...
    const int seclen = ((x * 3) + y) % 8;
    sec2 = _secGoogleWord.left(seclen);
    if ((y >= 10000) && (y < 100000)) {
        sec1 = QStringLiteral("&s=");
    }
}

QString GoogleMapProvider::_getURL(int x, int y, int zoom) const
{
    QString sec1;
    QString sec2;
    _getSecGoogleWords(x, y, sec1, sec2);
    return _mapUrl
        .arg(_getServerNum(x, y, 4))
        .arg(_versionRequest, _version, _language)
        .arg(x)
        .arg(sec1)
        .arg(y)
        .arg(zoom)
        .arg(sec2, _scale);
}
