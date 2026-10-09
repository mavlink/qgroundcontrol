#pragma once

#include "UnitTest.h"

class GPSReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _retiredWorkerCannotUpdateReplacement();
    void _receiverPublishesGcsPosition();
    void _fixedBasePositionIsGcsPosition();
    void _surveyedBasePositionIsGcsPosition();
    void _workerCanOutliveManager();
    void _shutdownJoinsRetiredWorkers_data();
    void _shutdownJoinsRetiredWorkers();
    void _notificationsFollowCompletedConnection();
    void _receiverFramesAreValidated_data();
    void _receiverFramesAreValidated();
    void _logsFixTransitionsWithoutCoordinates();
    void _automaticConnection();
    void _udpPositionOnlyReceiver();
    void _passiveForwardingRegistersCorrections_data();
    void _passiveForwardingRegistersCorrections();
    void _passiveInputStatus_data();
    void _passiveInputStatus();
    void _openFailureNamesCause();
    void _outputOverflowWarning_data();
    void _outputOverflowWarning();
#ifndef QGC_NO_SERIAL_LINK
    void _serialPortEntries();
    void _serialReservationSurvivesDelayedStop();
    void _manualPassiveBaudPreserved_data();
    void _manualPassiveBaudPreserved();
#endif
};
