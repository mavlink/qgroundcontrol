#pragma once

#include "UnitTest.h"

class GPSRtkTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testCoreAvailableWithoutReceiver();
    void _failedOpenNeverConnects();
    void _retiredWorkerCannotUpdateReplacement();
    void _receiverPublishesGcsPosition();
    void _fixedBasePositionIsGcsPosition();
    void _receiverIntegrityFacts();
    void _surveyedBasePositionIsGcsPosition();
    void _workerCanOutliveManager();
    void _notificationsFollowCompletedConnection_data();
    void _notificationsFollowCompletedConnection();
    void _factNotificationRetiresSession_data();
    void _factNotificationRetiresSession();
    void _receiverFramesAreValidated_data();
    void _receiverFramesAreValidated();
    void _snapshotUsageEvidence_data();
    void _snapshotUsageEvidence();
    void _currentBaseSaveValidity_data();
    void _currentBaseSaveValidity();
    void _logsFixTransitionsWithoutCoordinates();
    void _configurationDebugRedactsFixedBaseCoordinates();
    void _manufacturerIds_data();
    void _manufacturerIds();
    void _runtimeSettingsDoNotRequireAppRestart_data();
    void _runtimeSettingsDoNotRequireAppRestart();
    void _compactCorrectionsFollowReceiverSupport();
    void _receiverSettingsMapping_data();
    void _receiverSettingsMapping();
    void _invalidReceiverSettings_data();
    void _invalidReceiverSettings();
    void _persistentConsentMapping_data();
    void _persistentConsentMapping();
    void _configurationDiagnosticRetained_data();
    void _configurationDiagnosticRetained();
    void _rtkSettingsBinding();
    void _udpPositionOnlyReceiver();
    void _silentReceiverClearsSolution();
#ifndef QGC_NO_SERIAL_LINK
    void _serialReservationSurvivesDelayedStop();
    void _manualPassiveBaudPreserved_data();
    void _manualPassiveBaudPreserved();
#endif
};
