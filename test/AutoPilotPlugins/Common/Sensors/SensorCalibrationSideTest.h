#pragma once

#include "UnitTest.h"

class SensorCalibrationSideTest : public UnitTest
{
    Q_OBJECT

public:
    SensorCalibrationSideTest() = default;

private slots:
    void _testSideToMask();
    void _testMaskToSide();
    void _testParseSideText();
    void _testSideName();
};
