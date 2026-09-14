#pragma once

#include "GimbalControllerTestBase.h"

/// End-to-end tests for GIMBAL_DEVICE_ATTITUDE_STATUS delta_yaw handling through MockLink.
class GimbalControllerDeltaYawTest : public GimbalControllerTestBase
{
    Q_OBJECT

private slots:
    void _testHeadingSource_data();
    void _testHeadingSource();
    void _testDeltaYawTracksChanges();
    void _testVehicleHeadingIgnoredWhenDeltaYawValid();
    void _testDeltaYawAvailabilityIsPerMessage();
    void _testOnScreenControlYawLockUsesAbsoluteYaw_data();
    void _testOnScreenControlYawLockUsesAbsoluteYaw();
};
