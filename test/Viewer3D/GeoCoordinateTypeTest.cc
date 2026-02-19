#include "GeoCoordinateTypeTest.h"

#include <QtTest/QSignalSpy>

#include "Viewer3DGeoCoordinateType.h"

void GeoCoordinateTypeTest::_testGpsToLocal()
{
    Viewer3DGeoCoordinateType geo;

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    QGeoCoordinate coord(47.3987, 8.5466, 410);

    geo.setGpsRef(ref);
    geo.setCoordinate(coord);

    QVector3D local = geo.localCoordinate();

    // Should have non-zero east and north components
    QVERIFY(qAbs(local.x()) > 0.1f);
    QVERIFY(qAbs(local.y()) > 0.1f);
    // Z should be the coordinate altitude
    QCOMPARE_FUZZY(local.z(), 410.0f, 0.1f);
}

void GeoCoordinateTypeTest::_testSetGpsRefEmitsSignal()
{
    Viewer3DGeoCoordinateType geo;

    QSignalSpy refSpy(&geo, &Viewer3DGeoCoordinateType::gpsRefChanged);
    QSignalSpy localSpy(&geo, &Viewer3DGeoCoordinateType::localCoordinateChanged);
    QVERIFY(refSpy.isValid());
    QVERIFY(localSpy.isValid());

    // Set a coordinate first so that changing gpsRef triggers localCoordinate update
    geo.setCoordinate(QGeoCoordinate(47.3987, 8.5466, 410));

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    geo.setGpsRef(ref);

    QCOMPARE(refSpy.count(), 1);
    QCOMPARE(geo.gpsRef(), ref);
    QVERIFY(localSpy.count() >= 1);
}

void GeoCoordinateTypeTest::_testSetCoordinateEmitsSignal()
{
    Viewer3DGeoCoordinateType geo;

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    geo.setGpsRef(ref);

    QSignalSpy coordSpy(&geo, &Viewer3DGeoCoordinateType::coordinateChanged);
    QSignalSpy localSpy(&geo, &Viewer3DGeoCoordinateType::localCoordinateChanged);
    QVERIFY(coordSpy.isValid());
    QVERIFY(localSpy.isValid());

    QGeoCoordinate coord(47.3987, 8.5466, 410);
    geo.setCoordinate(coord);

    QCOMPARE(coordSpy.count(), 1);
    QCOMPARE(geo.coordinate(), coord);
    QVERIFY(localSpy.count() >= 1);
}

void GeoCoordinateTypeTest::_testSameValueNoEmit()
{
    Viewer3DGeoCoordinateType geo;

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    QGeoCoordinate coord(47.3987, 8.5466, 410);

    geo.setGpsRef(ref);
    geo.setCoordinate(coord);

    QSignalSpy refSpy(&geo, &Viewer3DGeoCoordinateType::gpsRefChanged);
    QSignalSpy coordSpy(&geo, &Viewer3DGeoCoordinateType::coordinateChanged);

    // Setting same values should not emit
    geo.setGpsRef(ref);
    geo.setCoordinate(coord);

    QCOMPARE(refSpy.count(), 0);
    QCOMPARE(coordSpy.count(), 0);
}

void GeoCoordinateTypeTest::_testLocalCoordinateAtOrigin()
{
    Viewer3DGeoCoordinateType geo;

    QGeoCoordinate ref(47.3977, 8.5456, 400);
    geo.setGpsRef(ref);
    geo.setCoordinate(ref);

    QVector3D local = geo.localCoordinate();

    // Same point as ref: local x,y should be ~0
    QCOMPARE_FUZZY(local.x(), 0.0f, 0.1f);
    QCOMPARE_FUZZY(local.y(), 0.0f, 0.1f);
    QCOMPARE_FUZZY(local.z(), 400.0f, 0.1f);
}

void GeoCoordinateTypeTest::_testCoordinateBeforeGpsRef()
{
    Viewer3DGeoCoordinateType geo;

    QSignalSpy localSpy(&geo, &Viewer3DGeoCoordinateType::localCoordinateChanged);
    QVERIFY(localSpy.isValid());

    // Set coordinate before gpsRef
    QGeoCoordinate coord(47.3987, 8.5466, 410);
    geo.setCoordinate(coord);

    // localCoordinate should still be computed (with default ref 0,0,0)
    // Setting gpsRef should trigger a recalculation
    QGeoCoordinate ref(47.3977, 8.5456, 400);
    geo.setGpsRef(ref);

    QVector3D local = geo.localCoordinate();
    QVERIFY(qAbs(local.x()) > 0.1f || qAbs(local.y()) > 0.1f);
}

UT_REGISTER_TEST(GeoCoordinateTypeTest, TestLabel::Unit)
