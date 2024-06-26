#pragma once

#include "MapProvider.h"

static constexpr const quint32 AVERAGE_AIRMAP_ELEV_SIZE = 2786;

class ElevationProvider : public MapProvider
{
    Q_OBJECT

protected:
    ElevationProvider(const QString& imageFormat, quint32 averageSize,
                      QGeoMapType::MapStyle mapType, QObject* parent = nullptr)
        : MapProvider(QStringLiteral("https://terrain-ce.suite.auterion.com/"), imageFormat, averageSize, mapType, parent) {}

public:
    bool isElevationProvider() const final { return true; }
};

class CopernicusElevationProvider : public ElevationProvider
{
    Q_OBJECT

public:
    CopernicusElevationProvider(QObject* parent = nullptr)
        : ElevationProvider(QStringLiteral("bin"), AVERAGE_AIRMAP_ELEV_SIZE,
                            QGeoMapType::StreetMap, parent) {}

    int long2tileX(double lon, int z) const final;
    int lat2tileY(double lat, int z) const final;

    QGCTileSet getTileCount(int zoom, double topleftLon,
                            double topleftLat, double bottomRightLon,
                            double bottomRightLat) const final;

private:
    QString _getURL(int x, int y, int zoom) const final;
};
