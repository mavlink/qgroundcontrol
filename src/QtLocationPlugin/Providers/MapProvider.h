#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include "QGCTileSet.h"

class AppSettings;
class Fact;

static constexpr int QGC_MAX_MAP_ZOOM = 23;
// Web Mercator is only defined to ±85.05112878°; tan/cos blow up at the poles.
static constexpr double QGC_MAX_MERCATOR_LATITUDE = 85.05112878;
static constexpr quint32 QGC_AVERAGE_TILE_SIZE = 13652;

// TODO: Inherit from QGeoMapType
class MapProvider
{
public:
    // Mirror of QGeoMapType::MapStyle (kept in sync manually so this header does
    // not pull QtLocation/private/qgeomaptype_p.h). Drift is caught at compile
    // time via static_assert in MapProvider.cpp; values are converted at the
    // QGeoMapType boundary in QGeoTiledMappingManagerEngineQGC.cpp.
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

    int getMapId() const { return _mapId; }

    const QString& getReferrer() const { return _referrer; }

    virtual QByteArray getToken() const { return QByteArray(); }

    virtual int long2tileX(double lon, int z) const;
    virtual int lat2tileY(double lat, int z) const;
    virtual double tileX2long(int x, int z) const;
    virtual double tileY2lat(int y, int z) const;

    // OSM tile-usage policy requires an app-identifiable User-Agent.
    virtual bool isOSMProvider() const { return false; }

    static bool isValidLongitude(double lon);
    static bool isValidLatitude(double lat);
    static bool isValidZoom(int zoom);

    // Virtual so providers with non-web-mercator tiling (e.g. the Copernicus
    // degree grid) can replace the default 2^zoom bound.
    virtual bool isValidTileCoordinate(int x, int y, int zoom) const;

    virtual QGCTileSet getTileCount(int zoom, double topleftLon, double topleftLat, double bottomRightLon,
                                    double bottomRightLat) const;

protected:
    // Centralizes the SettingsManager -> AppSettings -> Fact -> rawValue chain;
    // factString() returns QString() when the Fact is null.
    static AppSettings* appSettings();
    static QString factString(const Fact* fact);

    QString _tileXYToQuadKey(int tileX, int tileY, int levelOfDetail) const;
    int _getServerNum(int x, int y, int max) const;

    virtual QString _getURL(int x, int y, int zoom) const = 0;

    const QString _mapName;
    const QString _referrer;
    const QString _imageFormat;
    const quint32 _averageSize;
    const MapStyle _mapStyle;
    const QString _language;
    const int _mapId;

private:
    static int _mapIdIndex;
};
