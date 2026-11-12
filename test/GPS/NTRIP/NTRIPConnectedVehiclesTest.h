#pragma once

#include "BaseClasses/VehicleTest.h"

/// What the application tells an NTRIP connection that runs only while a vehicle is connected.
class NTRIPConnectedVehiclesTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _followsTheConnectedVehicles();
};
