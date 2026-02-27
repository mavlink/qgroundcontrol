#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class MAVLinkLogManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testInitMAVLinkLogManager();
};
