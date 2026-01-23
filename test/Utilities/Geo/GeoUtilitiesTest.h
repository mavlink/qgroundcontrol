#pragma once

#include "UnitTest.h"

class GeoUtilitiesTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Bounding box tests
    void _testBoundingBoxEmpty();
    void _testBoundingBoxSinglePoint();
    void _testBoundingBoxMultiplePoints();
    void _testBoundingBoxMultipleLists();
    void _testExpandBoundingBox();

    // Coordinate normalization tests
    void _testNormalizeLongitude();
    void _testNormalizeLongitudeEdgeCases();
    void _testNormalizeLatitude();
    void _testNormalizeLatitudeEdgeCases();
    void _testNormalizeCoordinate();
    void _testNormalizeCoordinates();

    // Altitude validation tests
    void _testIsValidAltitude();
    void _testIsValidAltitudeNaN();
    void _testIsValidAltitudeCoordinate();
    void _testValidateAltitudes();

    // Polygon validation tests
    void _testIsSelfIntersectingSimple();
    void _testIsSelfIntersectingComplex();
    void _testIsSelfIntersectingFigure8();
    void _testIsClockwise();
    void _testReverseVertices();
    void _testValidatePolygon();
    void _testValidatePolygonTooFewVertices();
    void _testValidatePolygonSelfIntersecting();
    void _testValidatePolygonZeroArea();

    // Coordinate validation tests
    void _testIsValidCoordinate();
    void _testValidateCoordinates();
    void _testValidatePolylineListCoordinates();

    // UTM conversion tests
    void _testConvertToUTM();
    void _testConvertFromUTM();
    void _testOptimalUTMZone();

    // Polygon geometry tests
    void _testPolygonCentroid();
    void _testPolygonCentroidDegenerate();
    void _testPolygonArea();
    void _testPolygonAreaWithHoles();
    void _testPolygonPerimeter();
    void _testPolylineMidpoint();

    // Douglas-Peucker simplification tests
    void _testSimplifyDouglasPeucker();
    void _testSimplifyToCount();

    // Visvalingam-Whyatt simplification tests
    void _testSimplifyVisvalingamWhyatt();
    void _testSimplifyVisvalingamToCount();

    // Point-in-polygon tests
    void _testPointInPolygon();
    void _testPointInPolygonEdgeCases();
    void _testPointInPolygonWithTolerance();

    // Path interpolation tests
    void _testInterpolateAlongPath();
    void _testInterpolateAlongPathAtDistance();
    void _testResamplePath();

    // WKT format tests
    void _testWKTParsePoint();
    void _testWKTParseLineString();
    void _testWKTParsePolygon();
    void _testWKTParsePolygonWithHoles();
    void _testWKTGenerate();
    void _testWKTRoundTrip();

    // Fuzz tests for parsers
    void _testFuzzWKTParser();
    void _testFuzzKMLParser();
    void _testFuzzGPXParser();

    // Performance benchmarks
    void _testBenchmarkDouglasPeucker();
    void _testBenchmarkBoundingBox();
    void _testBenchmarkVisvalingam();
    void _testBenchmarkPointInPolygon();

    // Convex hull tests
    void _testConvexHullTriangle();
    void _testConvexHullSquare();
    void _testConvexHullWithInterior();
    void _testConvexHullDegenerate();

    // Polygon offset tests
    void _testOffsetPolygon();
    void _testOffsetPolyline();
    void _testOffsetNegative();

    // Closest point tests
    void _testClosestPointOnPolygon();
    void _testClosestPointOnPolyline();
    void _testClosestPointOnPolygonVertex();

    // Path heading tests
    void _testHeadingAtDistance();
    void _testHeadingAtFraction();
    void _testPathHeadings();

    // Chaikin smoothing tests
    void _testSmoothChaikin();
    void _testSmoothChaikinPolyline();
    void _testSmoothChaikinIterations();

    // Anti-meridian tests
    void _testCrossesAntimeridian();
    void _testSplitAtAntimeridian();
    void _testNormalizeForAntimeridian();

    // Minimum bounding circle tests
    void _testMinimumBoundingCircle();
    void _testMinimumBoundingCircleSingle();
    void _testMinimumBoundingCircleTwoPoints();

    // Line-polygon intersection tests
    void _testLineIntersectsPolygon();
    void _testLinePolygonIntersections();
    void _testPolylineIntersectsPolygon();

    // Polygon hole validation tests
    void _testIsHoleContained();
    void _testValidateHoles();
    void _testValidateHolesOverlapping();

    // Great circle intersection tests
    void _testGreatCircleIntersection();
    void _testGeodesicSegmentsIntersect();
    void _testGreatCircleParallel();

    // Cross-track distance tests
    void _testCrossTrackDistance();
    void _testCrossTrackDistanceSign();
    void _testAlongTrackDistance();
    void _testCrossTrackDistanceToPath();

    // Turn radius validation tests
    void _testTurnAngle();
    void _testTurnAngleStraight();
    void _testRequiredTurnRadius();
    void _testIsTurnFeasible();
    void _testValidatePathTurns();

    // Path densification tests
    void _testDensifyPath();
    void _testDensifyPathNoChange();
    void _testDensifyPolygon();

    // No-fly zone tests
    void _testPointInNoFlyZone();
    void _testPathIntersectsNoFlyZone();
    void _testFindNoFlyZoneIntersections();

    // Polygon triangulation tests
    void _testTriangulatePolygonTriangle();
    void _testTriangulatePolygonSquare();
    void _testTriangulatePolygonConvex();
    void _testTriangulatePolygonConcave();

    // Plus Code tests
    void _testCoordinateToPlusCode();
    void _testPlusCodeToCoordinate();
    void _testPlusCodeRoundTrip();
    void _testIsValidPlusCode();

    // Path corridor tests
    void _testPathCorridor();
    void _testPointInCorridor();
    void _testPointInCorridorOutside();
};
