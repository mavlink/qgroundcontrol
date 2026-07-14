#include "TerrainQueryTest.h"

#include <QtCore/QMetaObject>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

#include "TerrainQuery.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileCopernicus.h"

// These tests run the full production terrain pipeline: query classes route through
// TerrainTileManager, whose elevation tile cache misses are served synthetic tiles
// built from the UnitTestTerrainData regions (see UnitTestTileGenerator). No network
// access ever occurs.

void TerrainQueryTest::_testRequestCoordinateHeights()
{
    TerrainAtCoordinateQuery* const query = new TerrainAtCoordinateQuery(true, this);
    QSignalSpy spy(query, &TerrainAtCoordinateQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    // Half an arc-second inside the flat region's north-west corner (pointNemo).
    // Exact edge coordinates are ambiguous with grid cell floor-indexing.
    const QGeoCoordinate nearNwCorner(pointNemo().latitude() - (UnitTestTerrainData::oneSecondDeg / 2.0),
                                      pointNemo().longitude() + (UnitTestTerrainData::oneSecondDeg / 2.0));
    const QList<QGeoCoordinate> coords = {nearNwCorner, flat10Region().center()};
    query->requestData(coords);

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    const QList<double> heights = qvariant_cast<QList<double>>(arguments.at(1));
    QCOMPARE(heights.size(), coords.size());
    QCOMPARE(heights.at(0), UnitTestTerrainData::Flat10Region::amslElevation);
    QCOMPARE(heights.at(1), UnitTestTerrainData::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestCoordinateHeightsSlope()
{
    TerrainAtCoordinateQuery* const query = new TerrainAtCoordinateQuery(true, this);
    QSignalSpy spy(query, &TerrainAtCoordinateQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    // Center of the linear slope region: halfway between -100m and 1000m. The tile
    // grid samples on 1 arc-second cells, so allow one cell (~3m of slope) of error.
    query->requestData({linearSlopeRegion().center()});

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    const QList<double> heights = qvariant_cast<QList<double>>(arguments.at(1));
    QCOMPARE(heights.size(), 1);
    QVERIFY(qAbs(heights.at(0) - 450.0) <= 5.0);
}

void TerrainQueryTest::_testRequestCoordinateHeightsOutsideRegions()
{
    TerrainAtCoordinateQuery* const query = new TerrainAtCoordinateQuery(true, this);
    QSignalSpy spy(query, &TerrainAtCoordinateQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    // Coordinates outside the test regions succeed with 0 height
    query->requestData({QGeoCoordinate(0.005, 0.005)});

    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    const QList<double> heights = qvariant_cast<QList<double>>(arguments.at(1));
    QCOMPARE(heights.size(), 1);
    QCOMPARE(heights.at(0), 0.0);
}

void TerrainQueryTest::_testRequestPathHeights()
{
    TerrainPathQuery* const query = new TerrainPathQuery(true, this);
    QSignalSpy spy(query, &TerrainPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    const double centerLat = flat10Region().center().latitude();
    const QGeoCoordinate from(centerLat, flat10Region().topLeft().longitude() + 0.02);
    const QGeoCoordinate to(centerLat, flat10Region().topRight().longitude() - 0.02);
    query->requestData(from, to);

    // The signal is synchronous when all tiles are already cached, async otherwise
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    const auto pathHeightInfo = qvariant_cast<TerrainPathQuery::PathHeightInfo_t>(arguments.at(1));
    QVERIFY(pathHeightInfo.heights.size() > 2);
    QCOMPARE(pathHeightInfo.heights.constFirst(), UnitTestTerrainData::Flat10Region::amslElevation);
    QCOMPARE(pathHeightInfo.heights.constLast(), UnitTestTerrainData::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestPathHeightsSpacing()
{
    TerrainPathQuery* const query = new TerrainPathQuery(true, this);
    QSignalSpy spy(query, &TerrainPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    const double centerLat = flat10Region().center().latitude();
    const QGeoCoordinate from(centerLat, flat10Region().topLeft().longitude() + 0.02);
    const QGeoCoordinate to(centerLat, flat10Region().topRight().longitude() - 0.02);
    query->requestData(from, to);

    // The signal is synchronous when all tiles are already cached, async otherwise
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    // Spacing should track the terrain tile sample spacing with a small tolerance.
    const auto pathHeightInfo = qvariant_cast<TerrainPathQuery::PathHeightInfo_t>(arguments.at(1));
    QVERIFY(pathHeightInfo.distanceBetween > 0.0);
    QVERIFY(pathHeightInfo.finalDistanceBetween > 0.0);
    QVERIFY(qAbs(pathHeightInfo.distanceBetween - TerrainTileCopernicus::kTileValueSpacingMeters) < 2.0);
    QVERIFY(qAbs(pathHeightInfo.finalDistanceBetween - TerrainTileCopernicus::kTileValueSpacingMeters) < 2.0);
}

void TerrainQueryTest::_testRequestCarpetHeights()
{
    TerrainAreaQuery* const query = new TerrainAreaQuery(true, this);
    QSignalSpy spy(query, &TerrainAreaQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    // Rectangle fully inside the flat region
    const QGeoCoordinate regionCenter = flat10Region().center();
    const QGeoCoordinate sw(regionCenter.latitude() - 0.01, regionCenter.longitude() - 0.01);
    const QGeoCoordinate ne(regionCenter.latitude() + 0.01, regionCenter.longitude() + 0.01);
    query->requestData(sw, ne);

    // The signal is synchronous when all tiles are already cached, async otherwise
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());

    const auto carpetHeightInfo = qvariant_cast<TerrainAreaQuery::CarpetHeightInfo_t>(arguments.at(1));
    QCOMPARE(carpetHeightInfo.minHeight, UnitTestTerrainData::Flat10Region::amslElevation);
    QCOMPARE(carpetHeightInfo.maxHeight, UnitTestTerrainData::Flat10Region::amslElevation);
    QVERIFY(!carpetHeightInfo.carpet.isEmpty());
    QVERIFY(!carpetHeightInfo.carpet.constFirst().isEmpty());
    QCOMPARE(carpetHeightInfo.carpet.constFirst().constFirst(), UnitTestTerrainData::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestCarpetHeightsStatsOnly()
{
    TerrainOfflineQuery* const query = new TerrainOfflineQuery(this);
    QSignalSpy spy(query, &TerrainQueryInterface::carpetHeightsReceived);
    QVERIFY(spy.isValid());

    const QGeoCoordinate regionCenter = flat10Region().center();
    const QGeoCoordinate sw(regionCenter.latitude() - 0.01, regionCenter.longitude() - 0.01);
    const QGeoCoordinate ne(regionCenter.latitude() + 0.01, regionCenter.longitude() + 0.01);
    query->requestCarpetHeights(sw, ne, true /* statsOnly */);

    // The signal is synchronous when all tiles are already cached, async otherwise
    QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, TestTimeout::mediumMs());
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool());
    QCOMPARE(arguments.at(1).toDouble(), UnitTestTerrainData::Flat10Region::amslElevation);
    QCOMPARE(arguments.at(2).toDouble(), UnitTestTerrainData::Flat10Region::amslElevation);
    QVERIFY(qvariant_cast<QList<QList<double>>>(arguments.at(3)).isEmpty());
}

void TerrainQueryTest::_testRequestCarpetHeightsInvalidBounds()
{
    TerrainOfflineQuery* const query = new TerrainOfflineQuery(this);
    QSignalSpy spy(query, &TerrainQueryInterface::carpetHeightsReceived);
    QVERIFY(spy.isValid());

    // SW and NE are reversed (NE is actually SW)
    const QGeoCoordinate sw = QGeoCoordinate(pointNemo().latitude() + UnitTestTerrainData::regionSizeDeg,
                                             pointNemo().longitude() + UnitTestTerrainData::regionSizeDeg);
    const QGeoCoordinate ne = pointNemo();
    expectLogMessage("Terrain.TerrainTileManager", QtWarningMsg, QRegularExpression("Invalid carpet bounds"));
    query->requestCarpetHeights(sw, ne, false /* statsOnly */);
    verifyExpectedLogMessage();

    // Signal is emitted synchronously for invalid bounds
    QCOMPARE(spy.count(), 1);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(!arguments.at(0).toBool());
    QVERIFY(qIsNaN(arguments.at(1).toDouble()));
    QVERIFY(qIsNaN(arguments.at(2).toDouble()));
}

void TerrainQueryTest::_testPolyPathQueryEmptyPath()
{
    TerrainPolyPathQuery* const query = new TerrainPolyPathQuery(true, this);
    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());
    const QList<QGeoCoordinate> emptyPath;
    expectLogMessage("Terrain.TerrainQuery", QtWarningMsg,
                     QRegularExpression("polyPath requires at least 2 coordinates"));
    query->requestData(emptyPath);
    verifyExpectedLogMessage();
    // Signal is emitted synchronously for invalid path
    QCOMPARE(spy.count(), 1);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
}

void TerrainQueryTest::_testPolyPathQuerySingleCoord()
{
    TerrainPolyPathQuery* const query = new TerrainPolyPathQuery(true, this);
    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());
    QList<QGeoCoordinate> singleCoordPath;
    (void) singleCoordPath.append(pointNemo());
    expectLogMessage("Terrain.TerrainQuery", QtWarningMsg,
                     QRegularExpression("polyPath requires at least 2 coordinates"));
    query->requestData(singleCoordPath);
    verifyExpectedLogMessage();
    // Signal is emitted synchronously for invalid path
    QCOMPARE(spy.count(), 1);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
}

void TerrainQueryTest::_testPolyPathQueryFailureClearsAccumulatedSegments()
{
    TerrainPolyPathQuery* const query = new TerrainPolyPathQuery(false, this);
    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    TerrainPathQuery::PathHeightInfo_t segment;
    segment.distanceBetween = 10.0;
    segment.finalDistanceBetween = 10.0;
    (void) segment.heights.append(UnitTestTerrainData::Flat10Region::amslElevation);
    (void) segment.heights.append(UnitTestTerrainData::Flat10Region::amslElevation);

    bool invoked = QMetaObject::invokeMethod(query, "_terrainDataReceived", Qt::DirectConnection, Q_ARG(bool, true),
                                             Q_ARG(TerrainPathQuery::PathHeightInfo_t, segment));
    QVERIFY(invoked);
    QCOMPARE(spy.count(), 1);
    const QVariantList successArgs = spy.takeFirst();
    QVERIFY(successArgs.at(0).toBool());

    const QList<TerrainPathQuery::PathHeightInfo_t> initialSegments =
        qvariant_cast<QList<TerrainPathQuery::PathHeightInfo_t>>(successArgs.at(1));
    QVERIFY(!initialSegments.isEmpty());

    const TerrainPathQuery::PathHeightInfo_t emptySegment{};
    invoked = QMetaObject::invokeMethod(query, "_terrainDataReceived", Qt::DirectConnection, Q_ARG(bool, false),
                                        Q_ARG(TerrainPathQuery::PathHeightInfo_t, emptySegment));
    QVERIFY(invoked);

    QCOMPARE(spy.count(), 1);
    const QVariantList failureArgs = spy.takeFirst();
    QVERIFY(!failureArgs.at(0).toBool());
    const QList<TerrainPathQuery::PathHeightInfo_t> clearedSegments =
        qvariant_cast<QList<TerrainPathQuery::PathHeightInfo_t>>(failureArgs.at(1));
    QVERIFY(clearedSegments.isEmpty());
}

UT_REGISTER_TEST(TerrainQueryTest, TestLabel::Integration, TestLabel::Terrain)
