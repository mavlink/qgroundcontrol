#pragma once

#include "UnitTest.h"

class PlanarGeometryTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _lineIntersection_test();
    void _batchLineIntersection_test();
    void _concaveLineIntersection_test();
    void _vertexLineIntersection_test();
    void _largeCoordinateLineIntersection_test();
    void _polygonOffset_test();
    void _polylineOffset_test();
    void _polylineBuffer_test();
    void _closedPolylineBuffer_test();
    void _acuteCornerOffset_test();
    void _largeDistanceOffset_test();

    // Benchmarks (nanobench)
    void _benchmarkIntersectLines();
};
