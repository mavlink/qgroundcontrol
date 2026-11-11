#pragma once

#include <chrono>

#include "NTRIPError.h"
#include "UnitTest.h"

namespace GPSTest {
class MockNTRIPTransport;
}

/// Tests observable NTRIP state transitions and reconnect behavior.
class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

    /// Fails @a transport and delivers the failure, which the manager logs.
    void _fail(GPSTest::MockNTRIPTransport* transport, const QString& detail, NTRIPError code = NTRIPError::SocketError,
               std::chrono::milliseconds retryAfter = {});

private slots:
    void _stopFromIdleIsNoop();
    void _statusMessageNamesStateWithoutDetail();
    void _canConnectNeedsCasterHost();
    void _transportFactory();
    void _stopCancelsDeferredSettings_data();
    void _stopCancelsDeferredSettings();
    void _newSessionRetryBudget_data();
    void _newSessionRetryBudget();
    void _plaintextCredentialWarningIsVisibleState_data();
    void _plaintextCredentialWarningIsVisibleState();
    void _certificatePinWriteBackKeepsConnection();
    void _optingOutForgetsCertificatePin_data();
    void _optingOutForgetsCertificatePin();

    // Reconnect backoff (migrated from NTRIPReconnectPolicyTest after the policy
    // was inlined into NTRIPManager). Driven through public state and ManualScheduler.
    void _reconnectGivesUpAfterAttemptCeiling();
    void _reconnectCancelStopsTimer();
    void _reconnectBackoffResetsOnCorrections();
    void _reconnectWaitsForNetworkAtFailure();
    void _reconnectWaitsWhenNetworkLostDuringBackoff();
    void _loopbackCasterBypassesNetworkGate_data();
    void _loopbackCasterBypassesNetworkGate();
    void _waitingStatusObserverCanStopManager();
    void _duplicateTransportErrorsScheduleOneRetry();
    void _retiredTransportErrorCannotAffectNewSession();
    void _retryPolicy_data();
    void _retryPolicy();
    void _unusableMountpointRetriedOnlyAfterCorrections_data();
    void _unusableMountpointRetriedOnlyAfterCorrections();
    void _httpRetryAfterReachesManager();
    void _unrepeatableAnswerWaitsForUser_data();
    void _unrepeatableAnswerWaitsForUser();
    void _version1CasterIsRemembered();
    void _retryPublicationSuperseded();
    void _missingMountpointDoesNotStartTransport();
    void _correctionIngressKeepsSessionAndIdentity();
    void _ggaSettingsUseInjectedProviders();
    void _connectedWithoutPositionSaysSo_data();
    void _connectedWithoutPositionSaysSo();
    void _mountpointChangeReconnects();
    void _transportDiagnosticsReachManager();
    void _statusCallbackStopsTransition();
    void _connectedCallbackStopsTransition_data();
    void _connectedCallbackStopsTransition();
};
