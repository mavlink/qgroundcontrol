#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_COPERNICUS_ELEV_SIZE = 2786;

class ElevationProvider : public MapProvider
{
protected:
    ElevationProvider(const QString &mapName, const QString &referrer, const QString &imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType)
        : MapProvider(
            mapName,
            referrer,
            imageFormat,
            averageSize,
            mapType) {}

public:
    bool isElevationProvider() const final { return true; }
};

class CopernicusElevationProvider : public ElevationProvider
{
public:
    CopernicusElevationProvider()
        : ElevationProvider(
            QStringLiteral("Copernicus Elevation"),
            QStringLiteral("https://terrain-ce.suite.auterion.com/"),
            QStringLiteral("bin"),
            AVERAGE_COPERNICUS_ELEV_SIZE,
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

    const QString _mapUrl = QStringLiteral("https://terrain-ce.suite.auterion.com/api/v1/carpet?points=%1,%2,%3,%4");
};
