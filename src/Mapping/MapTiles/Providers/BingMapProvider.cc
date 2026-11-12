#include "BingMapProvider.h"

#include <QtCore/QFile>

bool BingMapProvider::isNoTileImage(QByteArrayView image) const
{
    static const QByteArray noTileImage = [] {
        QFile file(QStringLiteral(":/res/BingNoTileBytes.dat"));
        return file.open(QFile::ReadOnly) ? file.readAll() : QByteArray();
    }();

    return !noTileImage.isEmpty() && (image == noTileImage);
}

QString BingMapProvider::_getURL(int x, int y, int zoom) const
{
    const QString key = _tileXYToQuadKey(x, y, zoom);
    return _mapUrl.arg(_getServerNum(x, y, 4)).arg(_mapTypeId, key, _imageFormat, _versionBingMaps, _language);
}
