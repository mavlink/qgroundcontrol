#pragma once

#include "UnitTest.h"

class PolygonClipperTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _lineIntersection_test();
    void _batchLineIntersection_test();
    void _concaveLineIntersection_test();
    void _largeCoordinateLineIntersection_test();
};
