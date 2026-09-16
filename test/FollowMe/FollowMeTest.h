#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class FollowMeTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testFollowMe();
    void _motionPolicyReports_data();
    void _motionPolicyReports();
};
