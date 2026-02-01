#include "TerrainQueryTest.h"

#include <QtTest/QSignalSpy>

#include "TerrainQuery.h"
#include "TerrainQueryInterface.h"

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

void TerrainQueryTest::_testTerrainAtCoordinateQuery()
{
    // Inject our test terrain query into the batch manager
    TerrainAtCoordinateBatchManager::instance()->setTerrainQueryInterface(new UnitTestTerrainQuery());

    QList<QGeoCoordinate> coordinates;
    (void) coordinates.append(pointNemo());
    (void) coordinates.append(QGeoCoordinate(pointNemo().latitude() - 0.01, pointNemo().longitude() + 0.01));

    TerrainAtCoordinateQuery* const query = new TerrainAtCoordinateQuery(true, this);
    QSignalSpy spy(query, &TerrainAtCoordinateQuery::terrainDataReceived);
    QVERIFY(spy.isValid());

    query->requestData(coordinates);

    QVERIFY(spy.wait(5000));
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
