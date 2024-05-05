/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TerrainQueryTest.h"
#include "TerrainTileManager.h"

const QGeoCoordinate UnitTestTerrainQuery::pointNemo{-48.875556, -123.392500};
const UnitTestTerrainQuery::Flat10Region UnitTestTerrainQuery::flat10Region{{
      pointNemo,
      QGeoCoordinate{
          pointNemo.latitude() - UnitTestTerrainQuery::regionSizeDeg,
          pointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg
      }
}};
const double UnitTestTerrainQuery::Flat10Region::amslElevation = 10;

const UnitTestTerrainQuery::LinearSlopeRegion UnitTestTerrainQuery::linearSlopeRegion{{
    flat10Region.topRight(),
    QGeoCoordinate{
        flat10Region.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
        flat10Region.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg
    }
}};
const double UnitTestTerrainQuery::LinearSlopeRegion::minAMSLElevation  = -100;
const double UnitTestTerrainQuery::LinearSlopeRegion::maxAMSLElevation  = 1000;
const double UnitTestTerrainQuery::LinearSlopeRegion::totalElevationChange     = maxAMSLElevation - minAMSLElevation;

const UnitTestTerrainQuery::HillRegion UnitTestTerrainQuery::hillRegion{{
    linearSlopeRegion.topRight(),
    QGeoCoordinate{
        linearSlopeRegion.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
        linearSlopeRegion.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg
    }
}};
const double UnitTestTerrainQuery::HillRegion::radius = UnitTestTerrainQuery::regionSizeDeg / UnitTestTerrainQuery::one_second_deg;

UnitTestTerrainQuery::UnitTestTerrainQuery(TerrainQueryInterface* parent)
    : TerrainQueryInterface(parent)
{

}

void UnitTestTerrainQuery::requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates) {
    QList<double> result = _requestCoordinateHeights(coordinates);
    emit qobject_cast<TerrainQueryInterface*>(parent())->coordinateHeightsReceived(result.size() == coordinates.size(), result);
}

void UnitTestTerrainQuery::requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord) {
    auto pathHeightInfo = _requestPathHeights(fromCoord, toCoord);
    emit qobject_cast<TerrainQueryInterface*>(parent())->pathHeightsReceived(
        pathHeightInfo.rgHeights.count() > 0,
        pathHeightInfo.distanceBetween,
        pathHeightInfo.finalDistanceBetween,
        pathHeightInfo.rgHeights
    );
}

void UnitTestTerrainQuery::requestCarpetHeights(const QGeoCoordinate& swCoord, const QGeoCoordinate& neCoord, bool) {
    QList<QList<double>> carpet;

    if (swCoord.longitude() > neCoord.longitude() || swCoord.latitude() > neCoord.latitude()) {
        qCWarning(TerrainQueryLog) << "UnitTestTerrainQuery::requestCarpetHeights: Internal Error - bad carpet coords";
        emit qobject_cast<TerrainQueryInterface*>(parent())->carpetHeightsReceived(false, qQNaN(), qQNaN(), carpet);
        return;
    }

    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::min();
    for (double lat = swCoord.latitude(); lat < neCoord.latitude(); lat++) {
        QGeoCoordinate fromCoord(lat, swCoord.longitude());
        QGeoCoordinate toCoord  (lat, neCoord.longitude());

        QList<double> row = _requestPathHeights(fromCoord, toCoord).rgHeights;
        if (row.size() == 0) {
            emit carpetHeightsReceived(false, qQNaN(), qQNaN(), QList<QList<double>>());
            return;
        }
        for (const auto val : row) {
            min = qMin(val, min);
            max = qMax(val, max);
        }
        carpet.append(row);
    }
    emit qobject_cast<TerrainQueryInterface*>(parent())->carpetHeightsReceived(true, min, max, carpet);
}

UnitTestTerrainQuery::PathHeightInfo_t UnitTestTerrainQuery::_requestPathHeights(const QGeoCoordinate& fromCoord, const QGeoCoordinate& toCoord)
{
    PathHeightInfo_t   pathHeights;

    pathHeights.rgCoords    = TerrainTileManager::pathQueryToCoords(fromCoord, toCoord, pathHeights.distanceBetween, pathHeights.finalDistanceBetween);
    pathHeights.rgHeights   = _requestCoordinateHeights(pathHeights.rgCoords);

    return pathHeights;
}

QList<double> UnitTestTerrainQuery::_requestCoordinateHeights(const QList<QGeoCoordinate>& coordinates)
{
    QList<double> result;

    for (const auto& coordinate : coordinates) {
        if (flat10Region.contains(coordinate)) {
            result.append(UnitTestTerrainQuery::Flat10Region::amslElevation);
        } else if (linearSlopeRegion.contains(coordinate)) {
            //cast to one_second_deg grid and round to int to emulate SRTM1 even better
            long x = (coordinate.longitude() - linearSlopeRegion.topLeft().longitude())/one_second_deg;
            long dx = regionSizeDeg/one_second_deg;
            double fraction = 1.0 * x / dx;
            result.append(std::round(UnitTestTerrainQuery::LinearSlopeRegion::minAMSLElevation + (fraction * UnitTestTerrainQuery::LinearSlopeRegion::totalElevationChange)));
        } else if (hillRegion.contains(coordinate)) {
            double arc_second_meters = (earths_radius_mts * one_second_deg) * (M_PI / 180);
            double x = (coordinate.latitude() - hillRegion.center().latitude()) * arc_second_meters / one_second_deg;
            double y = (coordinate.longitude() - hillRegion.center().longitude()) * arc_second_meters / one_second_deg;
            double x2y2 = pow(x, 2) + pow(y, 2);
            double r2 = pow(UnitTestTerrainQuery::HillRegion::radius, 2);
            double z;
            if (x2y2 <= r2) {
                z = sqrt(r2 - x2y2);
            } else {
                z = UnitTestTerrainQuery::Flat10Region::amslElevation;
            }
            result.append(z);
        } else {
            result.clear();
            break;
        }
    }

    return result;
}
