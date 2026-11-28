#include "MapProvider.h"

#include <algorithm>
#include <cmath>

#include <QtCore/QCryptographicHash>
#include <QtCore/QLocale>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QtMinMax>

#include "Fact.h"
#include "TileRegion.h"
#include "WebMercator.h"

namespace {

/// The language of \a locale as the tag of a language and a territory, as "de-DE"
QString mapProviderLanguageTag(const QLocale& locale)
{
    return locale.name().replace(QLatin1Char('_'), QLatin1Char('-'));
}

/// The language of the user interface of the system. Not as QLocale::uiLanguages() has it first, with the script
/// ("de-Latn-DE"): Bing answers a request that names a script with labels in English.
const QString& mapProviderSystemLanguage()
{
    static const QString language = [] {
        const QStringList languages = QLocale::system().uiLanguages();
        return mapProviderLanguageTag(languages.isEmpty() ? QLocale(QLocale::English)
                                                          : QLocale(languages.constFirst()));
    }();
    return language;
}

}  // namespace

MapProvider::MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                         quint32 averageSize, MapStyle mapStyle, int mapId)
    : _mapName(mapName)
    , _referrer(referrer)
    , _imageFormat(imageFormat)
    , _averageSize(averageSize)
    , _mapStyle(mapStyle)
    , _mapId(mapId)
{}

MapProvider::~MapProvider() {}

QList<const Fact*> MapProvider::cacheVariantSettings() const
{
    QList<const Fact*> settings = sourceSettings();
    if (const Fact* const languageSetting = labelLanguageSetting()) {
        settings.append(languageSetting);
    }
    return settings;
}

QString MapProvider::cacheVariant() const
{
    QStringList values;
    for (const Fact* const setting : sourceSettings()) {
        values.append(setting->rawValue().toString());
    }
    const QString variant = values.join(QLatin1Char('/'));
    const QString language = _labelLanguageOverride();
    if (language.isEmpty()) {
        return variant;
    }
    return variant.isEmpty() ? language : (variant + QLatin1Char('/') + language);
}

QString MapProvider::_labelLanguageOverride() const
{
    const Fact* const languageSetting = labelLanguageSetting();
    if (!languageSetting) {
        return QString();
    }

    // The tiles with labels in the language of the system keep the keys that such tiles always had in the tile cache
    const QLocale::Language language = static_cast<QLocale::Language>(languageSetting->rawValue().toInt());
    if ((language == QLocale::AnyLanguage) || (language == QLocale(mapProviderSystemLanguage()).language())) {
        return QString();
    }

    return mapProviderLanguageTag(QLocale(language));
}

QString MapProvider::_labelLanguage() const
{
    const QString language = _labelLanguageOverride();
    return language.isEmpty() ? mapProviderSystemLanguage() : language;
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

int MapProvider::long2tileX(double lon, int z) const
{
    // Bounded as the rows are, to the column of the east edge: that is the one after the last, which tileColumns()
    // counts the columns by
    const double columns = std::exp2(z);
    return static_cast<int>(std::fmin(std::fmax(std::floor(WebMercator::xOfLongitude(lon) * columns), 0.0), columns));
}

int MapProvider::lat2tileY(double lat, int z) const
{
    // Bounded to the rows of the map before the cast: a latitude beyond its edges gives a row beyond them, the south
    // pole an infinite one, and the cast of what int cannot hold is undefined. With fmax() and fmin(), which give
    // the bound for what is not a number.
    const double lastRow = std::exp2(z) - 1.0;
    return static_cast<int>(
        std::fmin(std::fmax(std::floor(WebMercator::yOfLatitude(lat) * std::exp2(z)), 0.0), lastRow));
}

double MapProvider::tileX2long(int x, int z) const
{
    return WebMercator::longitudeOfX(x / std::exp2(z));
}

double MapProvider::tileY2lat(int y, int z) const
{
    return WebMercator::latitudeOfY(y / std::exp2(z));
}

int MapProvider::tileColumns(int zoom) const
{
    return long2tileX(180.0, std::clamp(zoom, 1, QGC_MAX_MAP_ZOOM));
}

TileRegion MapProvider::getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                     double bottomRightLat) const
{
    // A map view gives the corner of a view beyond the map as an invalid coordinate, which is NaN
    if (!std::isfinite(topleftLon) || !std::isfinite(topleftLat) || !std::isfinite(bottomRightLon) ||
        !std::isfinite(bottomRightLat)) {
        return TileRegion();
    }

    zoom = std::clamp(zoom, 1, QGC_MAX_MAP_ZOOM);

    const int leftX = long2tileX(topleftLon, zoom);
    int rightX = long2tileX(bottomRightLon, zoom);
    if (rightX < leftX) {
        rightX += tileColumns(zoom);
    }

    // Rows count southwards for the tiles of a map and northwards for those of Copernicus
    const int cornerY0 = lat2tileY(topleftLat, zoom);
    const int cornerY1 = lat2tileY(bottomRightLat, zoom);
    return _tileRegion(leftX, std::min(cornerY0, cornerY1), rightX, std::max(cornerY0, cornerY1));
}

TileRegion MapProvider::_tileRegion(int x0, int y0, int x1, int y1) const
{
    TileRegion region;
    region.tileX0 = x0;
    region.tileY0 = y0;
    region.tileX1 = x1;
    region.tileY1 = y1;
    if ((x1 >= x0) && (y1 >= y0)) {
        region.tileCount = (static_cast<quint64>(x1 - x0) + 1) * (static_cast<quint64>(y1 - y0) + 1);
    }
    region.tileSize = averageSize() * region.tileCount;
    return region;
}
