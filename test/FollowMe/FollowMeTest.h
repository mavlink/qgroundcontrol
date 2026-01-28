#pragma once

#include "TestFixtures.h"

/// Tests for FollowMe functionality.
/// Tests GCSMotionReport struct, MotionCapability enum, timer constants, and integration.
class FollowMeTest : public VehicleTestNoConnect
{
    Q_OBJECT

public:
    FollowMeTest() = default;

private slots:
    // GCSMotionReport struct tests
    void _testGCSMotionReportDefaults();
    void _testGCSMotionReportLatLonInt();
    void _testGCSMotionReportAltitude();
    void _testGCSMotionReportHeading();
    void _testGCSMotionReportVelocity();
    void _testGCSMotionReportPosStdDev();

    // MotionCapability enum tests
    void _testMotionCapabilityPos();
    void _testMotionCapabilityVel();
    void _testMotionCapabilityAccel();
    void _testMotionCapabilityAttRates();
    void _testMotionCapabilityHeading();
    void _testMotionCapabilityBitmask();

    // FollowMe instance tests
    void _testFollowMeInstance();
    void _testFollowMeInstanceSingleton();

    // Timer constant tests
    void _testMotionUpdateInterval();

    // Integration tests
    void _testFollowMeInit();
    void _testFollowMeWithVehicle();
};
