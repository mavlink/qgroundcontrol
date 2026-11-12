#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include "QGCTileSet.h"

class Fact;

#define QGC_MAX_MAP_ZOOM 23
static constexpr const quint32 QGC_AVERAGE_TILE_SIZE = 13652;

class MapProvider
{
public:
    // Mirrors QGeoMapType::MapStyle so that this class does not need QtLocation.
    // QGeoTiledMappingManagerEngineQGC.cpp converts the values and asserts that they match.
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

    MapProvider(const QString& mapName, const QString& referrer, const QString& imageFormat,
                quint32 averageSize = QGC_AVERAGE_TILE_SIZE, MapStyle mapStyle = CustomMap);
    virtual ~MapProvider();

    QUrl getTileURL(int x, int y, int zoom) const;

    QString getImageFormat(QByteArrayView image) const;

    // TODO: Download Random Tile And Use That Size Instead?
    quint32 getAverageSize() const { return _averageSize; }

    MapStyle getMapStyle() const { return _mapStyle; }

    const QString& getMapName() const { return _mapName; }

    /// Permanent id of the provider, assigned by the provider table in QGCMapUrlEngine.cpp. 0 for a provider that is
    /// not in that table.
    int getMapId() const { return _mapId; }

    /// Sent as the Referer of a tile request, unless it is empty
    const QString& getReferrer() const { return _referrer; }

    /// True for a provider whose terms ask a request to name the application that sends it: its tile requests carry
    /// the user agent of the application, not that of a browser
    virtual bool requiresApplicationUserAgent() const { return false; }

    virtual QByteArray getToken() const { return QByteArray(); }

    /// The credit that the provider's terms ask for where its tiles are shown. Empty for a source the user sets.
    virtual QString attribution() const { return QString(); }

    /// The settings with which the user configures the source the provider fetches from. Empty for a provider with
    /// a fixed source. Main thread only, as their values are.
    virtual QList<Fact*> sourceSettings() const { return {}; }

    /// Identifies the source the provider fetches from: the values of sourceSettings(), joined with '/'. Tiles of
    /// different sources have different keys in the tile cache: see UrlFactory::getTileHash(). Empty for a provider
    /// with a fixed source. Reads settings, so call it on the main thread only.
    QString cacheVariant() const;

    virtual int long2tileX(double lon, int z) const;
    virtual int lat2tileY(double lat, int z) const;
    virtual double tileX2long(int x, int z) const;
    virtual double tileY2lat(int y, int z) const;

    virtual bool isElevationProvider() const { return false; }

    virtual bool isBingProvider() const { return false; }

    /// False hides the provider from user-facing provider selection lists
    virtual bool isUserSelectable() const { return true; }

    virtual QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

protected:
    QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const;
    int _getServerNum(int x, int y, int max) const;

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const MapStyle _mapStyle;
    const QString _language;

private:
    friend class UrlFactory;

    int _mapId = 0;
};
