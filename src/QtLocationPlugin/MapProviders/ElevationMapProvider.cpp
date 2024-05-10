#include "ElevationMapProvider.h"
#include "TerrainTile.h"

/*
License for the COPERNICUS dataset hosted on https://terrain-ce.suite.auterion.com/:

© DLR e.V. 2010-2014 and © Airbus Defence and Space GmbH 2014-2018 provided under COPERNICUS
by the European Union and ESA; all rights reserved.

*/

int CopernicusElevationProvider::long2tileX(double lon, int z) const
{
    Q_UNUSED(z)
    const int result = static_cast<int>(floor((lon + 180.0) / TerrainTile::tileSizeDegrees));
    return result;
}

int CopernicusElevationProvider::lat2tileY(double lat, int z) const
{
    Q_UNUSED(z)
    const int result = static_cast<int>(floor((lat + 90.0) / TerrainTile::tileSizeDegrees));
    return result;
}

QString CopernicusElevationProvider::_getURL(int x, int y, int zoom) const
{
    Q_UNUSED(zoom)
    const QString result = QString("https://terrain-ce.suite.auterion.com/api/v1/carpet?points=%1,%2,%3,%4")
                        .arg(static_cast<double>(y) * TerrainTile::tileSizeDegrees - 90.0)
                        .arg(static_cast<double>(x) * TerrainTile::tileSizeDegrees - 180.0)
                        .arg(static_cast<double>(y + 1) * TerrainTile::tileSizeDegrees - 90.0)
                        .arg(static_cast<double>(x + 1) * TerrainTile::tileSizeDegrees - 180.0);
    return result;
}

QGCTileSet CopernicusElevationProvider::getTileCount(int zoom, double topleftLon,
                                                 double topleftLat, double bottomRightLon,
                                                 double bottomRightLat) const
{
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);
    set.tileCount = (static_cast<quint64>(set.tileX1) - static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) - static_cast<quint64>(set.tileY0) + 1);
    set.tileSize = getAverageSize() * set.tileCount;
    return set;
}
