#include "TerrainTest.h"

#include "TerrainTileManager.h"

/// Point Nemo is a point on Earth furthest from land
static const QGeoCoordinate pointNemo = QGeoCoordinate(-48.875556, -123.392500);

const UnitTestTerrainQuery::Flat10Region UnitTestTerrainQuery::flat10Region{
    {pointNemo, QGeoCoordinate{pointNemo.latitude() - UnitTestTerrainQuery::regionSizeDeg,
                               pointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg}}};

const UnitTestTerrainQuery::LinearSlopeRegion UnitTestTerrainQuery::linearSlopeRegion{
    {flat10Region.topRight(),
     QGeoCoordinate{flat10Region.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
                    flat10Region.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg}}};

const UnitTestTerrainQuery::HillRegion UnitTestTerrainQuery::hillRegion{
    {linearSlopeRegion.topRight(),
     QGeoCoordinate{linearSlopeRegion.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
                    linearSlopeRegion.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg}}};

UnitTestTerrainQuery::UnitTestTerrainQuery(QObject* parent) : TerrainQueryInterface(parent)
{
}

UnitTestTerrainQuery::~UnitTestTerrainQuery()
{
}

void UnitTestTerrainQuery::requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    const QList<double> result = _requestCoordinateHeights(coordinates);
    emit coordinateHeightsReceived(result.size() == coordinates.size(), result);
}

void UnitTestTerrainQuery::requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    const PathHeightInfo_t pathHeightInfo = _requestPathHeights(fromCoord, toCoord);
    emit pathHeightsReceived(!pathHeightInfo.rgHeights.isEmpty(), pathHeightInfo.distanceBetween,
                             pathHeightInfo.finalDistanceBetween, pathHeightInfo.rgHeights);
}

void UnitTestTerrainQuery::requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord,
                                                bool statsOnly)
{
    Q_UNUSED(statsOnly);
    QList<QList<double>> carpet;
    if ((swCoord.longitude() > neCoord.longitude()) || (swCoord.latitude() > neCoord.latitude())) {
        emit carpetHeightsReceived(false, qQNaN(), qQNaN(), carpet);
        return;
    }
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    for (double lat = swCoord.latitude(); lat < neCoord.latitude(); lat++) {
        const QGeoCoordinate fromCoord(lat, swCoord.longitude());
        const QGeoCoordinate toCoord(lat, neCoord.longitude());
        const QList<double> row = _requestPathHeights(fromCoord, toCoord).rgHeights;
        if (row.isEmpty()) {
            emit carpetHeightsReceived(false, qQNaN(), qQNaN(), QList<QList<double>>());
            return;
        }
        for (const double val : row) {
            min = qMin(val, min);
            max = qMax(val, max);
        }
        (void)carpet.append(row);
    }
    emit carpetHeightsReceived(true, min, max, carpet);
}

UnitTestTerrainQuery::PathHeightInfo_t UnitTestTerrainQuery::_requestPathHeights(const QGeoCoordinate& fromCoord,
                                                                                 const QGeoCoordinate& toCoord)
{
    PathHeightInfo_t pathHeights;
    pathHeights.rgCoords = TerrainTileManager::_pathQueryToCoords(fromCoord, toCoord, pathHeights.distanceBetween,
                                                                  pathHeights.finalDistanceBetween);
    pathHeights.rgHeights = _requestCoordinateHeights(pathHeights.rgCoords);
    return pathHeights;
}

QList<double> UnitTestTerrainQuery::_requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    QList<double> result;
    for (const auto& coordinate : coordinates) {
        if (flat10Region.contains(coordinate)) {
            (void)result.append(UnitTestTerrainQuery::Flat10Region::amslElevation);
        } else if (linearSlopeRegion.contains(coordinate)) {
            // cast to oneSecondDeg grid and round to int to emulate SRTM1 even better
            const double x = (coordinate.longitude() - linearSlopeRegion.topLeft().longitude()) / oneSecondDeg;
            const double dx = regionSizeDeg / oneSecondDeg;
            const double fraction = x / dx;
            (void)result.append(std::round(UnitTestTerrainQuery::LinearSlopeRegion::minAMSLElevation +
                                           (fraction * UnitTestTerrainQuery::LinearSlopeRegion::totalElevationChange)));
        } else if (hillRegion.contains(coordinate)) {
            const double arcSecondMeters = (earthsRadiusMts * oneSecondDeg) * (M_PI / 180.);
            const double x = (coordinate.latitude() - hillRegion.center().latitude()) * arcSecondMeters / oneSecondDeg;
            const double y =
                (coordinate.longitude() - hillRegion.center().longitude()) * arcSecondMeters / oneSecondDeg;
            const double x2y2 = pow(x, 2) + pow(y, 2);
            const double r2 = pow(UnitTestTerrainQuery::HillRegion::radius, 2);
            const double z = (x2y2 <= r2) ? sqrt(r2 - x2y2) : UnitTestTerrainQuery::Flat10Region::amslElevation;
            (void)result.append(z);
        } else {
            result.clear();
            break;
        }
    }
    return result;
}

/*===========================================================================*/

TerrainTest::TerrainTest(QObject* parent) : UnitTest(parent)
{
}

void TerrainTest::init()
{
    UnitTest::init();
    _terrainQuery = new UnitTestTerrainQuery(this);
}

void TerrainTest::cleanup()
{
    delete _terrainQuery;
    _terrainQuery = nullptr;
    UnitTest::cleanup();
}
