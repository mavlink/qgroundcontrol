#pragma once

#include "MapProvider.h"

class ElevationProvider : public MapProvider
{
protected:
    ElevationProvider(const QString &mapName, const QString &referrer, const QString &imageFormat, quint32 averageSize,
                      MapProvider::MapStyle mapType)
        : MapProvider(
            mapName,
            referrer,
            imageFormat,
            averageSize,
            mapType) {}

public:
    bool isElevationProvider() const final { return true; }
    virtual QByteArray serialize(const QByteArray &image) const = 0;
};

/// \brief https://spacedata.copernicus.eu/collections/copernicus-digital-elevation-model
///
class CopernicusElevationProvider : public ElevationProvider
{
public:
    CopernicusElevationProvider()
        : ElevationProvider(
            kProviderKey,
            kProviderURL,
            QStringLiteral("bin"),
            kAvgElevSize,
            MapProvider::TerrainMap) {}

    int long2tileX(double lon, int z) const final;
    int lat2tileY(double lat, int z) const final;

    QGCTileSet getTileCount(int zoom, double topleftLon,
                            double topleftLat, double bottomRightLon,
                            double bottomRightLat) const final;

    QByteArray serialize(const QByteArray &image) const final;

    static constexpr const char *kProviderKey = "Copernicus";
    static constexpr const char *kProviderNotice = "© Airbus Defence and Space GmbH";
    static constexpr const char *kProviderURL = "https://terrain-ce.suite.auterion.com";
    static constexpr quint32 kAvgElevSize = 2786;

private:
    QString _getURL(int x, int y, int zoom) const final;

    const QString _mapUrl = QString(kProviderURL) + QStringLiteral("/api/v1/carpet?points=%1,%2,%3,%4");
};

/// \brief AWS Open Data Terrain Tiles (Mapzen/Tilezen terrarium encoding).
/// Standard slippy z/x/y PNG tiles, zooms 0-15; height = (R*256 + G + B/256) - 32768 meters.
/// GeoMap-only: not user selectable, so the shared terrain pipeline never sees it.
/// Attribution (required when surfaced in UI): Terrain Tiles by Mapzen/Tilezen —
/// SRTM (NASA), 3DEP (USGS), GMTED2010 (USGS), ETOPO1 (NOAA), and other sources.
/// https://registry.opendata.aws/terrain-tiles/
class TerrariumElevationProvider : public ElevationProvider
{
public:
    TerrariumElevationProvider()
        : ElevationProvider(kProviderKey, kProviderURL, QStringLiteral("png"), kAvgElevSize, MapProvider::TerrainMap)
    {}

    /// Tiles are stored as-is: the terrarium PNG is already the cache format
    QByteArray serialize(const QByteArray& image) const final { return image; }

    bool isUserSelectable() const final { return false; }

    static constexpr const char* kProviderKey = "Terrarium";
    static constexpr const char* kProviderURL = "https://s3.amazonaws.com/elevation-tiles-prod";
    static constexpr quint32 kAvgElevSize = 45000;

private:
    QString _getURL(int x, int y, int zoom) const final;
};
