#include "MapProvider.h"

#include <QGCLoggingCategory.h>
#include <QtCore/QCryptographicHash>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUrl>

#include "Fact.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(MapProviderLog, "MapTiles.MapProvider")

MapProvider::MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                         quint32 averageSize, MapStyle mapStyle)
    : _mapName(mapName)
    , _referrer(referrer)
    , _imageFormat(imageFormat)
    , _averageSize(averageSize)
    , _mapStyle(mapStyle)
    , _language(!QLocale::system().uiLanguages().isEmpty() ? QLocale::system().uiLanguages().constFirst() : "en")
{
    // qCDebug(MapProviderLog) << Q_FUNC_INFO << this << _mapId;
}

MapProvider::~MapProvider()
{
    // qCDebug(MapProviderLog) << Q_FUNC_INFO << this << _mapId;
}

QString MapProvider::cacheVariant() const
{
    QStringList values;
    for (const Fact* const setting : sourceSettings()) {
        values.append(setting->rawValue().toString());
    }
    return values.join(QLatin1Char('/'));
}

QUrl MapProvider::getTileURL(int x, int y, int zoom) const
{
    return QUrl(_getURL(x, y, zoom));
}

QString MapProvider::getImageFormat(QByteArrayView image) const
{
    if (image.size() < 3) {
        return QString();
    }

    const QString format = imageFormatFromSignature(image);
    return format.isEmpty() ? _imageFormat : format;
}

QString MapProvider::imageFormatFromSignature(QByteArrayView image)
{
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

    // A RIFF file, whose form type follows the four bytes of its size
    static constexpr QByteArrayView riffSignature("RIFF");
    static constexpr QByteArrayView webpFormType("WEBP");
    if (image.startsWith(riffSignature) && (image.size() >= 12) && (image.sliced(8, 4) == webpFormType)) {
        return QStringLiteral("webp");
    }

    return QString();
}

bool MapProvider::_hasSha256(QByteArrayView image, qsizetype size, QByteArrayView sha256Hex)
{
    return (image.size() == size) && (QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex() == sha256Hex);
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
    return static_cast<int>(
        floor((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

double MapProvider::tileX2long(int x, int z) const
{
    return x / std::pow(2.0, z) * 360.0 - 180.0;
}

double MapProvider::tileY2lat(int y, int z) const
{
    const double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, z);
    return qRadiansToDegrees(std::atan(std::sinh(n)));
}

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(topleftLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(bottomRightLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) - static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) - static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}

// Resolution math: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Resolution_and_Scale
