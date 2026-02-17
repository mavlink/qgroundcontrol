#pragma once

#include "BaseClasses/VehicleTest.h"

class InitialConnectTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _performTestCases();
    void _boardVendorProductId();
};
