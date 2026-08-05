#include "Viewer3DManagerTest.h"
#include <QtTest/QSignalSpy>


#include "Viewer3DManager.h"

void Viewer3DManagerTest::_testInitialState()
{
    Viewer3DManager mgr;

    QCOMPARE(mgr.displayMode(), Viewer3DManager::Map);
    QVERIFY(mgr.mapProvider() != nullptr);
    QVERIFY(!mgr.gpsRef().isValid());

    // No vehicle and no OSM map loaded: nothing can be outside the region
    QVERIFY(!mgr.vehicleOutsideMapRegion());
}

void Viewer3DManagerTest::_testCoordinateWithinRegion()
{
    const QGeoCoordinate bbMin(47.39, 8.54);
    const QGeoCoordinate bbMax(47.40, 8.55);

    QVERIFY(Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.395, 8.545), bbMin, bbMax));
    QVERIFY(Viewer3DManager::coordinateWithinRegion(bbMin, bbMin, bbMax));
    QVERIFY(Viewer3DManager::coordinateWithinRegion(bbMax, bbMin, bbMax));

    // Vehicle on another continent (issue #13816 scenario)
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(-35.3633, 149.165), bbMin, bbMax));

    // Just outside each edge
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.389, 8.545), bbMin, bbMax));
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.401, 8.545), bbMin, bbMax));
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.395, 8.539), bbMin, bbMax));
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.395, 8.551), bbMin, bbMax));

    // Invalid inputs are never "within"
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(), bbMin, bbMax));
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.395, 8.545), QGeoCoordinate(), bbMax));
    QVERIFY(!Viewer3DManager::coordinateWithinRegion(QGeoCoordinate(47.395, 8.545), bbMin, QGeoCoordinate()));
}

void Viewer3DManagerTest::_testSetDisplayMode()
{
    Viewer3DManager mgr;
    QSignalSpy spy(&mgr, &Viewer3DManager::displayModeChanged);
    QVERIFY(spy.isValid());

    // Map -> Map should be a no-op
    mgr.setDisplayMode(Viewer3DManager::Map);
    QCOMPARE(spy.count(), 0);
}

void Viewer3DManagerTest::_testSetDisplayModeNoop()
{
    Viewer3DManager mgr;
    QSignalSpy spy(&mgr, &Viewer3DManager::displayModeChanged);
    QVERIFY(spy.isValid());

    // Switching to same mode does not emit
    mgr.setDisplayMode(Viewer3DManager::Map);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(mgr.displayMode(), Viewer3DManager::Map);
}

UT_REGISTER_TEST(Viewer3DManagerTest, TestLabel::Unit)
