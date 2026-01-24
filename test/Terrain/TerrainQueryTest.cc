#include "TerrainQueryTest.h"
#include "TestHelpers.h"
#include "QtTestExtensions.h"
#include "TerrainTileManager.h"
#include "TerrainQuery.h"

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

// ============================================================================
// Test Constants
// ============================================================================

/// Point Nemo is a point on Earth furthest from land
const QGeoCoordinate TerrainQueryTest::kPointNemo(-48.875556, -123.392500);

// ============================================================================
// UnitTestTerrainQuery - Mock Terrain Provider
// ============================================================================

const UnitTestTerrainQuery::Flat10Region UnitTestTerrainQuery::flat10Region{{
    TerrainQueryTest::kPointNemo,
    QGeoCoordinate{
        TerrainQueryTest::kPointNemo.latitude() - UnitTestTerrainQuery::regionSizeDeg,
        TerrainQueryTest::kPointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg
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
        carpet.append(row);
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
            result.append(Flat10Region::amslElevation);
        } else if (linearSlopeRegion.contains(coordinate)) {
            // Cast to oneSecondDeg grid and round to int to emulate SRTM1
            const double x = (coordinate.longitude() - linearSlopeRegion.topLeft().longitude()) / oneSecondDeg;
            const double dx = regionSizeDeg / oneSecondDeg;
            const double fraction = x / dx;
            result.append(std::round(LinearSlopeRegion::minAMSLElevation + (fraction * LinearSlopeRegion::totalElevationChange)));
        } else if (hillRegion.contains(coordinate)) {
            const double arcSecondMeters = (earthsRadiusMts * oneSecondDeg) * (M_PI / 180.);
            const double x = (coordinate.latitude() - hillRegion.center().latitude()) * arcSecondMeters / oneSecondDeg;
            const double y = (coordinate.longitude() - hillRegion.center().longitude()) * arcSecondMeters / oneSecondDeg;
            const double x2y2 = pow(x, 2) + pow(y, 2);
            const double r2 = pow(HillRegion::radius, 2);
            const double z = (x2y2 <= r2) ? sqrt(r2 - x2y2) : Flat10Region::amslElevation;
            result.append(z);
        } else {
            result.clear();
            break;
        }
    }

    return result;
}

// ============================================================================
// TerrainQueryTest - Test Helpers
// ============================================================================

UnitTestTerrainQuery* TerrainQueryTest::_createQuery()
{
    UnitTestTerrainQuery* query = new UnitTestTerrainQuery(this);
    Q_ASSERT(query);
    return query;
}

// ============================================================================
// TerrainQueryTest - Coordinate Height Tests
// ============================================================================

void TerrainQueryTest::_testRequestCoordinateHeights()
{
    UnitTestTerrainQuery* query = _createQuery();
    QSignalSpy spy(query, &UnitTestTerrainQuery::coordinateHeightsReceived);
    QGC_VERIFY_SPY_VALID(spy);

    const QList<QGeoCoordinate> coords = { kPointNemo };
    query->requestCoordinateHeights(coords);

    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), true);
    QCOMPARE_EQ(args.at(1).toList().size(), coords.size());
    QCOMPARE_EQ(args.at(1).toList().at(0).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
}

// ============================================================================
// TerrainQueryTest - Path Height Tests
// ============================================================================

void TerrainQueryTest::_testRequestPathHeights()
{
    UnitTestTerrainQuery* query = _createQuery();
    QSignalSpy spy(query, &UnitTestTerrainQuery::pathHeightsReceived);
    QGC_VERIFY_SPY_VALID(spy);

    const QGeoCoordinate from = kPointNemo;
    const QGeoCoordinate to(kPointNemo.latitude(), kPointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg);
    query->requestPathHeights(from, to);

    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), true);
    QCOMPARE_GT(args.at(1).toDouble(), 0.);  // distanceBetween
    QCOMPARE_GT(args.at(2).toDouble(), 0.);  // finalDistanceBetween
    QCOMPARE_GT(args.at(3).toList().size(), 2);
    QCOMPARE_EQ(args.at(3).toList().constFirst().toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
    QCOMPARE_EQ(args.at(3).toList().constLast().toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
}

// ============================================================================
// TerrainQueryTest - Carpet Height Tests
// ============================================================================

void TerrainQueryTest::_testRequestCarpetHeights()
{
    UnitTestTerrainQuery* query = _createQuery();
    QSignalSpy spy(query, &UnitTestTerrainQuery::carpetHeightsReceived);
    QGC_VERIFY_SPY_VALID(spy);

    const QGeoCoordinate sw = kPointNemo;
    const QGeoCoordinate ne(kPointNemo.latitude() + UnitTestTerrainQuery::regionSizeDeg,
                           kPointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg);
    query->requestCarpetHeights(sw, ne, false);

    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), true);
    QCOMPARE_EQ(args.at(1).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);  // min
    QCOMPARE_EQ(args.at(2).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);  // max
    QGC_VERIFY_NOT_EMPTY(args.at(3).toList());
    QGC_VERIFY_NOT_EMPTY(args.at(3).toList().constFirst().toList());
    QCOMPARE_EQ(args.at(3).toList().constFirst().toList().constFirst().toDouble(),
                UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestCarpetHeightsInvalidBounds()
{
    UnitTestTerrainQuery* query = _createQuery();
    QSignalSpy spy(query, &UnitTestTerrainQuery::carpetHeightsReceived);
    QGC_VERIFY_SPY_VALID(spy);

    // SW and NE are reversed (invalid bounds)
    const QGeoCoordinate sw(kPointNemo.latitude() + UnitTestTerrainQuery::regionSizeDeg,
                           kPointNemo.longitude() + UnitTestTerrainQuery::regionSizeDeg);
    const QGeoCoordinate ne = kPointNemo;
    query->requestCarpetHeights(sw, ne, false);

    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), false);
    QVERIFY(qIsNaN(args.at(1).toDouble()));
    QVERIFY(qIsNaN(args.at(2).toDouble()));
}

// ============================================================================
// TerrainQueryTest - Poly Path Query Tests
// ============================================================================

void TerrainQueryTest::_testPolyPathQueryEmptyPath()
{
    TerrainPolyPathQuery* query = new TerrainPolyPathQuery(true, this);
    VERIFY_NOT_NULL(query);

    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QGC_VERIFY_SPY_VALID(spy);

    const QList<QGeoCoordinate> emptyPath;
    query->requestData(emptyPath);

    // Signal is emitted synchronously for invalid path
    QCOMPARE_EQ(spy.count(), 1);
    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), false);
}

void TerrainQueryTest::_testPolyPathQuerySingleCoord()
{
    TerrainPolyPathQuery* query = new TerrainPolyPathQuery(true, this);
    VERIFY_NOT_NULL(query);

    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QGC_VERIFY_SPY_VALID(spy);

    const QList<QGeoCoordinate> singleCoordPath = { kPointNemo };
    query->requestData(singleCoordPath);

    // Signal is emitted synchronously for invalid path
    QCOMPARE_EQ(spy.count(), 1);
    const QVariantList args = QGC_SPY_TAKE_FIRST(spy);
    QCOMPARE_EQ(args.at(0).toBool(), false);
}
