#pragma once

#include "MissionTest.h"

/// Tests GeoFenceController support for home-centered circular fences
/// (MAV_CMD_NAV_FENCE_HOME_CIRCLE_INCLUSION) against an ArduPilot MockLink.
class GeoFenceControllerTest : public MissionTest
{
    Q_OBJECT

public:
    explicit GeoFenceControllerTest(QObject* parent = nullptr);

private slots:
    void _testHomeCircleSupportedVersion_data();
    void _testHomeCircleSupportedVersion();
    void _testHomeCircleVehicleRoundTrip();
    void _testHomeCircleFollowsHomeWithoutDirty();
    void _testUploadRefusedWhenHomeCircleUnsupported();
    void _testHomeCircleJsonRoundTrip();
};
