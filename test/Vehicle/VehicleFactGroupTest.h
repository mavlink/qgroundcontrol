#pragma once

#include "BaseClasses/VehicleTest.h"

class VehicleFactGroupTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _attitudeQuaternionReprOffsetRates_data();
    void _attitudeQuaternionReprOffsetRates();
};
