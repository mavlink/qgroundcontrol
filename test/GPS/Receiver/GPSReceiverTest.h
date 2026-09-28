#pragma once

#include "UnitTest.h"

class GPSReceiverTest : public UnitTest
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
    void _automaticConnection();
    void _summaryLabel();
    void _udpPositionOnlyReceiver();
    void _passiveForwardingRegistersCorrections_data();
    void _passiveForwardingRegistersCorrections();
    void _silentReceiverClearsSolution();
#ifndef QGC_NO_SERIAL_LINK
    void _serialReservationSurvivesDelayedStop();
    void _manualPassiveBaudPreserved_data();
    void _manualPassiveBaudPreserved();
#endif
};
