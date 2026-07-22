#pragma once

#include "BaseClasses/VehicleTest.h"

class OnboardLogDownloadTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _downloadTest();
    void _ftpEraseCancelTest();
    void _ftpDownloadRefreshCancelTest();
    void _ftpCancellationPreservesFinalPathTest();
    void _ftpPreOpenRefreshCancelTest();
    void _ftpReentrantRefreshIsCoalescedTest();
    void _ftpRemoteSizeGrowthTest();
    void _ftpUnsafePathTest();
    void _ftpListingBatchLifecycleTest();
    void _ftpListingJobOwnershipTest();
    void _ftpSubdirectoryErrorContinuesTest();
    void _downloadPreflightTest();
    void _ftpVehicleChangeDuringEraseTest();
    void _downloadReentrancyTest();
    void _ftpTransportDestructionReentrancyTest();
    void _transportFeedbackTest();
    void _ftpDeferredContinuationTest();
    void _ftpDeferredEraseGenerationTest();
    void _ftpStaleDownloadCompletionTest();
    void _duplicateLogDataTest();
    void _duplicateLogEntryTest();
    void _approximateLogSizeTest();
    void _emptyArduPilotLogEntryTest();
    void _inconsistentLogCountTest();
    void _sessionStateTest();
    void _entryStateTest();
    void _remoteBoundsTest();
    void _destructorCleanupTest();
    void _logProtocolReentrancyTest();
    void _logProtocolFailureHandlingTest();
    void _logDownloadCancelStopsTimerTest();
    void _selectionAndSortTest();
    void _typedModelRoleTest();
    void _modelReentrancyCleanupTest();
    void _selectionInvalidationTest();
    void _vehicleReentrancyTest();
    void _firmwarePolicyTest();
    void _transportOverrideTest();
};
