#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class VehicleAirborneTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _airborneOnlyForAircraft_data();
    void _airborneOnlyForAircraft();
};
