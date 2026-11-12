#pragma once

#include <functional>
#include <memory>

#include <QtCore/QString>

#include "GPSCancellation.h"
#include "UnitTest.h"

class GPSReceiver;
class GPSTransport;

class GPSReceiverTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _retiredWorkerCannotUpdateReplacement();
    void _receiverPublishesGcsPosition();
    void _fixedBasePositionIsGcsPosition();
    void _fixedBaseCheck();
    void _surveyedBasePositionIsGcsPosition();
    void _workerCanOutliveManager();
    void _shutdownJoinsRetiredWorkers_data();
    void _shutdownJoinsRetiredWorkers();
    void _notificationsFollowCompletedConnection();
    void _receiverFramesAreValidated_data();
    void _receiverFramesAreValidated();
    void _logsFixTransitionsWithoutCoordinates();
    void _automaticConnection();
    void _detectedReceiverIsRememberedPerLink();
    void _statusTextNamesWhatDetectionTries_data();
    void _statusTextNamesWhatDetectionTries();
    void _statusTextFollowsState_data();
    void _statusTextFollowsState();
    void _statusTextSaysWhenNoCorrectionsAreSent_data();
    void _statusTextSaysWhenNoCorrectionsAreSent();
    void _udpPositionOnlyReceiver();
    void _passiveForwardingRegistersCorrections_data();
    void _passiveForwardingRegistersCorrections();
    void _passiveInputStatus_data();
    void _passiveInputStatus();
    void _tcpEndpointNamesHostAsEntered();
    void _openFailureNamesCause();
    void _outputOverflowWarning_data();
    void _outputOverflowWarning();
    void _recordsSessionsStartedWhileOn();
    void _recordingFileClearsWhenRecordingFails();
    void _recordingsStartedTogetherGetOwnFiles();
    void _withoutSerialLinksSerialSelectionUsesTcp();
    void _configuredPresentation_data();
    void _configuredPresentation();
    void _configurationDiagnosticRetained_data();
    void _configurationDiagnosticRetained();
    void _persistentConsentScope_data();
    void _persistentConsentScope();
    void _persistentConsentIsSpentOnConnect_data();
    void _persistentConsentIsSpentOnConnect();
    void _effectiveConnection_data();
    void _effectiveConnection();
#ifndef QGC_NO_SERIAL_LINK
    void _serialPortEntries();
    void _serialReservationSurvivesDelayedStop();
    void _manualPassiveBaudPreserved_data();
    void _manualPassiveBaudPreserved();

private:
    /// Opens the serial devices of @a receiver with @a open instead of the system's.
    static void _openSerialDevicesWith(
        GPSReceiver& receiver, std::function<std::unique_ptr<GPSTransport>(const QString&, GPSCancelToken)> open);
#endif
};
