#include "QGCGeoBoundingCubeTest.h"
#include "QGCGeoBoundingCube.h"

#include <QtTest/QTest>
#include <cmath>

void QGCGeoBoundingCubeTest::_initialStateTest()
{
    QGCGeoBoundingCube cube;

    // After construction, cube should be in reset (invalid) state
    QVERIFY(!cube.isValid());
}

void QGCGeoBoundingCubeTest::_resetTest()
{
    // Create a valid cube
    QGeoCoordinate nw(47.0, 8.0, 500.0);
    QGeoCoordinate se(46.0, 9.0, 100.0);
    QGCGeoBoundingCube cube(nw, se);

    QVERIFY(cube.isValid());

    cube.reset();
    QVERIFY(!cube.isValid());
}

void QGCGeoBoundingCubeTest::_isValidTest()
{
    // Invalid: default constructed
    QGCGeoBoundingCube invalid;
    QVERIFY(!invalid.isValid());

    // Valid: proper NW and SE coordinates
    QGeoCoordinate nw(47.0, 8.0, 500.0);  // Northwest
    QGeoCoordinate se(46.0, 9.0, 100.0);  // Southeast
    QGCGeoBoundingCube valid(nw, se);
    QVERIFY(valid.isValid());

    // Invalid: coordinates at boundary values
    QGeoCoordinate boundary1(QGCGeoBoundingCube::MaxSouth, 0.0, 0.0);
    QGeoCoordinate boundary2(0.0, 0.0, 0.0);
    QGCGeoBoundingCube boundary(boundary1, boundary2);
    QVERIFY(!boundary.isValid());
}

void QGCGeoBoundingCubeTest::_centerTest()
{
    // Create a cube around a known center
    QGeoCoordinate nw(48.0, 8.0, 1000.0);
    QGeoCoordinate se(46.0, 10.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    QGeoCoordinate center = cube.center();
    QVERIFY(center.isValid());

    // Center should be at latitude 47, longitude 9, altitude 500
    QCOMPARE_EQ(center.latitude(), 47.0);
    QCOMPARE_EQ(center.longitude(), 9.0);
    QCOMPARE_EQ(center.altitude(), 500.0);
}

void QGCGeoBoundingCubeTest::_widthHeightTest()
{
    QGeoCoordinate nw(47.5, 8.0, 100.0);
    QGeoCoordinate se(47.0, 9.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    double width = cube.width();
    double height = cube.height();

    // Width and height should be positive
    QVERIFY(width > 0);
    QVERIFY(height > 0);

    // At ~47 degrees latitude, 1 degree longitude is ~75km
    // 1 degree latitude is ~111km
    // Width (longitude difference of 1 degree) should be roughly 75km
    QVERIFY(width > 70000 && width < 80000);
    // Height (latitude difference of 0.5 degree) should be roughly 55km
    QVERIFY(height > 50000 && height < 60000);
}

void QGCGeoBoundingCubeTest::_areaTest()
{
    QGeoCoordinate nw(47.5, 8.0, 100.0);
    QGeoCoordinate se(47.0, 9.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    double area = cube.area();  // Returns km^2

    // Area should be positive
    QVERIFY(area > 0);

    // Area = width * height (in km^2)
    double widthKm = cube.width() / 1000.0;
    double heightKm = cube.height() / 1000.0;
    QCOMPARE_EQ(area, widthKm * heightKm);
}

void QGCGeoBoundingCubeTest::_radiusTest()
{
    QGeoCoordinate nw(47.5, 8.0, 100.0);
    QGeoCoordinate se(47.0, 9.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    double radius = cube.radius();

    // Radius should be half the diagonal distance
    double diagonal = nw.distanceTo(se);
    QCOMPARE_EQ(radius, diagonal / 2.0);
}

void QGCGeoBoundingCubeTest::_polygon2DTest()
{
    QGeoCoordinate nw(48.0, 8.0, 100.0);
    QGeoCoordinate se(47.0, 9.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    QList<QGeoCoordinate> polygon = cube.polygon2D();

    // Should return 5 coordinates (closed polygon)
    QCOMPARE(polygon.size(), 5);

    // First and last should be the same (closed)
    QCOMPARE(polygon.first().latitude(), polygon.last().latitude());
    QCOMPARE(polygon.first().longitude(), polygon.last().longitude());

    // Check corner coordinates
    QCOMPARE_EQ(polygon[0].latitude(), 48.0);  // NW
    QCOMPARE_EQ(polygon[0].longitude(), 8.0);
    QCOMPARE_EQ(polygon[1].latitude(), 48.0);  // NE
    QCOMPARE_EQ(polygon[1].longitude(), 9.0);
    QCOMPARE_EQ(polygon[2].latitude(), 47.0);  // SE
    QCOMPARE_EQ(polygon[2].longitude(), 9.0);
    QCOMPARE_EQ(polygon[3].latitude(), 47.0);  // SW
    QCOMPARE_EQ(polygon[3].longitude(), 8.0);
}

void QGCGeoBoundingCubeTest::_polygon2DClippedTest()
{
    // Create a large bounding cube
    QGeoCoordinate nw(50.0, 5.0, 100.0);
    QGeoCoordinate se(40.0, 15.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    double originalArea = cube.area();

    // Clip to a smaller area (e.g., 100 km^2)
    QList<QGeoCoordinate> clipped = cube.polygon2D(100.0);

    // Should still return 5 coordinates
    QCOMPARE(clipped.size(), 5);

    // Original area should be larger than clip target
    QVERIFY(originalArea > 100.0);
}

void QGCGeoBoundingCubeTest::_equalityOperatorTest()
{
    QGeoCoordinate nw(47.0, 8.0, 100.0);
    QGeoCoordinate se(46.0, 9.0, 0.0);

    QGCGeoBoundingCube cube1(nw, se);
    QGCGeoBoundingCube cube2(nw, se);
    QGCGeoBoundingCube cube3(nw, QGeoCoordinate(45.0, 10.0, 0.0));

    QVERIFY(cube1 == cube2);
    QVERIFY(!(cube1 == cube3));
    QVERIFY(cube1 != cube3);
}

void QGCGeoBoundingCubeTest::_assignmentOperatorTest()
{
    QGeoCoordinate nw(47.0, 8.0, 100.0);
    QGeoCoordinate se(46.0, 9.0, 0.0);

    QGCGeoBoundingCube cube1(nw, se);
    QGCGeoBoundingCube cube2;

    cube2 = cube1;

    QVERIFY(cube2 == cube1);
    QCOMPARE_EQ(cube2.pointNW.latitude(), nw.latitude());
    QCOMPARE_EQ(cube2.pointSE.longitude(), se.longitude());
}

void QGCGeoBoundingCubeTest::_coordsEqualityTest()
{
    QGeoCoordinate nw(48.0, 8.0, 100.0);
    QGeoCoordinate se(47.0, 9.0, 0.0);
    QGCGeoBoundingCube cube(nw, se);

    QList<QGeoCoordinate> polygon = cube.polygon2D();

    // Cube should equal its own polygon
    QVERIFY(cube == polygon);

    // Modify polygon and it should not equal
    QList<QGeoCoordinate> modified = polygon;
    modified[0] = QGeoCoordinate(49.0, 8.0, 0.0);
    QVERIFY(!(cube == modified));
}
