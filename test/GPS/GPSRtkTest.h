#pragma once

#include "UnitTest.h"

class GPSRtkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCountSatellitesClampsToMax();
    void _testCountSatellitesCountsUsed();
    void _testCountSatellitesIgnoresUsedBeyondCount();
};
