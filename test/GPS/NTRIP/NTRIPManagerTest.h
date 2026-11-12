#pragma once

#include <chrono>

#include <QtCore/QPointer>

#include "NTRIPError.h"
#include "UnitTest.h"

namespace GPSTest {
class MockNTRIPTransport;
}
class ManualScheduler;
class NTRIPManager;

/// Tests observable NTRIP state transitions and reconnect behavior.
class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

    /// Fails @a transport and delivers the failure, which the manager logs.
    void _fail(GPSTest::MockNTRIPTransport* transport, const QString& detail, NTRIPError code = NTRIPError::SocketError,
               std::chrono::milliseconds retryAfter = {});
    /// Fails the attempt in progress @a times, each followed by the next attempt. False unless each was made.
    bool _failAttempts(NTRIPManager& manager, ManualScheduler& scheduler, QPointer<GPSTest::MockNTRIPTransport>& latest,
                       int times);

private slots:
    void _stopFromIdleIsNoop();
    void _statusMessageNamesStateWithoutDetail();
    void _canConnectNeedsCasterHost();
    void _statusKeepsPlaceMarkers_data();
    void _statusKeepsPlaceMarkers();
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
    void _savedVersion1CasterIsAskedInThatFormAtOnce();
    void _retryPublicationSuperseded();
    void _missingMountpointDoesNotStartTransport();
    void _correctionIngressKeepsSessionAndIdentity();
    void _ggaSettingsUseInjectedProviders();
    void _connectedMessageSaysWhatTheStreamWaitsFor_data();
    void _connectedMessageSaysWhatTheStreamWaitsFor();
    void _mountpointChangeReconnects();
    void _transportDiagnosticsReachManager();
    void _statusCallbackStopsTransition();
    void _connectedCallbackStopsTransition_data();
    void _connectedCallbackStopsTransition();

    // Failover to the saved casters.
    void _failoverGoesOnAfterThreeFailures_data();
    void _failoverGoesOnAfterThreeFailures();
    void _failoverLeavesAWrongLoginToTheUser();
    void _failoverLeavesOutASavedCasterThatRefuses();
    void _failoverTakesOnlyCastersItCanUse_data();
    void _failoverTakesOnlyCastersItCanUse();
    void _failoverDropsAnotherCastersRetryAfter();
    void _failoverStartsAgainFromTheEnteredCaster_data();
    void _failoverStartsAgainFromTheEnteredCaster();
};
