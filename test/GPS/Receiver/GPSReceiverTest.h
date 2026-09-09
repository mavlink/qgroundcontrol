#pragma once

#include "UnitTest.h"

class GPSReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _projectionHandlesReentrantIntegrity_data();
    void _projectionHandlesReentrantIntegrity();
    void _retainedPositionFactsExpire();
    void _testCoreAvailableWithoutReceiver();
    void _positionSourceSelection();
    void _failedOpenNeverConnects();
    void _retiredWorkerCannotUpdateReplacement();
    void _facadeDestructionDoesNotStopSession();
    void _shutdownWithoutEventLoop_data();
    void _shutdownWithoutEventLoop();
    void _sourceHealthIndependentOfSurvey();
    void _liveFactsFollowHealth();
    void _testCountSatellitesClampsToMax();
    void _testCountSatellitesCountsUsed();
    void _testCountSatellitesIgnoresUsedBeyondCount();
};
