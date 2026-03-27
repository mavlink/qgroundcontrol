#pragma once

#include "UnitTest.h"

// Component-level tests for NTRIPManager's public API using a mock transport.
// Exercises lifecycle, error emission, and signal flow between NTRIPManager
// and its collaborators. For full-pipeline tests against a real TCP caster
// see NTRIPEndToEndTest.
class NTRIPIntegrationTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Connection lifecycle
    void testConnectDisconnect();
    void testAutoConnectEmitsStatus();
    void testStopCleansUpTransport();

    // Error handling
    void testTransportErrorTriggersReconnect();
    void testSslErrorReported();
    void testAuthFailedReported();

    // RTCM data flow
    void testRtcmFlowsToRouter();
    void testRtcmUpdatesStats();

    // GGA flow
    void testGgaSentAfterConnect();

    // Settings
    void testInvalidConfigRejectsStart();
};
