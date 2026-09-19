#pragma once

#include "UnitTest.h"

class GPSRtkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCoreAvailableWithoutReceiver();
    void _failedOpenNeverConnects();
    void _retiredWorkerCannotUpdateReplacement();
    void _workerCanOutliveManager();
    void _receiverFramesAreValidated_data();
    void _receiverFramesAreValidated();
    void _testCountSatellitesClampsToMax();
    void _testCountSatellitesCountsUsed();
    void _testCountSatellitesIgnoresUsedBeyondCount();
    void _countOnlyUsagePreservesInView();
    void _snapshotUsageEvidence_data();
    void _snapshotUsageEvidence();
    void _currentBaseSaveValidity_data();
    void _currentBaseSaveValidity();
    void _logsOnlyFixTransitions();
};
