#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies gripper detection against PX4 v1.17+ parameters where PD_GRIPPER_EN was removed
/// and enablement is expressed via PD_GRIPPER_TYPE >= 0 (issue #14930).
class PX4HasGripperTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit PX4HasGripperTest(QObject* parent = nullptr) : VehicleTest(parent) { setWaitForParameters(true); }

private slots:
    void _gripperDetectedFromGripperType();
};
