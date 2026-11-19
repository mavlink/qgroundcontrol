#include "MapProvider.h"

#include <QGCLoggingCategory.h>
#include <QtCore/QCryptographicHash>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QtMinMax>

#include "Fact.h"
#include "QGCTileSet.h"

QGC_LOGGING_CATEGORY(MapProviderLog, "MapTiles.MapProvider")

MapProvider::MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                         quint32 averageSize, MapStyle mapStyle, int mapId)
    : _mapName(mapName)
    , _referrer(referrer)
    , _imageFormat(imageFormat)
    , _averageSize(averageSize)
    , _mapStyle(mapStyle)
    , _language(!QLocale::system().uiLanguages().isEmpty() ? QLocale::system().uiLanguages().constFirst() : "en")
    , _mapId(mapId)
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

QString MapProvider::getTileHash(int x, int y, int z) const
{
    return getTileHash(x, y, z, cacheVariant());
}

QString MapProvider::getTileHash(int x, int y, int z, QStringView cacheVariant) const
{
    return QString::asprintf("%010d%08d%08d%03d", _mapId, x, y, z) + cacheVariantSuffix(cacheVariant);
}

QString MapProvider::cacheVariantSuffix(QStringView cacheVariant)
{
    if (cacheVariant.isEmpty()) {
        return QString();
    }

    // Not qHash(): the keys are stored, and its values differ between Qt versions and runs
    const QByteArray digest = QCryptographicHash::hash(cacheVariant.toUtf8(), QCryptographicHash::Sha1);
    return QLatin1Char('-') + QString::fromLatin1(digest.toHex().left(8));
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

int MapProvider::tileColumns(int zoom) const
{
    return long2tileX(180.0, qBound(1, zoom, QGC_MAX_MAP_ZOOM));
}

QGCTileSet MapProvider::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const
{
    zoom = qBound(1, zoom, QGC_MAX_MAP_ZOOM);

    const int leftX = long2tileX(topleftLon, zoom);
    int rightX = long2tileX(bottomRightLon, zoom);
    if (rightX < leftX) {
        rightX += tileColumns(zoom);
    }

    // Rows count southwards for the tiles of a map and northwards for those of Copernicus
    const int cornerY0 = lat2tileY(topleftLat, zoom);
    const int cornerY1 = lat2tileY(bottomRightLat, zoom);
    return _tileSet(leftX, qMin(cornerY0, cornerY1), rightX, qMax(cornerY0, cornerY1));
}

QGCTileSet MapProvider::_tileSet(int x0, int y0, int x1, int y1) const
{
    QGCTileSet set;
    set.tileX0 = x0;
    set.tileY0 = y0;
    set.tileX1 = x1;
    set.tileY1 = y1;
    if ((x1 >= x0) && (y1 >= y0)) {
        set.tileCount = (static_cast<quint64>(x1 - x0) + 1) * (static_cast<quint64>(y1 - y0) + 1);
    }
    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}

// Resolution math: https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames#Resolution_and_Scale
