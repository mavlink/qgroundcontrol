#pragma once

#include "TestFixtures.h"

/// Unit tests for QGCGeoBoundingCube geometry calculations.
class QGCGeoBoundingCubeTest : public OfflineTest
{
    Q_OBJECT

public:
    QGCGeoBoundingCubeTest() = default;

private slots:
    // Basic state tests
    void _initialStateTest();
    void _resetTest();
    void _isValidTest();

    // Geometry tests
    void _centerTest();
    void _widthHeightTest();
    void _areaTest();
    void _radiusTest();

    // Polygon tests
    void _polygon2DTest();
    void _polygon2DClippedTest();

    // Operator tests
    void _equalityOperatorTest();
    void _assignmentOperatorTest();
    void _coordsEqualityTest();
};
