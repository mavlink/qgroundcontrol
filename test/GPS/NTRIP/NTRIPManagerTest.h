#pragma once

#include "UnitTest.h"

/// Tests observable NTRIP state transitions and reconnect behavior.
class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void testInitialStateIsDisconnected();
    void testStopFromIdleIsNoop();
    void testStopCancelsDeferredSettings_data();
    void testStopCancelsDeferredSettings();
    void testNewSessionRetryBudget_data();
    void testNewSessionRetryBudget();
    void testPlaintextCredentialWarningIsVisibleState();
    void testConfigurationDebugRedactsCredentials();

    // Reconnect backoff (migrated from NTRIPReconnectPolicyTest after the policy
    // was inlined into NTRIPManager). Driven through public state and ManualScheduler.
    void testReconnectInitialBackoff();
    void testReconnectExponentialBackoff();
    void testReconnectMaxBackoff();
    void testReconnectCancelStopsTimer();
    void testReconnectResetAttempts();
    void testReconnectSignalFires();
    void testReconnectWaitsForNetworkAtFailure();
    void testReconnectWaitsWhenNetworkLostDuringBackoff();
    void testLoopbackCasterBypassesNetworkGate_data();
    void testLoopbackCasterBypassesNetworkGate();
    void testWaitingStatusObserverCanStopManager();
    void testDuplicateTransportErrorsScheduleOneRetry();
    void testRetiredTransportErrorCannotAffectNewSession();
    void testRetryPolicy_data();
    void testRetryPolicy();
    void testHttpRetryAfterReachesManager();
    void testHttpRetryAfterReachesManager_data();
    void testRetryPublicationSuperseded_data();
    void testRetryPublicationSuperseded();
    void testMissingMountpointDoesNotStartTransport();
    void testCorrectionIngressKeepsSessionAndIdentity();
    void testGgaSettingsUseInjectedProviders();
    void testFactChangesReconfigureTransport_data();
    void testFactChangesReconfigureTransport();
    void testTransportDiagnosticsReachManager();
    void testStatusCallbackStopsTransition();
    void testConnectedCallbackStopsTransition();
};
