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
    void testPlaintextCredentialWarningIsVisibleState();
    void testTerminalStateStopsUdpForwarder_data();
    void testTerminalStateStopsUdpForwarder();

    // Reconnect backoff (migrated from NTRIPReconnectPolicyTest after the policy
    // was inlined into NTRIPManager). Driven through the friend test seam.
    void testReconnectInitialBackoff();
    void testReconnectExponentialBackoff();
    void testReconnectMaxBackoff();
    void testReconnectCancelStopsTimer();
    void testReconnectResetAttempts();
    void testReconnectSignalFires();
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
    void testSettingsProduceExplicitConfiguration();
    void testGgaSettingsUseInjectedProviders();
    void testFactChangesReconfigureTransport_data();
    void testFactChangesReconfigureTransport();
    void testNtripOnlyUdpForwardingBypassesSelectionOnce();
    void testTransportDiagnosticsReachManager();
    void testStatusCallbackStopsTransition();
    void testCasterCallbackStopsTransition();
};
