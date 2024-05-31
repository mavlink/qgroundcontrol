/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MapProvider.h"
#include <QGCLoggingCategory.h>

#include <QtCore/QLocale>
#include <QtNetwork/QNetworkRequest>

QGC_LOGGING_CATEGORY(MapProviderLog, "qgc.qtlocationplugin.mapprovider")

MapProvider::MapProvider(
    const QString &referrer,
    const QString &imageFormat,
    quint32 averageSize,
    QGeoMapType::MapStyle mapStyle,
    QObject* parent)
    : QObject(parent)
    , _referrer(referrer)
    , _imageFormat(imageFormat)
    , _averageSize(averageSize)
    , _mapStyle(mapStyle)
    , _language(!QLocale::system().uiLanguages().isEmpty() ? QLocale::system().uiLanguages().constFirst() : "en") {}

QNetworkRequest MapProvider::getTileURL(int x, int y, int zoom) const
{
    QNetworkRequest request;
    const QString url = _getURL(x, y, zoom);
    if (url.isEmpty()) {
        return request;
    }
    request.setUrl(QUrl(url));
    request.setRawHeader(QByteArrayLiteral("Accept"), QByteArrayLiteral("*/*"));
    request.setRawHeader(QByteArrayLiteral("Referrer"), _referrer.toUtf8());
    request.setRawHeader(QByteArrayLiteral("User-Agent"), QByteArrayLiteral("Qt Location based application"));
    return request;
}

QString MapProvider::getImageFormat(QByteArrayView image) const
{
    if (image.size() < 3) {
        return QString();
    }

    static constexpr QByteArrayView pngSignature("\x89\x50\x4E\x47\x0D\x0A\x1A\x0A");
    if (image.startsWith(pngSignature)) {
        return QStringLiteral("png");
    }

    static constexpr QByteArrayView jpegSignature("\xFF\xD8\xFF");
    if (image.startsWith(jpegSignature)) {
        return QStringLiteral("jpg");
    }

    static constexpr QByteArrayView gifSignature("\x47\x49\x46\x38");
    if (image.startsWith(gifSignature)) {
        return QStringLiteral("gif");
    }

    return _imageFormat;
}

QString MapProvider::_tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const
{
    QString quadKey;
    for (int i = levelOfDetail; i > 0; i--) {
        char digit = '0';
        const int mask = 1 << (i - 1);

        if ((tileX & mask) != 0) {
            digit++;
        }
        if ((tileY & mask) != 0) {
            digit += 2;
        }

        (void) quadKey.append(digit);
    }

    return quadKey;
}

int MapProvider::_getServerNum(int x, int y, int max) const
{
    return (x + 2 * y) % max;
}

int MapProvider::long2tileX(double lon, int z) const
{
    return static_cast<int>(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int MapProvider::lat2tileY(double lat, int z) const
{
    return static_cast<int>(floor((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon,
                                     double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(topleftLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(bottomRightLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}
