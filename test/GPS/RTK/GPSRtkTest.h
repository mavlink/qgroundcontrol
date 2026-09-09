#pragma once

#include "UnitTest.h"

class GPSRtkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCoreAvailableWithoutReceiver();
    void _positionSourceSelection();
    void _failedOpenNeverConnects();
    void _retiredWorkerCannotUpdateReplacement();
    void _workerCanOutliveManager();
    void _shutdownWithoutEventLoop_data();
    void _shutdownWithoutEventLoop();
    void _sourceHealthIndependentOfSurvey();
    void _testCountSatellitesClampsToMax();
    void _testCountSatellitesCountsUsed();
    void _testCountSatellitesIgnoresUsedBeyondCount();
};
