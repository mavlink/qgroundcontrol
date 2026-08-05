#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Tests the ProximityRadarValues QML object against a MockLink vehicle with the
/// simulated proximity sensor ring enabled.
class ProximityRadarValuesTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testRadarValues();
};
