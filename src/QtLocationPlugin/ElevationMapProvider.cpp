#include "ElevationMapProvider.h"
#if defined(DEBUG_GOOGLE_MAPS)
#include <QFile>
#include <QStandardPaths>
#endif
#include "QGCMapEngine.h"
#include "TerrainTile.h"

/*
License for the COPERNICUS dataset hosted on https://terrain-ce.suite.auterion.com/:

© DLR e.V. 2010-2014 and © Airbus Defence and Space GmbH 2014-2018 provided under COPERNICUS
by the European Union and ESA; all rights reserved.

*/

ElevationProvider::ElevationProvider(const QString& imageFormat, quint32 averageSize, QGeoMapType::MapStyle mapType, QObject* parent)
    : MapProvider(QStringLiteral("https://terrain-ce.suite.auterion.com/"), imageFormat, averageSize, mapType, parent) {}

//-----------------------------------------------------------------------------
int CopernicusElevationProvider::long2tileX(const double lon, const int z) const {
    Q_UNUSED(z)
    return static_cast<int>(floor((lon + 180.0) / TerrainTile::tileSizeDegrees));
}

//-----------------------------------------------------------------------------
int CopernicusElevationProvider::lat2tileY(const double lat, const int z) const {
    Q_UNUSED(z)
    return static_cast<int>(floor((lat + 90.0) / TerrainTile::tileSizeDegrees));
}

QString CopernicusElevationProvider::_getURL(const int x, const int y, const int zoom, QNetworkAccessManager* networkManager) {
    Q_UNUSED(networkManager)
    Q_UNUSED(zoom)
    return QString("https://terrain-ce.suite.auterion.com/api/v1/carpet?points=%1,%2,%3,%4")
        .arg(static_cast<double>(y) * TerrainTile::tileSizeDegrees - 90.0)
        .arg(static_cast<double>(x) * TerrainTile::tileSizeDegrees - 180.0)
        .arg(static_cast<double>(y + 1) * TerrainTile::tileSizeDegrees - 90.0)
        .arg(static_cast<double>(x + 1) * TerrainTile::tileSizeDegrees - 180.0);
}

QGCTileSet CopernicusElevationProvider::getTileCount(const int zoom, const double topleftLon,
                                                 const double topleftLat, const double bottomRightLon,
                                                 const double bottomRightLat) const {
    QGCTileSet set;
    set.tileX0 = long2tileX(topleftLon, zoom);
    set.tileY0 = lat2tileY(bottomRightLat, zoom);
    set.tileX1 = long2tileX(bottomRightLon, zoom);
    set.tileY1 = lat2tileY(topleftLat, zoom);

    set.tileCount = (static_cast<quint64>(set.tileX1) -
                     static_cast<quint64>(set.tileX0) + 1) *
                    (static_cast<quint64>(set.tileY1) -
                     static_cast<quint64>(set.tileY0) + 1);

    set.tileSize = getAverageSize() * set.tileCount;

    return set;
}
