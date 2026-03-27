#pragma once

#include "UnitTest.h"

// Component-level tests for GPSRtk device orchestration using MockGPSRtk.
// Exercises device lifecycle, fact-group updates, and RTCM routing to mock
// devices. For full-pipeline tests (survey-in -> RTCM -> MAVLink) see
// RTKEndToEndTest.
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
