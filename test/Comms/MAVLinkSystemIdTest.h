#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class MAVLinkSystemIdTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _connectWideVehicle_data();
    void _connectWideVehicle();
    void _distinctSystems();
};
