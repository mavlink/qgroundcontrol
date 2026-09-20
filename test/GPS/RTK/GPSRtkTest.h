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
    void _connectionNotificationSupersedesAttempt_data();
    void _connectionNotificationSupersedesAttempt();
    void _factNotificationRetiresSession_data();
    void _factNotificationRetiresSession();
    void _manualNotificationSupersedesAttempt_data();
    void _manualNotificationSupersedesAttempt();
    void _receiverFramesAreValidated_data();
    void _receiverFramesAreValidated();
    void _testCountSatellitesClampsToMax();
    void _testCountSatellitesCountsUsed();
    void _testCountSatellitesIgnoresUsedBeyondCount();
    void _countOnlyUsagePreservesInView();
    void _snapshotUsageEvidence_data();
    void _snapshotUsageEvidence();
    void _unavailableSatelliteCoverage_data();
    void _unavailableSatelliteCoverage();
    void _currentBaseSaveValidity_data();
    void _currentBaseSaveValidity();
    void _logsFixTransitionsWithoutCoordinates();
    void _manufacturerIds_data();
    void _manufacturerIds();
    void _runtimeSettingsDoNotRequireAppRestart_data();
    void _runtimeSettingsDoNotRequireAppRestart();
    void _receiverSettingsMapping_data();
    void _receiverSettingsMapping();
    void _invalidReceiverSettings_data();
    void _invalidReceiverSettings();
    void _persistentConsentMapping_data();
    void _persistentConsentMapping();
    void _configurationDiagnosticRetained_data();
    void _configurationDiagnosticRetained();
    void _qmlConsentIsOneUse();
#ifndef QGC_NO_SERIAL_LINK
    void _explicitSerialSelectionAndDisconnect_data();
    void _explicitSerialSelectionAndDisconnect();
    void _manualSerialErrors_data();
    void _manualSerialErrors();
    void _serialReservationSurvivesDelayedStop();
    void _manualPassiveBaudPreserved_data();
    void _manualPassiveBaudPreserved();
#endif
};
