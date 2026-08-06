#pragma once

#include "UnitTest.h"

/// State-transition smoke tests for the NTRIP manager singleton.
///
/// The detailed GGA formatting suite that used to live here has been moved to
/// NTRIPGgaProviderTest — that is where the implementation of makeGGA lives and
/// where future sentence-format cases should go. This file focuses on the
/// NTRIPManager's observable state machine (connectionStatus, casterStatus)
/// rather than downstream sentence encoding.
class NTRIPManagerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void testInitialStateIsDisconnected();
    void testStopFromIdleIsNoop();
    void testPlaintextCredentialWarningIsVisibleState();
    void testErrorStateStopsUdpForwarder();

    // Reconnect backoff (migrated from NTRIPReconnectPolicyTest after the policy
    // was inlined into NTRIPManager). Driven through the friend test seam.
    void testReconnectInitialBackoff();
    void testReconnectExponentialBackoff();
    void testReconnectMaxBackoff();
    void testReconnectCancelStopsTimer();
    void testReconnectResetAttempts();
    void testReconnectSignalFires();
};
