#pragma once

#include "UnitTest.h"

class OrientationCalibrationStateTest : public UnitTest
{
    Q_OBJECT

public:
    OrientationCalibrationStateTest() = default;

private slots:
    void _testInitialState();
    void _testReset();
    void _testResetAllDone();
    void _testSetVisibleSides();
    void _testSetSideInProgress();
    void _testSetSideDone();
    void _testSetSideRotate();
    void _testAllVisibleSidesDone();
    void _testCompletedSideCount();
    void _testCurrentSide();
    void _testSignals();
};
