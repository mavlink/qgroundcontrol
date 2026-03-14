#pragma once

#include "UnitTest.h"

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
