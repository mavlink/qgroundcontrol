#pragma once

#include "MapProvider.h"

class ElevationProvider : public MapProvider
{
protected:
    ElevationProvider(const QString& mapName, const QString& referrer, const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapStyle)
        : MapProvider(mapName, referrer, imageFormat, averageSize, mapStyle) {}

public:
    bool isElevationProvider() const final { return true; }
};

class CopernicusElevationProvider : public ElevationProvider
{
public:
    CopernicusElevationProvider()
        : ElevationProvider(
            "Copernicus Elevation",
            QStringLiteral("https://terrain-ce.suite.auterion.com/"),
            QStringLiteral("bin"),
            2786,
            QGeoMapType::StreetMap) {}

    int long2tileX(double lon, int z) const final;
    int lat2tileY(double lat, int z) const final;

    QGCTileSet getTileCount(int zoom, double topleftLon,
                            double topleftLat, double bottomRightLon,
                            double bottomRightLat) const final;

    static constexpr const char* kProviderKey = "Copernicus Elevation";
    static constexpr const char* kProviderNotice = "© Airbus Defence and Space GmbH";

private:
    QString _getURL(int x, int y, int zoom) const final;
};
