#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QUrl>

#include "TileRegion.h"

class Fact;

inline constexpr int QGC_MAX_MAP_ZOOM = 23;
static constexpr const quint32 QGC_AVERAGE_TILE_SIZE = 13652;

class MapProvider
{
public:
    // Mirrors QGeoMapType::MapStyle so that this class does not need QtLocation.
    // QGCGeoTiledMappingManagerEngine.cc converts the values and asserts that they match.
    enum MapStyle
    {
        NoMap = 0,
        StreetMap,
        SatelliteMapDay,
        SatelliteMapNight,
        TerrainMap,
        HybridMap,
        TransitMap,
        GrayStreetMap,
        PedestrianMap,
        CarNavigationMap,
        CycleMap,
        CustomMap = 100
    };

    /// \a mapId is the permanent id of a provider of MapProviderRegistry, which MapProviderTable.cc gives out
    MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                quint32 averageSize = QGC_AVERAGE_TILE_SIZE, MapStyle mapStyle = CustomMap, int mapId = 0);
    virtual ~MapProvider();

    Q_DISABLE_COPY_MOVE(MapProvider)

    QUrl getTileURL(int x, int y, int zoom) const;

    /// The format of \a image by its signature, or else the format of the provider's tiles. Empty for fewer than 3
    /// bytes.
    QString getImageFormat(QByteArrayView image) const;

    /// The format of \a image by its signature: "png", "jpg", "gif" or "webp". Empty for what is none of these
    /// pictures.
    static QString imageFormatFromSignature(QByteArrayView image);

    quint32 averageSize() const { return _averageSize; }

    MapStyle mapStyle() const { return _mapStyle; }

    /// True for a map in dark colors, made to be read at night: what is drawn on it is light, as on satellite pictures
    virtual bool isNight() const { return false; }

    const QString& mapName() const { return _mapName; }

    /// Permanent id of the provider. 0 for a provider that is not one of MapProviderRegistry.
    int mapId() const { return _mapId; }

    /// Sent as the Referer of a tile request, unless it is empty
    const QString& referrer() const { return _referrer; }

    /// True for a provider whose terms ask a request to name the application that sends it: its tile requests carry
    /// the user agent of the application, not that of a browser
    virtual bool requiresApplicationUserAgent() const { return false; }

    virtual QByteArray token() const { return QByteArray(); }

    /// True for an image that the provider's servers send in place of a tile when they refuse a request. It is not
    /// a tile: one that is fetched is not cached, and one that is in the cache is fetched again.
    virtual bool isRefusalImage(QByteArrayView image) const
    {
        Q_UNUSED(image);
        return false;
    }

    /// True for an image that the provider's servers send, with the HTTP status of a tile, where the provider has no
    /// tile. It is not a tile: one that is fetched is not cached, and one that is in the cache is fetched again.
    virtual bool isNoTileImage(QByteArrayView image) const
    {
        Q_UNUSED(image);
        return false;
    }

    /// The deepest zoom level of which the provider has tiles anywhere: its server answers a request for a deeper tile
    /// with an error, or has none. QGC_MAX_MAP_ZOOM for a provider whose depth is not known.
    virtual int maxZoom() const { return QGC_MAX_MAP_ZOOM; }

    /// The credit that the provider's terms ask for where its tiles are shown. Empty for a source the user sets.
    virtual QString attribution() const { return QString(); }

    /// The address of the provider's list of who is credited beside attribution() for which area and zoom level of
    /// its tiles, in the form that EsriAttribution reads. Empty for a provider that publishes no such list.
    virtual QUrl areaAttributionUrl() const { return QUrl(); }

    /// Names the servers that the provider asks for its tiles: the providers with one name ask the same servers, and
    /// wait together when those ask for a pause (TileServerPauses). The name of the provider when it has servers
    /// of its own.
    virtual QString service() const { return _mapName; }

    /// The settings with which the user configures the source the provider fetches from. Empty for a provider with
    /// a fixed source. Main thread only, as their values are.
    virtual QList<const Fact*> sourceSettings() const { return {}; }

    /// The setting of the language of QGC, for a provider whose tiles have labels in the language that a request
    /// names: a QLocale::Language, QLocale::AnyLanguage for the language of the system. Null for any other
    /// provider. Main thread only, as its value is.
    virtual const Fact* labelLanguageSetting() const { return nullptr; }

    /// The settings that cacheVariant() reads: after a change of one, the provider fetches other tiles
    QList<const Fact*> cacheVariantSettings() const;

    /// Identifies the tiles the provider fetches: the values of sourceSettings() and, for labels in a language other
    /// than that of the system, the language, joined with '/'. Such tiles have keys of their own in the tile cache:
    /// see getTileHash(). Empty for a provider with a fixed source that has no labels, or has them in the language of
    /// the system. Reads settings, so call it on the main thread only.
    QString cacheVariant() const;

    /// Key of a tile in the tile cache: provider id, x, y and zoom as "%010d%08d%08d%03d". For a provider with a
    /// cache variant, cacheVariantSuffix() follows, so that the tiles of every source the user configures, and of
    /// every language of their labels, have keys of their own. Takes the variant from cacheVariant(): call it on the
    /// main thread only.
    QString getTileHash(int x, int y, int z) const;
    /// The key for \a cacheVariant, which the caller got on the main thread. Callable on any thread.
    QString getTileHash(int x, int y, int z, QStringView cacheVariant) const;

    /// What the key of a tile ends with for \a cacheVariant: '-' and the first 8 hex digits of the SHA-1 of the
    /// variant. Empty for an empty variant.
    static QString cacheVariantSuffix(QStringView cacheVariant);

    /// The first column, or the one after the last, for a longitude beyond the west or the east edge of the map
    virtual int long2tileX(double lon, int z) const;
    /// The first or the last row for a latitude beyond the north or the south edge of the map
    virtual int lat2tileY(double lat, int z) const;
    virtual double tileX2long(int x, int z) const;
    virtual double tileY2lat(int y, int z) const;

    virtual bool isElevationProvider() const { return false; }

    /// False for a map of tiles that the user imported into the tile cache: no server has them, and a tile that the
    /// cache has not is one that the map has not
    virtual bool hasServer() const { return true; }

    /// False hides the provider from user-facing provider selection lists
    virtual bool isUserSelectable() const { return true; }

    /// Number of tile columns around the world at \a zoom, which is limited as for getTileCount()
    int tileColumns(int zoom) const;

    /// The tiles of the region between the two corners at \a zoom, which is limited to 1..QGC_MAX_MAP_ZOOM, with
    /// their count and estimated size. The columns of a region whose left edge is east of its right one, which
    /// crosses the antimeridian, run on past the last column of the world: column x of the region is the tile column
    /// x % tileColumns(zoom). No tiles for a region with a corner that is not a finite number.
    virtual TileRegion getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

protected:
    /// The tiles of the columns \a x0 to \a x1 and of the rows \a y0 to \a y1, with their count and estimated size.
    /// No tiles for a first column or row that is beyond the last one.
    TileRegion _tileRegion(int x0, int y0, int x1, int y1) const;

    /// True for an \a image of \a size bytes whose SHA-256 is \a sha256Hex, in lower-case hex digits. Tells an image
    /// that is not kept in the sources. The size spares hashing other images.
    static bool _hasSha256(QByteArrayView image, qsizetype size, QByteArrayView sha256Hex);

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    /// The language that a request for a tile names, as the tag of a language and a territory ("de-DE"): that of the
    /// system, unless the tiles have labels and QGC is set to another language. Main thread only.
    QString _labelLanguage() const;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const MapStyle _mapStyle;

private:
    /// The language of QGC as _labelLanguage() names it, where the tiles have labels and it is not the language of
    /// the system. Empty otherwise.
    QString _labelLanguageOverride() const;

    const int _mapId;
};
