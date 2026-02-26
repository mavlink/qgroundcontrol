#include "TerrainQueryTest.h"

#include <QtCore/QMetaObject>
#include <QtTest/QSignalSpy>

#include "TerrainQuery.h"
#include "TerrainQueryInterface.h"
#include "TerrainTileCopernicus.h"

void TerrainQueryTest::_testRequestCoordinateHeights()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::coordinateHeightsReceived);
    QVERIFY(spy.isValid());
    const QList<QGeoCoordinate> coords = {pointNemo()};
    terrainQuery()->requestCoordinateHeights(coords);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QCOMPARE(arguments.at(1).toList().size(), coords.size());
    QVERIFY(arguments.at(1).toList().at(0).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestPathHeights()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::pathHeightsReceived);
    QVERIFY(spy.isValid());
    const QGeoCoordinate from = pointNemo();
    const QGeoCoordinate to =
        QGeoCoordinate(pointNemo().latitude(), pointNemo().longitude() + UnitTestTerrainQuery::regionSizeDeg);
    terrainQuery()->requestPathHeights(from, to);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QVERIFY(arguments.at(1).toDouble() > 0.);
    QVERIFY(arguments.at(2).toDouble() > 0.);
    QVERIFY(arguments.at(3).toList().size() > 2);
    QVERIFY(arguments.at(3).toList().constFirst().toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(arguments.at(3).toList().constLast().toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestPathHeightsSpacing()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::pathHeightsReceived);
    QVERIFY(spy.isValid());

    const QGeoCoordinate from = pointNemo();
    const QGeoCoordinate to = QGeoCoordinate(pointNemo().latitude(), pointNemo().longitude() + UnitTestTerrainQuery::regionSizeDeg);
    terrainQuery()->requestPathHeights(from, to);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);

    const double distanceBetween = arguments.at(1).toDouble();
    const double finalDistanceBetween = arguments.at(2).toDouble();
    QVERIFY(distanceBetween > 0.0);
    QVERIFY(finalDistanceBetween > 0.0);

    // Spacing should track the terrain tile sample spacing with a small tolerance.
    QVERIFY(qAbs(distanceBetween - TerrainTileCopernicus::kTileValueSpacingMeters) < 2.0);
    QVERIFY(qAbs(finalDistanceBetween - TerrainTileCopernicus::kTileValueSpacingMeters) < 2.0);
}

void TerrainQueryTest::_testRequestCarpetHeights()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::carpetHeightsReceived);
    QVERIFY(spy.isValid());
    const QGeoCoordinate sw = pointNemo();
    const QGeoCoordinate ne = QGeoCoordinate(pointNemo().latitude() + UnitTestTerrainQuery::regionSizeDeg,
                                             pointNemo().longitude() + UnitTestTerrainQuery::regionSizeDeg);
    terrainQuery()->requestCarpetHeights(sw, ne, false);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QVERIFY(arguments.at(1).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(arguments.at(2).toDouble() == UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(!arguments.at(3).toList().isEmpty());
    QVERIFY(!arguments.at(3).toList().constFirst().toList().isEmpty());
    QVERIFY(arguments.at(3).toList().constFirst().toList().constFirst().toDouble() ==
            UnitTestTerrainQuery::Flat10Region::amslElevation);
}

void TerrainQueryTest::_testRequestCarpetHeightsStatsOnly()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::carpetHeightsReceived);
    QVERIFY(spy.isValid());

    const QGeoCoordinate sw = pointNemo();
    const QGeoCoordinate ne = QGeoCoordinate(pointNemo().latitude() + UnitTestTerrainQuery::regionSizeDeg,
                                             pointNemo().longitude() + UnitTestTerrainQuery::regionSizeDeg);
    terrainQuery()->requestCarpetHeights(sw, ne, true);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);
    QCOMPARE(arguments.at(1).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
    QCOMPARE(arguments.at(2).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
    QVERIFY(arguments.at(3).toList().isEmpty());
}

void TerrainQueryTest::_testRequestCarpetHeightsInvalidBounds()
{
    QSignalSpy spy(terrainQuery(), &UnitTestTerrainQuery::carpetHeightsReceived);
    QVERIFY(spy.isValid());
    // SW and NE are reversed (NE is actually SW)
    const QGeoCoordinate sw = QGeoCoordinate(pointNemo().latitude() + UnitTestTerrainQuery::regionSizeDeg,
                                             pointNemo().longitude() + UnitTestTerrainQuery::regionSizeDeg);
    const QGeoCoordinate ne = pointNemo();
    terrainQuery()->requestCarpetHeights(sw, ne, false);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
    QVERIFY(qIsNaN(arguments.at(1).toDouble()));
    QVERIFY(qIsNaN(arguments.at(2).toDouble()));
}

void TerrainQueryTest::_testPolyPathQueryEmptyPath()
{
    TerrainPolyPathQuery* const query = new TerrainPolyPathQuery(true, this);
    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());
    const QList<QGeoCoordinate> emptyPath;
    query->requestData(emptyPath);
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
    (void)singleCoordPath.append(pointNemo());
    query->requestData(singleCoordPath);
    // Signal is emitted synchronously for invalid path
    QCOMPARE(spy.count(), 1);
    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);
}

void TerrainQueryTest::_testPolyPathQueryFailureClearsAccumulatedSegments()
{
    TerrainPolyPathQuery *const query = new TerrainPolyPathQuery(false, this);
    QSignalSpy spy(query, &TerrainPolyPathQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    TerrainPathQuery::PathHeightInfo_t segment;
    segment.distanceBetween = 10.0;
    segment.finalDistanceBetween = 10.0;
    (void) segment.heights.append(UnitTestTerrainQuery::Flat10Region::amslElevation);
    (void) segment.heights.append(UnitTestTerrainQuery::Flat10Region::amslElevation);

    bool invoked = QMetaObject::invokeMethod(query, "_terrainDataReceived", Qt::DirectConnection,
                                             Q_ARG(bool, true),
                                             Q_ARG(TerrainPathQuery::PathHeightInfo_t, segment));
    QVERIFY(invoked);
    QCOMPARE(spy.count(), 1);
    const QVariantList successArgs = spy.takeFirst();
    QVERIFY(successArgs.at(0).toBool());

    const QList<TerrainPathQuery::PathHeightInfo_t> initialSegments =
        qvariant_cast<QList<TerrainPathQuery::PathHeightInfo_t>>(successArgs.at(1));
    QVERIFY(!initialSegments.isEmpty());

    const TerrainPathQuery::PathHeightInfo_t emptySegment{};
    invoked = QMetaObject::invokeMethod(query, "_terrainDataReceived", Qt::DirectConnection,
                                        Q_ARG(bool, false),
                                        Q_ARG(TerrainPathQuery::PathHeightInfo_t, emptySegment));
    QVERIFY(invoked);

    QCOMPARE(spy.count(), 1);
    const QVariantList failureArgs = spy.takeFirst();
    QVERIFY(!failureArgs.at(0).toBool());
    const QList<TerrainPathQuery::PathHeightInfo_t> clearedSegments =
        qvariant_cast<QList<TerrainPathQuery::PathHeightInfo_t>>(failureArgs.at(1));
    QVERIFY(clearedSegments.isEmpty());
}

void TerrainQueryTest::_testTerrainAtCoordinateQuery()
{
    // Inject our test terrain query into the batch manager
    TerrainAtCoordinateBatchManager::instance()->setTerrainQueryInterface(new UnitTestTerrainQuery());

    QList<QGeoCoordinate> coordinates;
    (void)coordinates.append(pointNemo());
    (void)coordinates.append(QGeoCoordinate(pointNemo().latitude() - 0.01, pointNemo().longitude() + 0.01));

    TerrainAtCoordinateQuery* const query = new TerrainAtCoordinateQuery(true, this);
    QSignalSpy spy(query, &TerrainAtCoordinateQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    query->requestData(coordinates);

    QVERIFY_SIGNAL_WAIT(spy, TestTimeout::mediumMs());
    QCOMPARE(spy.count(), 1);

    const QVariantList arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);

    const QList<QVariant> heights = arguments.at(1).toList();
    QCOMPARE(heights.size(), coordinates.size());
    QCOMPARE(heights.at(0).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);
    QCOMPARE(heights.at(1).toDouble(), UnitTestTerrainQuery::Flat10Region::amslElevation);

    // Restore default terrain query for other tests
    TerrainAtCoordinateBatchManager::instance()->setTerrainQueryInterface(new TerrainOfflineQuery());
}

UT_REGISTER_TEST(TerrainQueryTest, TestLabel::Integration, TestLabel::Terrain, TestLabel::Network)
