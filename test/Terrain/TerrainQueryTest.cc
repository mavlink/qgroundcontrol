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
#include "TerrainQuery.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

/// Point Nemo is a point on Earth furthest from land
static const QGeoCoordinate pointNemo = QGeoCoordinate(-48.875556, -123.392500);

const UnitTestTerrainQuery::Flat10Region UnitTestTerrainQuery::flat10Region{{
    pointNemo,
    QGeoCoordinate{
        pointNemo.latitude() - UnitTestTerrainQuery::regionSizeDeg,
        pointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg
    }
}};

const UnitTestTerrainQuery::LinearSlopeRegion UnitTestTerrainQuery::linearSlopeRegion{{
    flat10Region.topRight(),
    QGeoCoordinate{
        flat10Region.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
        flat10Region.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg
    }
}};

const UnitTestTerrainQuery::HillRegion UnitTestTerrainQuery::hillRegion{{
    linearSlopeRegion.topRight(),
    QGeoCoordinate{
        linearSlopeRegion.topRight().latitude() - UnitTestTerrainQuery::regionSizeDeg,
        linearSlopeRegion.topRight().longitude() + UnitTestTerrainQuery::regionSizeDeg
    }
}};

UnitTestTerrainQuery::UnitTestTerrainQuery(QObject *parent)
    : TerrainQueryInterface(parent)
{

}

UnitTestTerrainQuery::~UnitTestTerrainQuery()
{

}

void UnitTestTerrainQuery::requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    const QList<double> result = _requestCoordinateHeights(coordinates);
    emit coordinateHeightsReceived(result.size() == coordinates.size(), result);
}

void UnitTestTerrainQuery::requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    const PathHeightInfo_t pathHeightInfo = _requestPathHeights(fromCoord, toCoord);
    emit pathHeightsReceived(
        !pathHeightInfo.rgHeights.isEmpty(),
        pathHeightInfo.distanceBetween,
        pathHeightInfo.finalDistanceBetween,
        pathHeightInfo.rgHeights
    );
}

void UnitTestTerrainQuery::requestCarpetHeights(const QGeoCoordinate &swCoord, const QGeoCoordinate &neCoord, bool statsOnly)
{
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
        (void) carpet.append(row);
    }

    emit carpetHeightsReceived(true, min, max, carpet);
}

UnitTestTerrainQuery::PathHeightInfo_t UnitTestTerrainQuery::_requestPathHeights(const QGeoCoordinate &fromCoord, const QGeoCoordinate &toCoord)
{
    PathHeightInfo_t pathHeights;
    pathHeights.rgCoords = TerrainTileManager::_pathQueryToCoords(fromCoord, toCoord, pathHeights.distanceBetween, pathHeights.finalDistanceBetween);
    pathHeights.rgHeights = _requestCoordinateHeights(pathHeights.rgCoords);
    return pathHeights;
}

QList<double> UnitTestTerrainQuery::_requestCoordinateHeights(const QList<QGeoCoordinate> &coordinates)
{
    QList<double> result;

    for (const auto &coordinate : coordinates) {
        if (flat10Region.contains(coordinate)) {
            (void) result.append(UnitTestTerrainQuery::Flat10Region::amslElevation);
        } else if (linearSlopeRegion.contains(coordinate)) {
            // cast to oneSecondDeg grid and round to int to emulate SRTM1 even better
            const double x = (coordinate.longitude() - linearSlopeRegion.topLeft().longitude()) / oneSecondDeg;
            const double dx = regionSizeDeg / oneSecondDeg;
            const double fraction = x / dx;
            (void) result.append(std::round(UnitTestTerrainQuery::LinearSlopeRegion::minAMSLElevation + (fraction * UnitTestTerrainQuery::LinearSlopeRegion::totalElevationChange)));
        } else if (hillRegion.contains(coordinate)) {
            const double arcSecondMeters = (earthsRadiusMts * oneSecondDeg) * (M_PI / 180.);
            const double x = (coordinate.latitude() - hillRegion.center().latitude()) * arcSecondMeters / oneSecondDeg;
            const double y = (coordinate.longitude() - hillRegion.center().longitude()) * arcSecondMeters / oneSecondDeg;
            const double x2y2 = pow(x, 2) + pow(y, 2);
            const double r2 = pow(UnitTestTerrainQuery::HillRegion::radius, 2);
            const double z = (x2y2 <= r2) ? sqrt(r2 - x2y2) : UnitTestTerrainQuery::Flat10Region::amslElevation;
            (void) result.append(z);
        } else {
            result.clear();
            break;
        }
    }

    return result;
}

/*===========================================================================*/

void TerrainQueryTest::_testRequestCoordinateHeights()
{
    UnitTestTerrainQuery* const query = new UnitTestTerrainQuery(this);
    QSignalSpy spy(query, &UnitTestTerrainQuery::coordinateHeightsReceived);
    QVERIFY(spy.isValid());

    const QList<QGeoCoordinate> coords = {
        pointNemo
    };
    query->requestCoordinateHeights(coords);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QCOMPARE(arguments.at(1).toList().size(), coords.size());
    QVERIFY(arguments.at(1).toList().at(0).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestPathHeights()
{
    UnitTestTerrainQuery* const query = new UnitTestTerrainQuery(this);
    QSignalSpy spy(query, &UnitTestTerrainQuery::pathHeightsReceived);
    QVERIFY(spy.isValid());

    const QGeoCoordinate from = pointNemo;
    const QGeoCoordinate to = QGeoCoordinate(pointNemo.latitude(), pointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg);
    query->requestPathHeights(from, to);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QVERIFY(arguments.at(1).toDouble() > 0.);
    QVERIFY(arguments.at(2).toDouble() > 0.);
    QVERIFY(arguments.at(3).toList().size() > 2);
    QVERIFY(arguments.at(3).toList().constFirst().toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(arguments.at(3).toList().constLast().toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestCarpetHeights()
{
    UnitTestTerrainQuery* const query = new UnitTestTerrainQuery(this);
    QSignalSpy spy(query, &UnitTestTerrainQuery::carpetHeightsReceived);
    QVERIFY(spy.isValid());

    const QGeoCoordinate sw = pointNemo;
    const QGeoCoordinate ne = QGeoCoordinate(pointNemo.latitude() + UnitTestTerrainQuery::regionSizeDeg, pointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg);
    query->requestCarpetHeights(sw, ne, false);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QVERIFY(arguments.at(1).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(arguments.at(2).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(!arguments.at(3).toList().isEmpty());
    QVERIFY(!arguments.at(3).toList().constFirst().toList().isEmpty());
    QVERIFY(arguments.at(3).toList().constFirst().toList().constFirst().toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
}

// Test Requires Internet, so disable by default.
// Or, check if internet and elevation server are available?
#if 0
void TerrainQueryTest::_testTerrainAtCoordinateQuery()
{
    const QGeoCoordinate coord{QRandomGenerator::global()->bounded(90.0), QRandomGenerator::global()->bounded(90.0), 0.0};
    QList<double> altitudes;
    QList<QGeoCoordinate> coordinates;
    (void) coordinates.append(pointNemo);
    (void) coordinates.append(coord);
    TerrainAtCoordinateQuery* query = new TerrainAtCoordinateQuery(true, this);
    query->requestData(coordinates);
    bool result = false;
    for (uint8_t attempts = 0; attempts < 20; attempts++) {
        bool error = false;
        const bool altAvailable = TerrainAtCoordinateQuery::getAltitudesForCoordinates(coordinates, altitudes, error);
        QVERIFY(!error);
        result |= altAvailable;
        if (result) {
            break;
        }
        QTest::qWait(500);
    }
    QVERIFY(result);
}
#endif
