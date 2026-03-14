#pragma once

#include "UnitTest.h"

class RTKIntegrationTest : public UnitTest
{
    Q_OBJECT

private slots:
    // Mock device lifecycle
    void testMockConnectDisconnect();
    void testMockFactGroupUpdates();
    void testMockSurveyInProgress();

    // RTCM routing with mock
    void testRtcmRoutesToMockDevice();
    void testRtcmRoutesToMultipleMocks();
    void testRemoveDeviceStopsRouting();

    // Position simulation
    void testSimulatePosition();
    void testSimulateSatellites();
};
