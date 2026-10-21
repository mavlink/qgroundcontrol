#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies that PX4FirmwarePlugin::guidedModeTakeoff arms the vehicle only in response to the
/// MAV_CMD_NAV_TAKEOFF it sent itself.
class PX4TakeoffTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4TakeoffTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _takeoffAfterRejectedTakeoff_data();
    void _takeoffAfterRejectedTakeoff();
    void _repeatedTakeoffWhilePendingArmsOnce();

private:
    void _prepareTakeoff();
    void _waitForTakeoffResult();
    void _verifyArmCommandCount(int expectedCount);
};
