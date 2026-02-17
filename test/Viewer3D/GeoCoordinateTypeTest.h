#pragma once

#include "UnitTest.h"

class GeoCoordinateTypeTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testGpsToLocal();
    void _testSetGpsRefEmitsSignal();
    void _testSetCoordinateEmitsSignal();
    void _testSameValueNoEmit();
    void _testLocalCoordinateAtOrigin();
    void _testCoordinateBeforeGpsRef();
};
