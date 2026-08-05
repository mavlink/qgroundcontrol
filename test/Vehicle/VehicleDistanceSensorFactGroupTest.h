#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Tests DISTANCE_SENSOR handling in VehicleDistanceSensorFactGroup using the
/// MockLink simulated proximity sensor ring (MockConfiguration::OptionEnableProximity).
class VehicleDistanceSensorFactGroupTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testProximityTelemetry();
    void _testProximityValuesUpdate();
    void _testNoProximityWithoutOption();

private:
    void _startVehicle(MockConfiguration::Options options);
};
