#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QUrl>

#include "QGCTileSet.h"

class Fact;

#define QGC_MAX_MAP_ZOOM 23
static constexpr const quint32 QGC_AVERAGE_TILE_SIZE = 13652;

class MapProvider
{
public:
    // Mirrors QGeoMapType::MapStyle so that this class does not need QtLocation.
    // QGeoTiledMappingManagerEngineQGC.cc converts the values and asserts that they match.
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

    /// \a mapId is the permanent id of a provider of UrlFactory, which MapProviderTable.cc gives out
    MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                quint32 averageSize = QGC_AVERAGE_TILE_SIZE, MapStyle mapStyle = CustomMap, int mapId = 0);
    virtual ~MapProvider();

    QUrl getTileURL(int x, int y, int zoom) const;

    /// The format of \a image by its signature, or else the format of the provider's tiles. Empty for fewer than 3
    /// bytes.
    QString getImageFormat(QByteArrayView image) const;

    /// The format of \a image by its signature: "png", "jpg", "gif" or "webp". Empty for what is none of these
    /// pictures.
    static QString imageFormatFromSignature(QByteArrayView image);

    quint32 getAverageSize() const { return _averageSize; }

    MapStyle getMapStyle() const { return _mapStyle; }

    const QString& getMapName() const { return _mapName; }

    /// Permanent id of the provider. 0 for a provider that is not one of UrlFactory.
    int getMapId() const { return _mapId; }

    /// Sent as the Referer of a tile request, unless it is empty
    const QString& getReferrer() const { return _referrer; }

    /// True for a provider whose terms ask a request to name the application that sends it: its tile requests carry
    /// the user agent of the application, not that of a browser
    virtual bool requiresApplicationUserAgent() const { return false; }

    virtual QByteArray getToken() const { return QByteArray(); }

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

    /// The credit that the provider's terms ask for where its tiles are shown. Empty for a source the user sets.
    virtual QString attribution() const { return QString(); }

    /// The address of the provider's list of who is credited beside attribution() for which area and zoom level of
    /// its tiles, in the form that EsriAttribution reads. Empty for a provider that publishes no such list.
    virtual QUrl areaAttributionUrl() const { return QUrl(); }

    /// The settings with which the user configures the source the provider fetches from. Empty for a provider with
    /// a fixed source. Main thread only, as their values are.
    virtual QList<Fact*> sourceSettings() const { return {}; }

    /// Identifies the source the provider fetches from: the values of sourceSettings(), joined with '/'. Tiles of
    /// different sources have different keys in the tile cache: see getTileHash(). Empty for a provider with a
    /// fixed source. Reads settings, so call it on the main thread only.
    QString cacheVariant() const;

    /// Key of a tile in the tile cache: provider id, x, y and zoom as "%010d%08d%08d%03d". For a provider with a
    /// cache variant, cacheVariantSuffix() follows, so that the tiles of every source the user configures have keys
    /// of their own. Takes the variant from cacheVariant(): call it on the main thread only.
    QString getTileHash(int x, int y, int z) const;
    /// The key for \a cacheVariant, which the caller got on the main thread. Callable on any thread.
    QString getTileHash(int x, int y, int z, QStringView cacheVariant) const;

    /// What the key of a tile ends with for \a cacheVariant: '-' and the first 8 hex digits of the SHA-1 of the
    /// variant. Empty for an empty variant.
    static QString cacheVariantSuffix(QStringView cacheVariant);

    virtual int long2tileX(double lon, int z) const;
    virtual int lat2tileY(double lat, int z) const;
    virtual double tileX2long(int x, int z) const;
    virtual double tileY2lat(int y, int z) const;

    virtual bool isElevationProvider() const { return false; }

    /// False hides the provider from user-facing provider selection lists
    virtual bool isUserSelectable() const { return true; }

    /// Number of tile columns around the world at \a zoom, which is limited as for getTileCount()
    int tileColumns(int zoom) const;

    /// The tiles of the region between the two corners at \a zoom, which is limited to 1..QGC_MAX_MAP_ZOOM, with
    /// their count and estimated size. The columns of a region whose left edge is east of its right one, which
    /// crosses the antimeridian, run on past the last column of the world: column x of the set is the tile column
    /// x % tileColumns(zoom).
    virtual QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

protected:
    /// The tiles of the columns \a x0 to \a x1 and of the rows \a y0 to \a y1, with their count and estimated size.
    /// No tiles for a first column or row that is beyond the last one.
    QGCTileSet _tileSet(int x0, int y0, int x1, int y1) const;

    QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const;
    int _getServerNum(int x, int y, int max) const;

    /// True for an \a image of \a size bytes whose SHA-256 is \a sha256Hex, in lower-case hex digits. Tells an image
    /// that is not kept in the sources. The size spares hashing other images.
    static bool _hasSha256(QByteArrayView image, qsizetype size, QByteArrayView sha256Hex);

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const MapStyle _mapStyle;
    const QString _language;

private:
    const int _mapId;
};
