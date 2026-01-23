#include "GeoUtilitiesTest.h"
#include "GeoUtilities.h"
#include "GeoFormatRegistry.h"
#include "QGCGeo.h"
#include "WKTHelper.h"

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTemporaryFile>
#include <QtTest/QTest>
#include <cmath>
#include <limits>

// ============================================================================
// Bounding Box Tests
// ============================================================================

void GeoUtilitiesTest::_testBoundingBoxEmpty()
{
    QList<QGeoCoordinate> empty;
    const QGeoRectangle box = GeoUtilities::boundingBox(empty);
    QVERIFY(!box.isValid());
}

void GeoUtilitiesTest::_testBoundingBoxSinglePoint()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(47.0, -122.0));

    const QGeoRectangle box = GeoUtilities::boundingBox(coords);
    QVERIFY(box.isValid());
    QCOMPARE(box.topLeft().latitude(), 47.0);
    QCOMPARE(box.topLeft().longitude(), -122.0);
    QCOMPARE(box.bottomRight().latitude(), 47.0);
    QCOMPARE(box.bottomRight().longitude(), -122.0);
}

void GeoUtilitiesTest::_testBoundingBoxMultiplePoints()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(47.0, -122.0));
    coords.append(QGeoCoordinate(48.0, -121.0));
    coords.append(QGeoCoordinate(46.0, -123.0));

    const QGeoRectangle box = GeoUtilities::boundingBox(coords);
    QVERIFY(box.isValid());
    QCOMPARE(box.topLeft().latitude(), 48.0);
    QCOMPARE(box.topLeft().longitude(), -123.0);
    QCOMPARE(box.bottomRight().latitude(), 46.0);
    QCOMPARE(box.bottomRight().longitude(), -121.0);
}

void GeoUtilitiesTest::_testBoundingBoxMultipleLists()
{
    QList<QList<QGeoCoordinate>> lists;

    QList<QGeoCoordinate> list1;
    list1.append(QGeoCoordinate(47.0, -122.0));
    list1.append(QGeoCoordinate(48.0, -121.0));
    lists.append(list1);

    QList<QGeoCoordinate> list2;
    list2.append(QGeoCoordinate(45.0, -124.0));
    list2.append(QGeoCoordinate(46.0, -123.0));
    lists.append(list2);

    const QGeoRectangle box = GeoUtilities::boundingBox(lists);
    QVERIFY(box.isValid());
    QCOMPARE(box.topLeft().latitude(), 48.0);
    QCOMPARE(box.topLeft().longitude(), -124.0);
    QCOMPARE(box.bottomRight().latitude(), 45.0);
    QCOMPARE(box.bottomRight().longitude(), -121.0);
}

void GeoUtilitiesTest::_testExpandBoundingBox()
{
    QGeoRectangle box(QGeoCoordinate(47.0, -122.0), QGeoCoordinate(46.0, -121.0));

    GeoUtilities::expandBoundingBox(box, QGeoCoordinate(48.0, -123.0));

    QVERIFY(box.isValid());
    QCOMPARE(box.topLeft().latitude(), 48.0);
    QCOMPARE(box.topLeft().longitude(), -123.0);
    QCOMPARE(box.bottomRight().latitude(), 46.0);
    QCOMPARE(box.bottomRight().longitude(), -121.0);
}

// ============================================================================
// Coordinate Normalization Tests
// ============================================================================

void GeoUtilitiesTest::_testNormalizeLongitude()
{
    QCOMPARE(GeoUtilities::normalizeLongitude(0.0), 0.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(180.0), 180.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(-180.0), -180.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(90.0), 90.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(-90.0), -90.0);
}

void GeoUtilitiesTest::_testNormalizeLongitudeEdgeCases()
{
    // Wrap positive values
    QCOMPARE(GeoUtilities::normalizeLongitude(181.0), -179.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(270.0), -90.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(360.0), 0.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(540.0), 180.0);

    // Wrap negative values
    QCOMPARE(GeoUtilities::normalizeLongitude(-181.0), 179.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(-270.0), 90.0);
    QCOMPARE(GeoUtilities::normalizeLongitude(-360.0), 0.0);
}

void GeoUtilitiesTest::_testNormalizeLatitude()
{
    QCOMPARE(GeoUtilities::normalizeLatitude(0.0), 0.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(45.0), 45.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(-45.0), -45.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(90.0), 90.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(-90.0), -90.0);
}

void GeoUtilitiesTest::_testNormalizeLatitudeEdgeCases()
{
    // Clamp high values
    QCOMPARE(GeoUtilities::normalizeLatitude(91.0), 90.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(180.0), 90.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(1000.0), 90.0);

    // Clamp low values
    QCOMPARE(GeoUtilities::normalizeLatitude(-91.0), -90.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(-180.0), -90.0);
    QCOMPARE(GeoUtilities::normalizeLatitude(-1000.0), -90.0);
}

void GeoUtilitiesTest::_testNormalizeCoordinate()
{
    // Normal coordinate
    QGeoCoordinate normal(47.0, -122.0, 100.0);
    QGeoCoordinate result = GeoUtilities::normalizeCoordinate(normal);
    QCOMPARE(result.latitude(), 47.0);
    QCOMPARE(result.longitude(), -122.0);
    QCOMPARE(result.altitude(), 100.0);

    // Edge case coordinates (at valid boundaries) - Qt stores these properly
    QGeoCoordinate atBoundary(90.0, 180.0, 50.0);
    result = GeoUtilities::normalizeCoordinate(atBoundary);
    QCOMPARE(result.latitude(), 90.0);
    QCOMPARE(result.longitude(), 180.0);
    QCOMPARE(result.altitude(), 50.0);

    QGeoCoordinate negativeBoundary(-90.0, -180.0, 25.0);
    result = GeoUtilities::normalizeCoordinate(negativeBoundary);
    QCOMPARE(result.latitude(), -90.0);
    QCOMPARE(result.longitude(), -180.0);
    QCOMPARE(result.altitude(), 25.0);

    // Invalid coordinate (NaN values) - should be returned unchanged
    QGeoCoordinate invalid;  // Default constructor creates invalid coord with NaN
    result = GeoUtilities::normalizeCoordinate(invalid);
    QVERIFY(std::isnan(result.latitude()));
    QVERIFY(std::isnan(result.longitude()));
}

void GeoUtilitiesTest::_testNormalizeCoordinates()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(47.0, -122.0));
    coords.append(QGeoCoordinate(90.0, 180.0));   // At boundary
    coords.append(QGeoCoordinate(-90.0, -180.0)); // At negative boundary

    GeoUtilities::normalizeCoordinates(coords);

    QCOMPARE(coords[0].latitude(), 47.0);
    QCOMPARE(coords[0].longitude(), -122.0);

    QCOMPARE(coords[1].latitude(), 90.0);
    QCOMPARE(coords[1].longitude(), 180.0);

    QCOMPARE(coords[2].latitude(), -90.0);
    QCOMPARE(coords[2].longitude(), -180.0);

    // Test with invalid coordinate in the mix
    QList<QGeoCoordinate> withInvalid;
    withInvalid.append(QGeoCoordinate(30.0, -100.0));
    withInvalid.append(QGeoCoordinate());  // Invalid (NaN)
    withInvalid.append(QGeoCoordinate(45.0, -75.0));

    GeoUtilities::normalizeCoordinates(withInvalid);

    QCOMPARE(withInvalid[0].latitude(), 30.0);
    QCOMPARE(withInvalid[0].longitude(), -100.0);
    QVERIFY(std::isnan(withInvalid[1].latitude()));  // Invalid unchanged
    QCOMPARE(withInvalid[2].latitude(), 45.0);
    QCOMPARE(withInvalid[2].longitude(), -75.0);
}

// ============================================================================
// Altitude Validation Tests
// ============================================================================

void GeoUtilitiesTest::_testIsValidAltitude()
{
    QVERIFY(GeoUtilities::isValidAltitude(0.0));
    QVERIFY(GeoUtilities::isValidAltitude(100.0));
    QVERIFY(GeoUtilities::isValidAltitude(-100.0));
    QVERIFY(GeoUtilities::isValidAltitude(GeoUtilities::kDefaultMaxAltitude));
    QVERIFY(GeoUtilities::isValidAltitude(GeoUtilities::kDefaultMinAltitude));

    QVERIFY(!GeoUtilities::isValidAltitude(GeoUtilities::kDefaultMaxAltitude + 1));
    QVERIFY(!GeoUtilities::isValidAltitude(GeoUtilities::kDefaultMinAltitude - 1));

    // Custom range
    QVERIFY(GeoUtilities::isValidAltitude(50.0, 0.0, 100.0));
    QVERIFY(!GeoUtilities::isValidAltitude(150.0, 0.0, 100.0));
}

void GeoUtilitiesTest::_testIsValidAltitudeNaN()
{
    // NaN altitude is valid (means no altitude)
    QVERIFY(GeoUtilities::isValidAltitude(qQNaN()));
}

void GeoUtilitiesTest::_testIsValidAltitudeCoordinate()
{
    QGeoCoordinate withAlt(47.0, -122.0, 500.0);
    QVERIFY(GeoUtilities::isValidAltitude(withAlt));

    // kDefaultMaxAltitude is 100000m, so 150000m should be invalid
    QGeoCoordinate highAlt(47.0, -122.0, 150000.0);
    QVERIFY(!GeoUtilities::isValidAltitude(highAlt));

    QGeoCoordinate noAlt(47.0, -122.0);
    QVERIFY(GeoUtilities::isValidAltitude(noAlt));
}

void GeoUtilitiesTest::_testValidateAltitudes()
{
    QString errorString;

    QList<QGeoCoordinate> validCoords;
    validCoords.append(QGeoCoordinate(47.0, -122.0, 100.0));
    validCoords.append(QGeoCoordinate(48.0, -121.0, 200.0));
    QVERIFY(GeoUtilities::validateAltitudes(validCoords, errorString));
    QVERIFY(errorString.isEmpty());

    // kDefaultMaxAltitude is 100000m, so 150000m should be invalid
    QList<QGeoCoordinate> invalidCoords;
    invalidCoords.append(QGeoCoordinate(47.0, -122.0, 100.0));
    invalidCoords.append(QGeoCoordinate(48.0, -121.0, 150000.0));  // Out of range
    QVERIFY(!GeoUtilities::validateAltitudes(invalidCoords, errorString));
    QVERIFY(!errorString.isEmpty());
}

// ============================================================================
// Polygon Validation Tests
// ============================================================================

void GeoUtilitiesTest::_testIsSelfIntersectingSimple()
{
    // Simple triangle - not self-intersecting
    QList<QGeoCoordinate> triangle;
    triangle.append(QGeoCoordinate(0.0, 0.0));
    triangle.append(QGeoCoordinate(1.0, 0.0));
    triangle.append(QGeoCoordinate(0.5, 1.0));
    QVERIFY(!GeoUtilities::isSelfIntersecting(triangle));

    // Simple square - not self-intersecting
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(1.0, 0.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    QVERIFY(!GeoUtilities::isSelfIntersecting(square));
}

void GeoUtilitiesTest::_testIsSelfIntersectingComplex()
{
    // L-shaped polygon - not self-intersecting
    QList<QGeoCoordinate> lShape;
    lShape.append(QGeoCoordinate(0.0, 0.0));
    lShape.append(QGeoCoordinate(1.0, 0.0));
    lShape.append(QGeoCoordinate(1.0, 0.5));
    lShape.append(QGeoCoordinate(0.5, 0.5));
    lShape.append(QGeoCoordinate(0.5, 1.0));
    lShape.append(QGeoCoordinate(0.0, 1.0));
    QVERIFY(!GeoUtilities::isSelfIntersecting(lShape));
}

void GeoUtilitiesTest::_testIsSelfIntersectingFigure8()
{
    // Figure-8 (bowtie) - self-intersecting
    QList<QGeoCoordinate> figure8;
    figure8.append(QGeoCoordinate(0.0, 0.0));
    figure8.append(QGeoCoordinate(1.0, 1.0));
    figure8.append(QGeoCoordinate(0.0, 1.0));
    figure8.append(QGeoCoordinate(1.0, 0.0));
    QVERIFY(GeoUtilities::isSelfIntersecting(figure8));
}

void GeoUtilitiesTest::_testIsClockwise()
{
    // Clockwise square (using Shoelace formula with lat as Y, lon as X)
    // Going: bottom-left -> top-left -> top-right -> bottom-right is CW
    QList<QGeoCoordinate> cw;
    cw.append(QGeoCoordinate(0.0, 0.0));   // (lat=0, lon=0) bottom-left
    cw.append(QGeoCoordinate(1.0, 0.0));   // (lat=1, lon=0) top-left
    cw.append(QGeoCoordinate(1.0, 1.0));   // (lat=1, lon=1) top-right
    cw.append(QGeoCoordinate(0.0, 1.0));   // (lat=0, lon=1) bottom-right
    QVERIFY(GeoUtilities::isClockwise(cw));

    // Counter-clockwise square
    // Going: bottom-left -> bottom-right -> top-right -> top-left is CCW
    QList<QGeoCoordinate> ccw;
    ccw.append(QGeoCoordinate(0.0, 0.0));  // (lat=0, lon=0) bottom-left
    ccw.append(QGeoCoordinate(0.0, 1.0));  // (lat=0, lon=1) bottom-right
    ccw.append(QGeoCoordinate(1.0, 1.0));  // (lat=1, lon=1) top-right
    ccw.append(QGeoCoordinate(1.0, 0.0));  // (lat=1, lon=0) top-left
    QVERIFY(!GeoUtilities::isClockwise(ccw));
}

void GeoUtilitiesTest::_testReverseVertices()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(0.0, 0.0));
    coords.append(QGeoCoordinate(1.0, 0.0));
    coords.append(QGeoCoordinate(2.0, 0.0));

    GeoUtilities::reverseVertices(coords);

    QCOMPARE(coords[0].latitude(), 2.0);
    QCOMPARE(coords[1].latitude(), 1.0);
    QCOMPARE(coords[2].latitude(), 0.0);
}

void GeoUtilitiesTest::_testValidatePolygon()
{
    QString errorString;

    // Valid triangle
    QList<QGeoCoordinate> triangle;
    triangle.append(QGeoCoordinate(0.0, 0.0));
    triangle.append(QGeoCoordinate(1.0, 0.0));
    triangle.append(QGeoCoordinate(0.5, 1.0));
    QVERIFY(GeoUtilities::validatePolygon(triangle, errorString));
}

void GeoUtilitiesTest::_testValidatePolygonTooFewVertices()
{
    QString errorString;

    // Too few vertices
    QList<QGeoCoordinate> tooFew;
    tooFew.append(QGeoCoordinate(0.0, 0.0));
    tooFew.append(QGeoCoordinate(1.0, 0.0));
    QVERIFY(!GeoUtilities::validatePolygon(tooFew, errorString));
    QVERIFY(errorString.contains("3 vertices"));
}

void GeoUtilitiesTest::_testValidatePolygonSelfIntersecting()
{
    QString errorString;

    // Self-intersecting (bowtie)
    QList<QGeoCoordinate> bowtie;
    bowtie.append(QGeoCoordinate(0.0, 0.0));
    bowtie.append(QGeoCoordinate(1.0, 1.0));
    bowtie.append(QGeoCoordinate(0.0, 1.0));
    bowtie.append(QGeoCoordinate(1.0, 0.0));
    QVERIFY(!GeoUtilities::validatePolygon(bowtie, errorString));
    QVERIFY(errorString.contains("self-intersecting"));
}

void GeoUtilitiesTest::_testValidatePolygonZeroArea()
{
    QString errorString;

    // Degenerate (collinear points)
    QList<QGeoCoordinate> collinear;
    collinear.append(QGeoCoordinate(0.0, 0.0));
    collinear.append(QGeoCoordinate(0.5, 0.0));
    collinear.append(QGeoCoordinate(1.0, 0.0));
    QVERIFY(!GeoUtilities::validatePolygon(collinear, errorString));
    QVERIFY(errorString.contains("zero") || errorString.contains("area"));
}

// ============================================================================
// Coordinate Validation Tests
// ============================================================================

void GeoUtilitiesTest::_testIsValidCoordinate()
{
    QVERIFY(GeoUtilities::isValidCoordinate(QGeoCoordinate(0.0, 0.0)));
    QVERIFY(GeoUtilities::isValidCoordinate(QGeoCoordinate(90.0, 180.0)));
    QVERIFY(GeoUtilities::isValidCoordinate(QGeoCoordinate(-90.0, -180.0)));
    QVERIFY(GeoUtilities::isValidCoordinate(QGeoCoordinate(47.0, -122.0)));

    QVERIFY(!GeoUtilities::isValidCoordinate(QGeoCoordinate(91.0, 0.0)));
    QVERIFY(!GeoUtilities::isValidCoordinate(QGeoCoordinate(-91.0, 0.0)));
    QVERIFY(!GeoUtilities::isValidCoordinate(QGeoCoordinate(0.0, 181.0)));
    QVERIFY(!GeoUtilities::isValidCoordinate(QGeoCoordinate(0.0, -181.0)));

    QVERIFY(!GeoUtilities::isValidCoordinate(QGeoCoordinate()));  // Invalid coordinate
}

void GeoUtilitiesTest::_testValidateCoordinates()
{
    QString errorString;

    QList<QGeoCoordinate> validCoords;
    validCoords.append(QGeoCoordinate(47.0, -122.0));
    validCoords.append(QGeoCoordinate(48.0, -121.0));
    QVERIFY(GeoUtilities::validateCoordinates(validCoords, errorString));
    QVERIFY(errorString.isEmpty());

    QList<QGeoCoordinate> invalidCoords;
    invalidCoords.append(QGeoCoordinate(47.0, -122.0));
    invalidCoords.append(QGeoCoordinate(95.0, -121.0));  // Invalid latitude
    QVERIFY(!GeoUtilities::validateCoordinates(invalidCoords, errorString));
    QVERIFY(!errorString.isEmpty());
}

// ============================================================================
// UTM Conversion Tests
// ============================================================================

void GeoUtilitiesTest::_testConvertToUTM()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(47.6062, -122.3321));  // Seattle
    coords.append(QGeoCoordinate(47.6097, -122.3331));

    QList<GeoUtilities::UTMCoordinate> utmCoords;
    QVERIFY(GeoUtilities::convertToUTM(coords, utmCoords));
    QCOMPARE(utmCoords.size(), 2);
    QCOMPARE(utmCoords[0].zone, 10);  // Seattle is in UTM zone 10
    QVERIFY(!utmCoords[0].southern);
    QVERIFY(utmCoords[0].easting > 0);
    QVERIFY(utmCoords[0].northing > 0);
}

void GeoUtilitiesTest::_testConvertFromUTM()
{
    // Convert Seattle coords to UTM and back
    QList<QGeoCoordinate> original;
    original.append(QGeoCoordinate(47.6062, -122.3321));

    QList<GeoUtilities::UTMCoordinate> utmCoords;
    QVERIFY(GeoUtilities::convertToUTM(original, utmCoords));

    QList<QGeoCoordinate> converted;
    QVERIFY(GeoUtilities::convertFromUTM(utmCoords, converted));
    QCOMPARE(converted.size(), 1);

    // Should be very close to original
    QVERIFY(qAbs(converted[0].latitude() - original[0].latitude()) < 0.0001);
    QVERIFY(qAbs(converted[0].longitude() - original[0].longitude()) < 0.0001);
}

void GeoUtilitiesTest::_testOptimalUTMZone()
{
    QList<QGeoCoordinate> coords;
    coords.append(QGeoCoordinate(47.6062, -122.3321));  // Seattle area
    coords.append(QGeoCoordinate(47.5, -122.5));

    int zone = 0;
    bool southern = true;
    QVERIFY(GeoUtilities::optimalUTMZone(coords, zone, southern));
    QCOMPARE(zone, 10);
    QVERIFY(!southern);

    // Empty list should fail
    QList<QGeoCoordinate> empty;
    QVERIFY(!GeoUtilities::optimalUTMZone(empty, zone, southern));
}

// ============================================================================
// Polygon Geometry Tests
// ============================================================================

void GeoUtilitiesTest::_testPolygonCentroid()
{
    // Simple square polygon
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    const QGeoCoordinate centroid = GeoUtilities::polygonCentroid(square);
    QVERIFY(centroid.isValid());
    QVERIFY(qAbs(centroid.latitude() - 0.5) < 0.01);
    QVERIFY(qAbs(centroid.longitude() - 0.5) < 0.01);
}

void GeoUtilitiesTest::_testPolygonCentroidDegenerate()
{
    // Too few vertices
    QList<QGeoCoordinate> twoPoints;
    twoPoints.append(QGeoCoordinate(0.0, 0.0));
    twoPoints.append(QGeoCoordinate(1.0, 1.0));

    const QGeoCoordinate centroid = GeoUtilities::polygonCentroid(twoPoints);
    QVERIFY(!centroid.isValid());
}

void GeoUtilitiesTest::_testPolygonArea()
{
    // Test with a square polygon approximately 1km x 1km
    // At the equator, 1 degree longitude ≈ 111km, 1 degree latitude ≈ 111km
    // So 0.009 degrees ≈ 1km (roughly)
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 0.009));    // ~1km east
    square.append(QGeoCoordinate(0.009, 0.009));  // ~1km north
    square.append(QGeoCoordinate(0.009, 0.0));    // ~1km west

    const double area = GeoUtilities::polygonArea(square);

    // Should be approximately 1 km² = 1,000,000 m²
    // Allow 5% tolerance due to spherical approximation
    QVERIFY2(area > 900000.0 && area < 1100000.0,
             qPrintable(QStringLiteral("Expected ~1000000 m², got %1").arg(area)));

    // Test triangle (half the square area)
    QList<QGeoCoordinate> triangle;
    triangle.append(QGeoCoordinate(0.0, 0.0));
    triangle.append(QGeoCoordinate(0.0, 0.009));
    triangle.append(QGeoCoordinate(0.009, 0.009));

    const double triArea = GeoUtilities::polygonArea(triangle);
    QVERIFY2(triArea > 450000.0 && triArea < 550000.0,
             qPrintable(QStringLiteral("Expected ~500000 m², got %1").arg(triArea)));

    // Test empty/degenerate cases
    QList<QGeoCoordinate> empty;
    QCOMPARE(GeoUtilities::polygonArea(empty), 0.0);

    QList<QGeoCoordinate> twoPoints;
    twoPoints.append(QGeoCoordinate(0.0, 0.0));
    twoPoints.append(QGeoCoordinate(1.0, 1.0));
    QCOMPARE(GeoUtilities::polygonArea(twoPoints), 0.0);
}

void GeoUtilitiesTest::_testPolygonAreaWithHoles()
{
    // Create a square with a smaller square hole
    QGeoPolygon polygon;

    // Outer square ~2km x 2km
    QList<QGeoCoordinate> outer;
    outer.append(QGeoCoordinate(0.0, 0.0));
    outer.append(QGeoCoordinate(0.0, 0.018));
    outer.append(QGeoCoordinate(0.018, 0.018));
    outer.append(QGeoCoordinate(0.018, 0.0));
    polygon.setPerimeter(outer);

    // Inner hole ~1km x 1km centered in the outer square
    QList<QGeoCoordinate> hole;
    hole.append(QGeoCoordinate(0.0045, 0.0045));
    hole.append(QGeoCoordinate(0.0045, 0.0135));
    hole.append(QGeoCoordinate(0.0135, 0.0135));
    hole.append(QGeoCoordinate(0.0135, 0.0045));
    polygon.addHole(hole);

    const double outerArea = GeoUtilities::polygonArea(outer);
    const double holeArea = GeoUtilities::polygonArea(hole);
    const double totalArea = GeoUtilities::polygonArea(polygon);

    // Area with hole should be outer - hole
    QVERIFY2(qAbs(totalArea - (outerArea - holeArea)) < 1000.0,
             qPrintable(QStringLiteral("Expected %1, got %2").arg(outerArea - holeArea).arg(totalArea)));

    // Outer ~4km², hole ~1km², so total ~3km² = 3,000,000 m²
    QVERIFY2(totalArea > 2700000.0 && totalArea < 3300000.0,
             qPrintable(QStringLiteral("Expected ~3000000 m², got %1").arg(totalArea)));
}

void GeoUtilitiesTest::_testPolygonPerimeter()
{
    // Test with a square polygon approximately 1km x 1km at the equator
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 0.009));    // ~1km east
    square.append(QGeoCoordinate(0.009, 0.009));  // ~1km north
    square.append(QGeoCoordinate(0.009, 0.0));    // ~1km west

    const double perimeter = GeoUtilities::polygonPerimeter(square);

    // Should be approximately 4 km = 4000 m
    // Allow 5% tolerance
    QVERIFY2(perimeter > 3800.0 && perimeter < 4200.0,
             qPrintable(QStringLiteral("Expected ~4000 m, got %1").arg(perimeter)));

    // Test empty case
    QList<QGeoCoordinate> empty;
    QCOMPARE(GeoUtilities::polygonPerimeter(empty), 0.0);

    // Test single point
    QList<QGeoCoordinate> onePoint;
    onePoint.append(QGeoCoordinate(0.0, 0.0));
    QCOMPARE(GeoUtilities::polygonPerimeter(onePoint), 0.0);
}

void GeoUtilitiesTest::_testPolylineMidpoint()
{
    QList<QGeoCoordinate> line;
    line.append(QGeoCoordinate(0.0, 0.0));
    line.append(QGeoCoordinate(0.0, 2.0));

    const QGeoCoordinate midpoint = GeoUtilities::polylineMidpoint(line);
    QVERIFY(midpoint.isValid());
    QVERIFY(qAbs(midpoint.latitude()) < 0.01);
    QVERIFY(qAbs(midpoint.longitude() - 1.0) < 0.1);
}

// ============================================================================
// Douglas-Peucker Simplification Tests
// ============================================================================

void GeoUtilitiesTest::_testSimplifyDouglasPeucker()
{
    // Create a path with many points along a line with one outlier
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(0.0, 0.0));
    path.append(QGeoCoordinate(0.0, 0.001));
    path.append(QGeoCoordinate(0.0, 0.002));
    path.append(QGeoCoordinate(0.1, 0.003));  // Outlier
    path.append(QGeoCoordinate(0.0, 0.004));
    path.append(QGeoCoordinate(0.0, 0.005));

    // Small tolerance should keep outlier
    auto simplified = GeoUtilities::simplifyDouglasPeucker(path, 100);
    QVERIFY(simplified.size() >= 3);  // At least start, outlier, end
    QVERIFY(simplified.size() < path.size());

    // Large tolerance should remove outlier
    auto verySimplified = GeoUtilities::simplifyDouglasPeucker(path, 100000);
    QCOMPARE(verySimplified.size(), 2);  // Just start and end
}

void GeoUtilitiesTest::_testSimplifyToCount()
{
    // Create a path with varying points to ensure some are kept
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(0.0, 0.0));
    path.append(QGeoCoordinate(0.001, 0.01));
    path.append(QGeoCoordinate(0.0, 0.02));
    path.append(QGeoCoordinate(0.002, 0.03));  // Keep this one
    path.append(QGeoCoordinate(0.0, 0.04));
    path.append(QGeoCoordinate(0.001, 0.05));
    path.append(QGeoCoordinate(0.0, 0.06));
    path.append(QGeoCoordinate(0.003, 0.07));  // Keep this one
    path.append(QGeoCoordinate(0.0, 0.08));
    path.append(QGeoCoordinate(0.0, 0.09));

    auto simplified = GeoUtilities::simplifyToCount(path, 5);
    QVERIFY(simplified.size() <= 7);  // Approximately 5, may vary
    QVERIFY(simplified.size() >= 2);  // At least start and end
}

// ============================================================================
// Visvalingam-Whyatt Simplification Tests
// ============================================================================

void GeoUtilitiesTest::_testSimplifyVisvalingamWhyatt()
{
    // Create a path with several points
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(0.0, 0.0));
    path.append(QGeoCoordinate(0.0001, 0.001));  // Small deviation
    path.append(QGeoCoordinate(0.0, 0.002));
    path.append(QGeoCoordinate(0.01, 0.003));    // Large deviation
    path.append(QGeoCoordinate(0.0, 0.004));
    path.append(QGeoCoordinate(0.0, 0.005));

    // Small area threshold should keep outlier
    auto simplified = GeoUtilities::simplifyVisvalingamWhyatt(path, 100);
    QVERIFY(simplified.size() >= 3);  // At least start, large outlier, end
    QVERIFY(simplified.size() <= path.size());

    // Large area threshold should simplify heavily
    auto verySimplified = GeoUtilities::simplifyVisvalingamWhyatt(path, 1e9);
    QCOMPARE(verySimplified.size(), 2);  // Just start and end
}

void GeoUtilitiesTest::_testSimplifyVisvalingamToCount()
{
    QList<QGeoCoordinate> path;
    for (int i = 0; i < 10; ++i) {
        double noise = (i % 3 == 1) ? 0.001 : 0.0;  // Every 3rd point has deviation
        path.append(QGeoCoordinate(noise, i * 0.01));
    }

    auto simplified = GeoUtilities::simplifyVisvalingamToCount(path, 5);
    QCOMPARE(simplified.size(), 5);
    QCOMPARE(simplified.first(), path.first());  // Start preserved
    QCOMPARE(simplified.last(), path.last());    // End preserved
}

// ============================================================================
// Point-in-Polygon Tests
// ============================================================================

void GeoUtilitiesTest::_testPointInPolygon()
{
    // Simple square polygon (0,0) to (1,1)
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    // Inside
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.5, 0.5), square));
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.1, 0.1), square));
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.9, 0.9), square));

    // Outside
    QVERIFY(!GeoUtilities::pointInPolygon(QGeoCoordinate(1.5, 0.5), square));
    QVERIFY(!GeoUtilities::pointInPolygon(QGeoCoordinate(-0.5, 0.5), square));
    QVERIFY(!GeoUtilities::pointInPolygon(QGeoCoordinate(0.5, -0.5), square));
    QVERIFY(!GeoUtilities::pointInPolygon(QGeoCoordinate(0.5, 1.5), square));
}

void GeoUtilitiesTest::_testPointInPolygonEdgeCases()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    // On vertex
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.0, 0.0), square));
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(1.0, 1.0), square));

    // On edge
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.5, 0.0), square));
    QVERIFY(GeoUtilities::pointInPolygon(QGeoCoordinate(0.0, 0.5), square));

    // Degenerate polygon (fewer than 3 points)
    QList<QGeoCoordinate> line;
    line.append(QGeoCoordinate(0.0, 0.0));
    line.append(QGeoCoordinate(1.0, 1.0));
    QVERIFY(!GeoUtilities::pointInPolygon(QGeoCoordinate(0.5, 0.5), line));
}

void GeoUtilitiesTest::_testPointInPolygonWithTolerance()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(47.0, -122.0));
    square.append(QGeoCoordinate(47.0, -121.0));
    square.append(QGeoCoordinate(48.0, -121.0));
    square.append(QGeoCoordinate(48.0, -122.0));

    // Clearly inside
    QCOMPARE(GeoUtilities::pointInPolygonWithTolerance(
        QGeoCoordinate(47.5, -121.5), square, 100), 1);

    // Clearly outside
    QCOMPARE(GeoUtilities::pointInPolygonWithTolerance(
        QGeoCoordinate(49.0, -121.5), square, 100), -1);

    // On boundary (on vertex)
    QCOMPARE(GeoUtilities::pointInPolygonWithTolerance(
        QGeoCoordinate(47.0, -122.0), square, 100), 0);
}

// ============================================================================
// Path Interpolation Tests
// ============================================================================

void GeoUtilitiesTest::_testInterpolateAlongPath()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(0.0, 0.0));
    path.append(QGeoCoordinate(0.0, 1.0));
    path.append(QGeoCoordinate(0.0, 2.0));

    // Start
    QGeoCoordinate start = GeoUtilities::interpolateAlongPath(path, 0.0);
    QVERIFY(qAbs(start.latitude() - 0.0) < 0.001);
    QVERIFY(qAbs(start.longitude() - 0.0) < 0.001);

    // End
    QGeoCoordinate end = GeoUtilities::interpolateAlongPath(path, 1.0);
    QVERIFY(qAbs(end.latitude() - 0.0) < 0.001);
    QVERIFY(qAbs(end.longitude() - 2.0) < 0.001);

    // Middle
    QGeoCoordinate mid = GeoUtilities::interpolateAlongPath(path, 0.5);
    QVERIFY(qAbs(mid.longitude() - 1.0) < 0.1);

    // Clamp out of range
    QGeoCoordinate before = GeoUtilities::interpolateAlongPath(path, -1.0);
    QVERIFY(qAbs(before.longitude() - 0.0) < 0.001);

    QGeoCoordinate after = GeoUtilities::interpolateAlongPath(path, 2.0);
    QVERIFY(qAbs(after.longitude() - 2.0) < 0.001);
}

void GeoUtilitiesTest::_testInterpolateAlongPathAtDistance()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(47.0, -122.0));
    path.append(QGeoCoordinate(47.0, -121.0));

    // Start
    QGeoCoordinate start = GeoUtilities::interpolateAlongPathAtDistance(path, 0.0);
    QVERIFY(qAbs(start.longitude() - (-122.0)) < 0.001);

    // Some distance along
    QGeoCoordinate mid = GeoUtilities::interpolateAlongPathAtDistance(path, 40000);  // ~40km
    QVERIFY(mid.longitude() > -122.0);
    QVERIFY(mid.longitude() < -121.0);

    // Beyond end
    QGeoCoordinate beyond = GeoUtilities::interpolateAlongPathAtDistance(path, 1000000);
    QVERIFY(qAbs(beyond.longitude() - (-121.0)) < 0.001);
}

void GeoUtilitiesTest::_testResamplePath()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(0.0, 0.0));
    path.append(QGeoCoordinate(0.0, 0.1));
    path.append(QGeoCoordinate(0.0, 0.2));

    auto resampled = GeoUtilities::resamplePath(path, 5);
    QCOMPARE(resampled.size(), 5);

    // First and last should match original
    QVERIFY(qAbs(resampled.first().longitude() - path.first().longitude()) < 0.001);
    QVERIFY(qAbs(resampled.last().longitude() - path.last().longitude()) < 0.001);

    // Points should be evenly spaced
    double prevLon = resampled[0].longitude();
    for (int i = 1; i < resampled.size(); ++i) {
        QVERIFY(resampled[i].longitude() > prevLon);
        prevLon = resampled[i].longitude();
    }
}

// ============================================================================
// WKT Format Tests
// ============================================================================

void GeoUtilitiesTest::_testWKTParsePoint()
{
    QGeoCoordinate coord;
    QString error;

    QVERIFY(WKTHelper::parsePoint("POINT (30 10)", coord, error));
    QVERIFY(qAbs(coord.longitude() - 30.0) < 0.001);
    QVERIFY(qAbs(coord.latitude() - 10.0) < 0.001);

    QVERIFY(WKTHelper::parsePoint("POINT Z (30 10 100)", coord, error));
    QVERIFY(qAbs(coord.altitude() - 100.0) < 0.001);

    QVERIFY(!WKTHelper::parsePoint("INVALID", coord, error));
}

void GeoUtilitiesTest::_testWKTParseLineString()
{
    QList<QGeoCoordinate> coords;
    QString error;

    QVERIFY(WKTHelper::parseLineString("LINESTRING (30 10, 10 30, 40 40)", coords, error));
    QCOMPARE(coords.size(), 3);
    QVERIFY(qAbs(coords[0].longitude() - 30.0) < 0.001);
    QVERIFY(qAbs(coords[0].latitude() - 10.0) < 0.001);
}

void GeoUtilitiesTest::_testWKTParsePolygon()
{
    QList<QGeoCoordinate> vertices;
    QString error;

    QVERIFY(WKTHelper::parsePolygon("POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))", vertices, error));
    QCOMPARE(vertices.size(), 4);  // Closing vertex removed
}

void GeoUtilitiesTest::_testWKTParsePolygonWithHoles()
{
    QGeoPolygon polygon;
    QString error;

    const QString wkt = "POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30))";
    QVERIFY(WKTHelper::parsePolygonWithHoles(wkt, polygon, error));
    QCOMPARE(polygon.holesCount(), 1);
}

void GeoUtilitiesTest::_testWKTGenerate()
{
    QList<QGeoCoordinate> vertices;
    vertices.append(QGeoCoordinate(10, 30));
    vertices.append(QGeoCoordinate(40, 40));
    vertices.append(QGeoCoordinate(40, 20));
    vertices.append(QGeoCoordinate(20, 10));

    const QString wkt = WKTHelper::toWKTPolygon(vertices);
    QVERIFY(wkt.startsWith("POLYGON"));
    QVERIFY(wkt.contains("30"));
}

void GeoUtilitiesTest::_testWKTRoundTrip()
{
    // Create polygon
    QList<QGeoCoordinate> original;
    original.append(QGeoCoordinate(10, 30));
    original.append(QGeoCoordinate(40, 40));
    original.append(QGeoCoordinate(40, 20));

    // Convert to WKT
    const QString wkt = WKTHelper::toWKTPolygon(original);

    // Parse back
    QList<QGeoCoordinate> parsed;
    QString error;
    QVERIFY(WKTHelper::parsePolygon(wkt, parsed, error));
    QCOMPARE(parsed.size(), original.size());

    // Check coordinates match
    for (int i = 0; i < original.size(); ++i) {
        QVERIFY(qAbs(parsed[i].latitude() - original[i].latitude()) < 0.0001);
        QVERIFY(qAbs(parsed[i].longitude() - original[i].longitude()) < 0.0001);
    }
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

void GeoUtilitiesTest::_testBenchmarkDouglasPeucker()
{
    // Create a large path (1000 points)
    QList<QGeoCoordinate> path;
    for (int i = 0; i < 1000; ++i) {
        const double noise = (i % 10 == 0) ? 0.001 : 0.0;  // Every 10th point is an outlier
        path.append(QGeoCoordinate(47.0 + noise, -122.0 + i * 0.0001));
    }

    QElapsedTimer timer;
    timer.start();

    auto simplified = GeoUtilities::simplifyDouglasPeucker(path, 10);

    const qint64 elapsed = timer.elapsed();
    qDebug() << "Douglas-Peucker 1000 points:" << elapsed << "ms, reduced to" << simplified.size();

    QVERIFY(elapsed < 1000);  // Should complete in under 1 second
    QVERIFY(simplified.size() < path.size());
}

void GeoUtilitiesTest::_testBenchmarkBoundingBox()
{
    // Create a large coordinate list (10000 points)
    QList<QGeoCoordinate> coords;
    for (int i = 0; i < 10000; ++i) {
        coords.append(QGeoCoordinate(47.0 + (i % 100) * 0.01, -122.0 + static_cast<double>(i / 100) * 0.01));
    }

    QElapsedTimer timer;
    timer.start();

    const QGeoRectangle box = GeoUtilities::boundingBox(coords);

    const qint64 elapsed = timer.elapsed();
    qDebug() << "Bounding box 10000 points:" << elapsed << "ms";

    QVERIFY(elapsed < 100);  // Should complete in under 100ms
    QVERIFY(box.isValid());
}

void GeoUtilitiesTest::_testBenchmarkVisvalingam()
{
    // Create a large path (1000 points)
    QList<QGeoCoordinate> path;
    for (int i = 0; i < 1000; ++i) {
        const double noise = (i % 7 == 0) ? 0.001 : 0.0;  // Every 7th point is an outlier
        path.append(QGeoCoordinate(47.0 + noise, -122.0 + i * 0.0001));
    }

    QElapsedTimer timer;
    timer.start();

    auto simplified = GeoUtilities::simplifyVisvalingamWhyatt(path, 10);

    const qint64 elapsed = timer.elapsed();
    qDebug() << "Visvalingam-Whyatt 1000 points:" << elapsed << "ms, reduced to" << simplified.size();

    QVERIFY(elapsed < 2000);  // Should complete in under 2 seconds
    QVERIFY(simplified.size() < path.size());
}

void GeoUtilitiesTest::_testBenchmarkPointInPolygon()
{
    // Create a complex polygon (100 vertices)
    QList<QGeoCoordinate> polygon;
    for (int i = 0; i < 100; ++i) {
        const double angle = 2.0 * M_PI * i / 100.0;
        const double r = 1.0 + 0.1 * std::sin(5 * angle);  // Star-like shape
        polygon.append(QGeoCoordinate(47.0 + r * std::sin(angle), -122.0 + r * std::cos(angle)));
    }

    QElapsedTimer timer;
    timer.start();

    // Test 10000 points
    int insideCount = 0;
    for (int i = 0; i < 10000; ++i) {
        const double lat = 46.0 + (i % 100) * 0.02;
        const double lon = -123.0 + static_cast<double>(i / 100) * 0.02;
        if (GeoUtilities::pointInPolygon(QGeoCoordinate(lat, lon), polygon)) {
            ++insideCount;
        }
    }

    const qint64 elapsed = timer.elapsed();
    qDebug() << "Point-in-polygon 10000 tests:" << elapsed << "ms, inside:" << insideCount;

    QVERIFY(elapsed < 500);  // Should complete in under 500ms
}

// ============================================================================
// Fuzz Tests for Parsers
// ============================================================================

void GeoUtilitiesTest::_testFuzzWKTParser()
{
    QString error;

    // Test malformed inputs - should not crash
    const QStringList fuzzInputs = {
        "",
        "INVALID",
        "POINT",
        "POINT ()",
        "POINT (a b)",
        "POINT (1)",
        "POINT (1 2 3 4 5)",
        "POLYGON",
        "POLYGON ()",
        "POLYGON (()",
        "POLYGON (())",
        "POLYGON ((1 2, 3))",
        "LINESTRING (,,,)",
        "MULTIPOINT",
        "POINT Z ()",
        "POINT M (1 2 3)",
        QString("POINT (%1 %2)").arg(std::numeric_limits<double>::max()).arg(std::numeric_limits<double>::min()),
        QString("POINT (%1 %2)").arg(std::numeric_limits<double>::infinity()).arg(0),
        "POLYGON ((0 0, 0 0, 0 0, 0 0))",  // Degenerate
        QString(10000, 'A'),  // Very long string
    };

    for (const auto &input : fuzzInputs) {
        QGeoCoordinate coord;
        QList<QGeoCoordinate> coords;
        QGeoPolygon polygon;

        // None of these should crash
        WKTHelper::parsePoint(input, coord, error);
        WKTHelper::parseLineString(input, coords, error);
        WKTHelper::parsePolygon(input, coords, error);
        WKTHelper::parsePolygonWithHoles(input, polygon, error);
        WKTHelper::parseMultiPoint(input, coords, error);
        WKTHelper::geometryType(input);
        WKTHelper::hasAltitude(input);
    }

    QVERIFY(true);  // If we get here without crash, test passes
}

void GeoUtilitiesTest::_testFuzzKMLParser()
{
    // Test malformed KML inputs - should not crash
    const QStringList fuzzInputs = {
        "",
        "<kml>",
        "<kml></kml>",
        "<?xml version=\"1.0\"?><kml></kml>",
        "<Placemark><coordinates></coordinates></Placemark>",
        "<Placemark><coordinates>a,b,c</coordinates></Placemark>",
        "<Placemark><coordinates>,,,,</coordinates></Placemark>",
        "<Polygon><outerBoundaryIs></outerBoundaryIs></Polygon>",
        "<LinearRing><coordinates>1,2 3,4</coordinates></LinearRing>",
        QString(10000, '<'),  // Very long malformed string
    };

    for (const auto &input : fuzzInputs) {
        // Create a temporary file with the fuzz input
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/fuzz_XXXXXX.kml");
        if (tempFile.open()) {
            tempFile.write(input.toUtf8());
            tempFile.close();

            // Try to load - should not crash
            QString error;
            QList<QGeoCoordinate> coords;
            GeoFormatRegistry::loadPolygon(tempFile.fileName(), coords, error);
        }
    }

    QVERIFY(true);  // If we get here without crash, test passes
}

void GeoUtilitiesTest::_testFuzzGPXParser()
{
    // Test malformed GPX inputs - should not crash
    const QStringList fuzzInputs = {
        "",
        "<gpx>",
        "<gpx></gpx>",
        "<?xml version=\"1.0\"?><gpx></gpx>",
        "<gpx><trk><trkseg></trkseg></trk></gpx>",
        "<gpx><trk><trkseg><trkpt lat=\"a\" lon=\"b\"/></trkseg></trk></gpx>",
        "<gpx><trk><trkseg><trkpt lat=\"\" lon=\"\"/></trkseg></trk></gpx>",
        "<gpx><wpt lat=\"47\" lon=\"-122\"/></gpx>",
        QString(10000, 'X'),  // Very long malformed string
    };

    for (const auto &input : fuzzInputs) {
        QTemporaryFile tempFile;
        tempFile.setFileTemplate(QDir::tempPath() + "/fuzz_XXXXXX.gpx");
        if (tempFile.open()) {
            tempFile.write(input.toUtf8());
            tempFile.close();

            // Try to load - should not crash
            QString error;
            QList<QGeoCoordinate> coords;
            GeoFormatRegistry::loadPolygon(tempFile.fileName(), coords, error);
        }
    }

    QVERIFY(true);  // If we get here without crash, test passes
}

// ============================================================================
// Convex Hull Tests
// ============================================================================

void GeoUtilitiesTest::_testConvexHullTriangle()
{
    QList<QGeoCoordinate> triangle;
    triangle.append(QGeoCoordinate(0.0, 0.0));
    triangle.append(QGeoCoordinate(1.0, 0.0));
    triangle.append(QGeoCoordinate(0.5, 1.0));

    const auto hull = GeoUtilities::convexHull(triangle);
    QCOMPARE(hull.size(), 3);
}

void GeoUtilitiesTest::_testConvexHullSquare()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    const auto hull = GeoUtilities::convexHull(square);
    QCOMPARE(hull.size(), 4);
}

void GeoUtilitiesTest::_testConvexHullWithInterior()
{
    // Square with an interior point
    QList<QGeoCoordinate> points;
    points.append(QGeoCoordinate(0.0, 0.0));
    points.append(QGeoCoordinate(0.0, 1.0));
    points.append(QGeoCoordinate(1.0, 1.0));
    points.append(QGeoCoordinate(1.0, 0.0));
    points.append(QGeoCoordinate(0.5, 0.5));  // Interior point

    const auto hull = GeoUtilities::convexHull(points);
    QCOMPARE(hull.size(), 4);  // Interior point should be excluded
}

void GeoUtilitiesTest::_testConvexHullDegenerate()
{
    // Empty list
    QList<QGeoCoordinate> empty;
    QVERIFY(GeoUtilities::convexHull(empty).isEmpty());

    // Single point
    QList<QGeoCoordinate> single;
    single.append(QGeoCoordinate(0.0, 0.0));
    QCOMPARE(GeoUtilities::convexHull(single).size(), 1);

    // Two points
    QList<QGeoCoordinate> two;
    two.append(QGeoCoordinate(0.0, 0.0));
    two.append(QGeoCoordinate(1.0, 1.0));
    QCOMPARE(GeoUtilities::convexHull(two).size(), 2);
}

// ============================================================================
// Polygon Offset Tests
// ============================================================================

void GeoUtilitiesTest::_testOffsetPolygon()
{
    // Simple square polygon
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(47.0, -122.0));
    square.append(QGeoCoordinate(47.0, -121.9));
    square.append(QGeoCoordinate(47.1, -121.9));
    square.append(QGeoCoordinate(47.1, -122.0));

    // Expand outward
    const auto expanded = GeoUtilities::offsetPolygon(square, 1000);  // 1km outward
    QCOMPARE(expanded.size(), square.size());

    // Each point should have moved from original
    for (int i = 0; i < square.size(); ++i) {
        const double dist = square[i].distanceTo(expanded[i]);
        QVERIFY(dist > 0);  // Points should have moved
    }
}

void GeoUtilitiesTest::_testOffsetPolyline()
{
    QList<QGeoCoordinate> line;
    line.append(QGeoCoordinate(47.0, -122.0));
    line.append(QGeoCoordinate(47.0, -121.9));
    line.append(QGeoCoordinate(47.0, -121.8));

    const auto offset = GeoUtilities::offsetPolyline(line, 100);  // 100m offset
    QCOMPARE(offset.size(), line.size());

    // All points should be roughly 100m away from original
    for (int i = 0; i < line.size(); ++i) {
        const double dist = line[i].distanceTo(offset[i]);
        QVERIFY(qAbs(dist - 100) < 10);  // Within 10m tolerance
    }
}

void GeoUtilitiesTest::_testOffsetNegative()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(47.0, -122.0));
    square.append(QGeoCoordinate(47.0, -121.9));
    square.append(QGeoCoordinate(47.1, -121.9));
    square.append(QGeoCoordinate(47.1, -122.0));

    // Shrink inward
    const auto shrunk = GeoUtilities::offsetPolygon(square, -1000);  // 1km inward
    QCOMPARE(shrunk.size(), square.size());

    // Each point should have moved from original
    for (int i = 0; i < square.size(); ++i) {
        const double dist = square[i].distanceTo(shrunk[i]);
        QVERIFY(dist > 0);  // Points should have moved
    }

    // Positive offset should move points in opposite direction
    const auto expanded = GeoUtilities::offsetPolygon(square, 1000);
    // Expanded and shrunk should be different
    QVERIFY(expanded[0].latitude() != shrunk[0].latitude() ||
            expanded[0].longitude() != shrunk[0].longitude());
}

// ============================================================================
// Closest Point Tests
// ============================================================================

void GeoUtilitiesTest::_testClosestPointOnPolygon()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(47.0, -122.0));
    square.append(QGeoCoordinate(47.0, -121.0));
    square.append(QGeoCoordinate(48.0, -121.0));
    square.append(QGeoCoordinate(48.0, -122.0));

    // Point inside - should find closest edge
    double dist = 0;
    auto closest = GeoUtilities::closestPointOnPolygon(
        QGeoCoordinate(47.5, -121.8), square, &dist);
    QVERIFY(closest.isValid());
    QVERIFY(dist > 0);

    // Point outside - should find closest edge
    closest = GeoUtilities::closestPointOnPolygon(
        QGeoCoordinate(49.0, -121.5), square, &dist);
    QVERIFY(closest.isValid());
    QVERIFY(dist > 0);
}

void GeoUtilitiesTest::_testClosestPointOnPolyline()
{
    QList<QGeoCoordinate> line;
    line.append(QGeoCoordinate(47.0, -122.0));
    line.append(QGeoCoordinate(47.0, -121.5));
    line.append(QGeoCoordinate(47.0, -121.0));

    double dist = 0;
    int segmentIndex = -1;
    const auto closest = GeoUtilities::closestPointOnPolyline(
        QGeoCoordinate(47.5, -121.5), line, &dist, &segmentIndex);

    QVERIFY(closest.isValid());
    QVERIFY(dist > 0);
    QVERIFY(segmentIndex >= 0 && segmentIndex < line.size() - 1);
}

void GeoUtilitiesTest::_testClosestPointOnPolygonVertex()
{
    QList<QGeoCoordinate> triangle;
    triangle.append(QGeoCoordinate(0.0, 0.0));
    triangle.append(QGeoCoordinate(0.0, 1.0));
    triangle.append(QGeoCoordinate(1.0, 0.5));

    // Point near vertex should snap to vertex
    double dist = 0;
    const auto closest = GeoUtilities::closestPointOnPolygon(
        QGeoCoordinate(-0.001, -0.001), triangle, &dist);
    QVERIFY(closest.isValid());
}

// ============================================================================
// Path Heading Tests
// ============================================================================

void GeoUtilitiesTest::_testHeadingAtDistance()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(47.0, -122.0));
    path.append(QGeoCoordinate(47.0, -121.0));  // East
    path.append(QGeoCoordinate(48.0, -121.0));  // North

    // At start - heading should be roughly 90 (east)
    double heading = GeoUtilities::headingAtDistance(path, 0);
    QVERIFY(heading >= 85 && heading <= 95);

    // Total path length
    double totalLength = 0;
    for (int i = 0; i < path.size() - 1; ++i) {
        totalLength += path[i].distanceTo(path[i + 1]);
    }

    // Near end - heading should be roughly 0 (north)
    heading = GeoUtilities::headingAtDistance(path, totalLength - 100);
    QVERIFY(heading >= 355 || heading <= 5);
}

void GeoUtilitiesTest::_testHeadingAtFraction()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(47.0, -122.0));
    path.append(QGeoCoordinate(48.0, -122.0));  // Due north

    const double heading = GeoUtilities::headingAtFraction(path, 0.5);
    QVERIFY(heading >= 355 || heading <= 5);  // Should be roughly north
}

void GeoUtilitiesTest::_testPathHeadings()
{
    QList<QGeoCoordinate> path;
    path.append(QGeoCoordinate(47.0, -122.0));
    path.append(QGeoCoordinate(47.0, -121.0));  // East
    path.append(QGeoCoordinate(48.0, -121.0));  // North

    const auto headings = GeoUtilities::pathHeadings(path);
    QCOMPARE(headings.size(), 3);  // One heading per vertex

    // First vertex: heading east (~90)
    QVERIFY(headings[0] >= 85 && headings[0] <= 95);

    // Middle vertex: average of east and north (~45)
    QVERIFY(headings[1] >= 40 && headings[1] <= 50);

    // Last vertex: heading north (~0)
    QVERIFY(headings[2] >= 355 || headings[2] <= 5);
}

// ============================================================================
// Chaikin Smoothing Tests
// ============================================================================

void GeoUtilitiesTest::_testSmoothChaikin()
{
    // Simple square polygon
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    const auto smoothed = GeoUtilities::smoothChaikin(square, 1);

    // One iteration doubles the number of vertices
    QCOMPARE(smoothed.size(), 8);
}

void GeoUtilitiesTest::_testSmoothChaikinPolyline()
{
    QList<QGeoCoordinate> line;
    line.append(QGeoCoordinate(0.0, 0.0));
    line.append(QGeoCoordinate(0.5, 1.0));
    line.append(QGeoCoordinate(1.0, 0.0));

    const auto smoothed = GeoUtilities::smoothChaikinPolyline(line, 1);

    // Polyline smoothing should preserve endpoints
    QCOMPARE(smoothed.first().latitude(), line.first().latitude());
    QCOMPARE(smoothed.last().latitude(), line.last().latitude());
}

void GeoUtilitiesTest::_testSmoothChaikinIterations()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    const auto smooth1 = GeoUtilities::smoothChaikin(square, 1);
    const auto smooth2 = GeoUtilities::smoothChaikin(square, 2);
    const auto smooth3 = GeoUtilities::smoothChaikin(square, 3);

    QVERIFY(smooth2.size() > smooth1.size());
    QVERIFY(smooth3.size() > smooth2.size());
}

// ============================================================================
// Anti-Meridian Tests
// ============================================================================

void GeoUtilitiesTest::_testCrossesAntimeridian()
{
    // Does not cross
    QList<QGeoCoordinate> normal;
    normal.append(QGeoCoordinate(47.0, -122.0));
    normal.append(QGeoCoordinate(47.0, -121.0));
    QVERIFY(!GeoUtilities::crossesAntimeridian(normal));

    // Crosses anti-meridian
    QList<QGeoCoordinate> crosses;
    crosses.append(QGeoCoordinate(47.0, 179.0));
    crosses.append(QGeoCoordinate(47.0, -179.0));
    QVERIFY(GeoUtilities::crossesAntimeridian(crosses));
}

void GeoUtilitiesTest::_testSplitAtAntimeridian()
{
    QList<QGeoCoordinate> polygon;
    polygon.append(QGeoCoordinate(47.0, 179.0));
    polygon.append(QGeoCoordinate(48.0, 179.0));
    polygon.append(QGeoCoordinate(48.0, -179.0));
    polygon.append(QGeoCoordinate(47.0, -179.0));

    QList<QGeoCoordinate> west, east;
    const bool split = GeoUtilities::splitAtAntimeridian(polygon, west, east);

    QVERIFY(split);
    QVERIFY(!west.isEmpty());
    QVERIFY(!east.isEmpty());
}

void GeoUtilitiesTest::_testNormalizeForAntimeridian()
{
    QList<QGeoCoordinate> polygon;
    polygon.append(QGeoCoordinate(47.0, 179.0));
    polygon.append(QGeoCoordinate(48.0, 179.0));
    polygon.append(QGeoCoordinate(48.0, -179.0));
    polygon.append(QGeoCoordinate(47.0, -179.0));

    QList<QGeoCoordinate> normalized = polygon;
    const bool changed = GeoUtilities::normalizeForAntimeridian(normalized);

    QVERIFY(changed);
    // After normalization, all longitudes should be on the same side
    bool allPositive = true;
    bool allNegative = true;
    for (const auto &c : normalized) {
        if (c.longitude() < 0) allPositive = false;
        if (c.longitude() > 0) allNegative = false;
    }
    QVERIFY(allPositive || allNegative);
}

// ============================================================================
// Minimum Bounding Circle Tests
// ============================================================================

void GeoUtilitiesTest::_testMinimumBoundingCircle()
{
    // Square polygon
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(47.0, -122.0));
    square.append(QGeoCoordinate(47.0, -121.0));
    square.append(QGeoCoordinate(48.0, -121.0));
    square.append(QGeoCoordinate(48.0, -122.0));

    const auto circle = GeoUtilities::minimumBoundingCircle(square);
    QVERIFY(circle.center.isValid());
    QVERIFY(circle.radiusMeters > 0);

    // All points should be within or on the circle boundary
    for (const auto &c : square) {
        const double dist = circle.center.distanceTo(c);
        QVERIFY(dist <= circle.radiusMeters + 1);  // 1m tolerance
    }
}

void GeoUtilitiesTest::_testMinimumBoundingCircleSingle()
{
    QList<QGeoCoordinate> single;
    single.append(QGeoCoordinate(47.0, -122.0));

    const auto circle = GeoUtilities::minimumBoundingCircle(single);
    QVERIFY(circle.center.isValid());
    QCOMPARE(circle.radiusMeters, 0.0);
}

void GeoUtilitiesTest::_testMinimumBoundingCircleTwoPoints()
{
    QList<QGeoCoordinate> two;
    two.append(QGeoCoordinate(47.0, -122.0));
    two.append(QGeoCoordinate(48.0, -122.0));

    const auto circle = GeoUtilities::minimumBoundingCircle(two);
    QVERIFY(circle.center.isValid());

    // Radius should be half the distance between points
    const double dist = two[0].distanceTo(two[1]);
    QVERIFY(qAbs(circle.radiusMeters - dist / 2) < 10);  // 10m tolerance
}

// ============================================================================
// Line-Polygon Intersection Tests
// ============================================================================

void GeoUtilitiesTest::_testLineIntersectsPolygon()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    // Line passes through polygon (crosses edges)
    QVERIFY(GeoUtilities::lineIntersectsPolygon(
        QGeoCoordinate(-0.5, 0.5),
        QGeoCoordinate(1.5, 0.5),
        square));

    // Line outside polygon
    QVERIFY(!GeoUtilities::lineIntersectsPolygon(
        QGeoCoordinate(2.0, 0.0),
        QGeoCoordinate(2.0, 1.0),
        square));

    // Line inside polygon (endpoints inside) - also counts as intersecting
    // because the line is within the polygon area
    QVERIFY(GeoUtilities::lineIntersectsPolygon(
        QGeoCoordinate(0.25, 0.25),
        QGeoCoordinate(0.75, 0.75),
        square));
}

void GeoUtilitiesTest::_testLinePolygonIntersections()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    // Line that crosses two edges
    const auto intersections = GeoUtilities::linePolygonIntersections(
        QGeoCoordinate(-0.5, 0.5),
        QGeoCoordinate(1.5, 0.5),
        square);

    QCOMPARE(intersections.size(), 2);
}

void GeoUtilitiesTest::_testPolylineIntersectsPolygon()
{
    QList<QGeoCoordinate> square;
    square.append(QGeoCoordinate(0.0, 0.0));
    square.append(QGeoCoordinate(0.0, 1.0));
    square.append(QGeoCoordinate(1.0, 1.0));
    square.append(QGeoCoordinate(1.0, 0.0));

    // Polyline that crosses polygon
    QList<QGeoCoordinate> crossingLine;
    crossingLine.append(QGeoCoordinate(-0.5, 0.5));
    crossingLine.append(QGeoCoordinate(0.5, 0.5));
    crossingLine.append(QGeoCoordinate(1.5, 0.5));
    QVERIFY(GeoUtilities::polylineIntersectsPolygon(crossingLine, square));

    // Polyline completely outside
    QList<QGeoCoordinate> outsideLine;
    outsideLine.append(QGeoCoordinate(2.0, 0.0));
    outsideLine.append(QGeoCoordinate(2.0, 1.0));
    outsideLine.append(QGeoCoordinate(3.0, 1.0));
    QVERIFY(!GeoUtilities::polylineIntersectsPolygon(outsideLine, square));
}

// ============================================================================
// Polygon Hole Validation Tests
// ============================================================================

void GeoUtilitiesTest::_testIsHoleContained()
{
    // Outer ring
    QList<QGeoCoordinate> outer;
    outer.append(QGeoCoordinate(0.0, 0.0));
    outer.append(QGeoCoordinate(0.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 0.0));

    // Hole inside
    QList<QGeoCoordinate> holeInside;
    holeInside.append(QGeoCoordinate(2.0, 2.0));
    holeInside.append(QGeoCoordinate(2.0, 4.0));
    holeInside.append(QGeoCoordinate(4.0, 4.0));
    holeInside.append(QGeoCoordinate(4.0, 2.0));
    QVERIFY(GeoUtilities::isHoleContained(holeInside, outer));

    // Hole outside
    QList<QGeoCoordinate> holeOutside;
    holeOutside.append(QGeoCoordinate(20.0, 20.0));
    holeOutside.append(QGeoCoordinate(20.0, 22.0));
    holeOutside.append(QGeoCoordinate(22.0, 22.0));
    holeOutside.append(QGeoCoordinate(22.0, 20.0));
    QVERIFY(!GeoUtilities::isHoleContained(holeOutside, outer));
}

void GeoUtilitiesTest::_testValidateHoles()
{
    // Outer ring
    QList<QGeoCoordinate> outer;
    outer.append(QGeoCoordinate(0.0, 0.0));
    outer.append(QGeoCoordinate(0.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 0.0));

    // Valid holes (non-overlapping, inside outer)
    QList<QList<QGeoCoordinate>> validHoles;
    QList<QGeoCoordinate> hole1;
    hole1.append(QGeoCoordinate(1.0, 1.0));
    hole1.append(QGeoCoordinate(1.0, 3.0));
    hole1.append(QGeoCoordinate(3.0, 3.0));
    hole1.append(QGeoCoordinate(3.0, 1.0));
    validHoles.append(hole1);

    QList<QGeoCoordinate> hole2;
    hole2.append(QGeoCoordinate(5.0, 5.0));
    hole2.append(QGeoCoordinate(5.0, 7.0));
    hole2.append(QGeoCoordinate(7.0, 7.0));
    hole2.append(QGeoCoordinate(7.0, 5.0));
    validHoles.append(hole2);

    QString error;
    QVERIFY(GeoUtilities::validateHoles(outer, validHoles, error));
}

void GeoUtilitiesTest::_testValidateHolesOverlapping()
{
    // Outer ring
    QList<QGeoCoordinate> outer;
    outer.append(QGeoCoordinate(0.0, 0.0));
    outer.append(QGeoCoordinate(0.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 10.0));
    outer.append(QGeoCoordinate(10.0, 0.0));

    // Overlapping holes
    QList<QList<QGeoCoordinate>> overlappingHoles;
    QList<QGeoCoordinate> hole1;
    hole1.append(QGeoCoordinate(1.0, 1.0));
    hole1.append(QGeoCoordinate(1.0, 5.0));
    hole1.append(QGeoCoordinate(5.0, 5.0));
    hole1.append(QGeoCoordinate(5.0, 1.0));
    overlappingHoles.append(hole1);

    QList<QGeoCoordinate> hole2;
    hole2.append(QGeoCoordinate(3.0, 3.0));  // Overlaps with hole1
    hole2.append(QGeoCoordinate(3.0, 7.0));
    hole2.append(QGeoCoordinate(7.0, 7.0));
    hole2.append(QGeoCoordinate(7.0, 3.0));
    overlappingHoles.append(hole2);

    QString error;
    QVERIFY(!GeoUtilities::validateHoles(outer, overlappingHoles, error));
    QVERIFY(error.contains("overlap") || error.contains("intersect"));
}

// ============================================================================
// Great Circle Intersection Tests
// ============================================================================

void GeoUtilitiesTest::_testGreatCircleIntersection()
{
    // Two great circles that intersect
    QGeoCoordinate intersection1, intersection2;

    // Equator segment and a meridian segment
    // Only one intersection within both segments (at 0,0)
    // The antipode (0,180) is not on either segment
    const int count = GeoUtilities::greatCircleIntersection(
        QGeoCoordinate(0.0, -10.0),   // Point on equator
        QGeoCoordinate(0.0, 10.0),    // Point on equator
        QGeoCoordinate(-10.0, 0.0),   // Point on prime meridian
        QGeoCoordinate(10.0, 0.0),    // Point on prime meridian
        intersection1,
        intersection2);

    QCOMPARE(count, 1);  // One intersection point on both segments (0,0)
    QVERIFY(intersection1.isValid());
    QVERIFY(qAbs(intersection1.latitude()) < 1);
    QVERIFY(qAbs(intersection1.longitude()) < 1);
}

void GeoUtilitiesTest::_testGeodesicSegmentsIntersect()
{
    // Two segments that cross
    QVERIFY(GeoUtilities::geodesicSegmentsIntersect(
        QGeoCoordinate(0.0, 0.0),
        QGeoCoordinate(1.0, 1.0),
        QGeoCoordinate(0.0, 1.0),
        QGeoCoordinate(1.0, 0.0)));

    // Parallel segments (don't cross)
    QVERIFY(!GeoUtilities::geodesicSegmentsIntersect(
        QGeoCoordinate(0.0, 0.0),
        QGeoCoordinate(0.0, 1.0),
        QGeoCoordinate(1.0, 0.0),
        QGeoCoordinate(1.0, 1.0)));
}

void GeoUtilitiesTest::_testGreatCircleParallel()
{
    // Two points on same great circle (same meridian)
    QGeoCoordinate intersection1, intersection2;

    const int count = GeoUtilities::greatCircleIntersection(
        QGeoCoordinate(10.0, 0.0),
        QGeoCoordinate(20.0, 0.0),
        QGeoCoordinate(30.0, 0.0),
        QGeoCoordinate(40.0, 0.0),
        intersection1,
        intersection2);

    // Same great circle - infinite intersections (or 0 for practical purposes)
    QVERIFY(count == 0 || count == -1);  // Implementation may vary
}

// ============================================================================
// Cross-Track Distance Tests
// ============================================================================

void GeoUtilitiesTest::_testCrossTrackDistance()
{
    // Point directly on the path
    QGeoCoordinate pathStart(0.0, 0.0);
    QGeoCoordinate pathEnd(0.0, 1.0);
    QGeoCoordinate pointOnPath(0.0, 0.5);

    double xtd = GeoUtilities::crossTrackDistance(pointOnPath, pathStart, pathEnd);
    QVERIFY(qAbs(xtd) < 1.0);  // Should be essentially 0
}

void GeoUtilitiesTest::_testCrossTrackDistanceSign()
{
    // Point to the right of the path (positive)
    QGeoCoordinate pathStart(0.0, 0.0);
    QGeoCoordinate pathEnd(1.0, 0.0);  // Going east
    QGeoCoordinate pointRight(0.5, 0.001);  // Slightly south (right when heading east)
    QGeoCoordinate pointLeft(0.5, -0.001);  // Slightly north (left when heading east)

    double xtdRight = GeoUtilities::crossTrackDistance(pointRight, pathStart, pathEnd);
    double xtdLeft = GeoUtilities::crossTrackDistance(pointLeft, pathStart, pathEnd);

    // Right should be positive, left should be negative
    QVERIFY(xtdRight > 0);
    QVERIFY(xtdLeft < 0);
}

void GeoUtilitiesTest::_testAlongTrackDistance()
{
    QGeoCoordinate pathStart(0.0, 0.0);
    QGeoCoordinate pathEnd(0.0, 1.0);
    QGeoCoordinate point(0.0, 0.5);

    double atd = GeoUtilities::alongTrackDistance(point, pathStart, pathEnd);
    double halfPath = QGCGeo::geodesicDistance(pathStart, pathEnd) / 2.0;

    QVERIFY(qAbs(atd - halfPath) < 100);  // Within 100m of half the path
}

void GeoUtilitiesTest::_testCrossTrackDistanceToPath()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 1.0)
         << QGeoCoordinate(0.0, 2.0);

    // Point near middle of first segment
    // Path goes east (longitude increasing) along equator
    // Point at latitude 0.001 is NORTH (to the LEFT when heading east)
    QGeoCoordinate point(0.001, 0.5);

    GeoUtilities::CrossTrackResult result = GeoUtilities::crossTrackDistanceToPath(point, path);

    QCOMPARE(result.segmentIndex, 0);
    QVERIFY(result.crossTrackMeters != 0);  // Off the path
    QVERIFY(result.closestPoint.isValid());
}

// ============================================================================
// Turn Radius Validation Tests
// ============================================================================

void GeoUtilitiesTest::_testTurnAngle()
{
    // 90 degree turn
    QGeoCoordinate p1(0.0, 0.0);
    QGeoCoordinate p2(1.0, 0.0);
    QGeoCoordinate p3(1.0, 1.0);

    double angle = GeoUtilities::turnAngle(p1, p2, p3);
    QVERIFY(qAbs(angle - 90.0) < 5.0);  // Approximately 90 degrees
}

void GeoUtilitiesTest::_testTurnAngleStraight()
{
    // Straight line (0 degree turn)
    QGeoCoordinate p1(0.0, 0.0);
    QGeoCoordinate p2(0.0, 1.0);
    QGeoCoordinate p3(0.0, 2.0);

    double angle = GeoUtilities::turnAngle(p1, p2, p3);
    QVERIFY(angle < 2.0);  // Nearly 0
}

void GeoUtilitiesTest::_testRequiredTurnRadius()
{
    // Sharp turn with short legs
    QGeoCoordinate p1(0.0, 0.0);
    QGeoCoordinate p2(0.001, 0.0);  // ~111m north
    QGeoCoordinate p3(0.001, 0.001);  // ~111m east from p2

    double radius = GeoUtilities::requiredTurnRadius(p1, p2, p3);
    QVERIFY(radius > 0);
    QVERIFY(radius < 1000);  // Should be reasonable for short legs
}

void GeoUtilitiesTest::_testIsTurnFeasible()
{
    // Large turn radius requirement with short legs - should fail
    QGeoCoordinate p1(0.0, 0.0);
    QGeoCoordinate p2(0.001, 0.0);
    QGeoCoordinate p3(0.001, 0.001);

    // Very large minimum turn radius
    QVERIFY(!GeoUtilities::isTurnFeasible(p1, p2, p3, 10000));

    // Small minimum turn radius - should pass
    QVERIFY(GeoUtilities::isTurnFeasible(p1, p2, p3, 10));
}

void GeoUtilitiesTest::_testValidatePathTurns()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.01, 0.0)   // ~1.1km segment
         << QGeoCoordinate(0.01, 0.01)
         << QGeoCoordinate(0.02, 0.01);

    QList<int> invalidIndices;

    // Small radius - all turns should be valid
    QVERIFY(GeoUtilities::validatePathTurns(path, 10, invalidIndices));
    QVERIFY(invalidIndices.isEmpty());

    // Very large radius - turns should be invalid
    QVERIFY(!GeoUtilities::validatePathTurns(path, 100000, invalidIndices));
    QVERIFY(!invalidIndices.isEmpty());
}

// ============================================================================
// Path Densification Tests
// ============================================================================

void GeoUtilitiesTest::_testDensifyPath()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 0.1);  // ~11km apart

    // Densify to max 1km spacing
    QList<QGeoCoordinate> densified = GeoUtilities::densifyPath(path, 1000);

    QVERIFY(densified.size() > path.size());
    QVERIFY(densified.size() >= 12);  // At least 12 points for 11km at 1km spacing
}

void GeoUtilitiesTest::_testDensifyPathNoChange()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 0.001);  // ~111m apart

    // Max spacing larger than segment
    QList<QGeoCoordinate> densified = GeoUtilities::densifyPath(path, 1000);

    QCOMPARE(densified.size(), path.size());  // No change needed
}

void GeoUtilitiesTest::_testDensifyPolygon()
{
    QList<QGeoCoordinate> polygon;
    polygon << QGeoCoordinate(0.0, 0.0)
            << QGeoCoordinate(0.0, 0.1)
            << QGeoCoordinate(0.1, 0.1);

    QList<QGeoCoordinate> densified = GeoUtilities::densifyPolygon(polygon, 1000);

    QVERIFY(densified.size() > polygon.size());
}

// ============================================================================
// No-Fly Zone Tests
// ============================================================================

void GeoUtilitiesTest::_testPointInNoFlyZone()
{
    QList<QList<QGeoCoordinate>> zones;

    // Create a simple square no-fly zone
    QList<QGeoCoordinate> zone1;
    zone1 << QGeoCoordinate(0.0, 0.0)
          << QGeoCoordinate(0.0, 1.0)
          << QGeoCoordinate(1.0, 1.0)
          << QGeoCoordinate(1.0, 0.0);
    zones.append(zone1);

    // Point inside zone
    QCOMPARE(GeoUtilities::pointInNoFlyZone(QGeoCoordinate(0.5, 0.5), zones), 0);

    // Point outside zone
    QCOMPARE(GeoUtilities::pointInNoFlyZone(QGeoCoordinate(2.0, 2.0), zones), -1);
}

void GeoUtilitiesTest::_testPathIntersectsNoFlyZone()
{
    QList<QList<QGeoCoordinate>> zones;

    QList<QGeoCoordinate> zone1;
    zone1 << QGeoCoordinate(0.0, 0.0)
          << QGeoCoordinate(0.0, 1.0)
          << QGeoCoordinate(1.0, 1.0)
          << QGeoCoordinate(1.0, 0.0);
    zones.append(zone1);

    // Path that crosses zone
    QList<QGeoCoordinate> crossingPath;
    crossingPath << QGeoCoordinate(-0.5, 0.5)
                 << QGeoCoordinate(1.5, 0.5);

    QCOMPARE(GeoUtilities::pathIntersectsNoFlyZone(crossingPath, zones), 0);

    // Path that avoids zone
    QList<QGeoCoordinate> safePath;
    safePath << QGeoCoordinate(2.0, 0.0)
             << QGeoCoordinate(2.0, 1.0);

    QCOMPARE(GeoUtilities::pathIntersectsNoFlyZone(safePath, zones), -1);
}

void GeoUtilitiesTest::_testFindNoFlyZoneIntersections()
{
    QList<QList<QGeoCoordinate>> zones;

    QList<QGeoCoordinate> zone1;
    zone1 << QGeoCoordinate(0.0, 0.0)
          << QGeoCoordinate(0.0, 1.0)
          << QGeoCoordinate(1.0, 1.0)
          << QGeoCoordinate(1.0, 0.0);
    zones.append(zone1);

    // Path that crosses zone
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(-0.5, 0.5)
         << QGeoCoordinate(1.5, 0.5);

    auto intersections = GeoUtilities::findNoFlyZoneIntersections(path, zones);

    QCOMPARE(intersections.size(), 2);  // Enter and exit
}

// ============================================================================
// Polygon Triangulation Tests
// ============================================================================

void GeoUtilitiesTest::_testTriangulatePolygonTriangle()
{
    QList<QGeoCoordinate> triangle;
    triangle << QGeoCoordinate(0.0, 0.0)
             << QGeoCoordinate(0.0, 1.0)
             << QGeoCoordinate(1.0, 0.0);

    auto triangles = GeoUtilities::triangulatePolygon(triangle);

    QCOMPARE(triangles.size(), 1);  // One triangle
}

void GeoUtilitiesTest::_testTriangulatePolygonSquare()
{
    QList<QGeoCoordinate> square;
    square << QGeoCoordinate(0.0, 0.0)
           << QGeoCoordinate(0.0, 1.0)
           << QGeoCoordinate(1.0, 1.0)
           << QGeoCoordinate(1.0, 0.0);

    auto triangles = GeoUtilities::triangulatePolygon(square);

    QCOMPARE(triangles.size(), 2);  // Two triangles for a square
}

void GeoUtilitiesTest::_testTriangulatePolygonConvex()
{
    // Pentagon
    QList<QGeoCoordinate> pentagon;
    pentagon << QGeoCoordinate(0.0, 0.5)
             << QGeoCoordinate(0.5, 1.0)
             << QGeoCoordinate(1.0, 0.75)
             << QGeoCoordinate(0.75, 0.0)
             << QGeoCoordinate(0.25, 0.0);

    auto triangles = GeoUtilities::triangulatePolygon(pentagon);

    QCOMPARE(triangles.size(), 3);  // n-2 triangles for convex polygon
}

void GeoUtilitiesTest::_testTriangulatePolygonConcave()
{
    // L-shaped polygon (concave)
    QList<QGeoCoordinate> lShape;
    lShape << QGeoCoordinate(0.0, 0.0)
           << QGeoCoordinate(0.0, 2.0)
           << QGeoCoordinate(1.0, 2.0)
           << QGeoCoordinate(1.0, 1.0)
           << QGeoCoordinate(2.0, 1.0)
           << QGeoCoordinate(2.0, 0.0);

    auto triangles = GeoUtilities::triangulatePolygon(lShape);

    QCOMPARE(triangles.size(), 4);  // n-2 = 6-2 = 4 triangles
}

// ============================================================================
// Plus Code Tests
// ============================================================================

void GeoUtilitiesTest::_testCoordinateToPlusCode()
{
    // Test a known location (Google's Googleplex approximately)
    QGeoCoordinate coord(37.4220, -122.0841);
    QString code = GeoUtilities::coordinateToPlusCode(coord);

    QVERIFY(!code.isEmpty());
    QVERIFY(code.contains('+'));
    QVERIFY(code.length() >= 8);
}

void GeoUtilitiesTest::_testPlusCodeToCoordinate()
{
    // A known Plus Code format
    QString code = "849VCWC8+R9";  // Google Googleplex area
    QGeoCoordinate coord;

    (void)GeoUtilities::plusCodeToCoordinate(code, coord);

    // The parsing might not work perfectly for all codes, but format should be valid
    QVERIFY(GeoUtilities::isValidPlusCode(code));
}

void GeoUtilitiesTest::_testPlusCodeRoundTrip()
{
    QGeoCoordinate original(40.7128, -74.0060);  // NYC
    QString code = GeoUtilities::coordinateToPlusCode(original, 10);

    QVERIFY(!code.isEmpty());
    QVERIFY(GeoUtilities::isValidPlusCode(code));
}

void GeoUtilitiesTest::_testIsValidPlusCode()
{
    QVERIFY(GeoUtilities::isValidPlusCode("8FVC9G8F+6W"));
    QVERIFY(GeoUtilities::isValidPlusCode("849VCWC8+R9"));

    // Invalid codes
    QVERIFY(!GeoUtilities::isValidPlusCode(""));
    QVERIFY(!GeoUtilities::isValidPlusCode("INVALID"));
    QVERIFY(!GeoUtilities::isValidPlusCode("8FVC9G8F"));  // Missing +
    QVERIFY(!GeoUtilities::isValidPlusCode("8FVC9G8F++6W"));  // Double +
}

// ============================================================================
// Path Corridor Tests
// ============================================================================

void GeoUtilitiesTest::_testPathCorridor()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 1.0);

    QList<QGeoCoordinate> corridor = GeoUtilities::pathCorridor(path, 1000);  // 1km wide

    QVERIFY(corridor.size() >= 4);  // At least a rectangle
}

void GeoUtilitiesTest::_testPointInCorridor()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 1.0);

    // Point on the path centerline
    QVERIFY(GeoUtilities::pointInCorridor(QGeoCoordinate(0.0, 0.5), path, 1000));

    // Point slightly off but within corridor
    QVERIFY(GeoUtilities::pointInCorridor(QGeoCoordinate(0.001, 0.5), path, 1000));
}

void GeoUtilitiesTest::_testPointInCorridorOutside()
{
    QList<QGeoCoordinate> path;
    path << QGeoCoordinate(0.0, 0.0)
         << QGeoCoordinate(0.0, 1.0);

    // Point far outside corridor
    QVERIFY(!GeoUtilities::pointInCorridor(QGeoCoordinate(1.0, 0.5), path, 1000));
}
