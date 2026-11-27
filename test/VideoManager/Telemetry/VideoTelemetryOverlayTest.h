#pragma once

#include "BaseClasses/VehicleTest.h"

class VideoTelemetryOverlayTest : public VehicleTestNoInitialConnect
{
    Q_OBJECT

private slots:
    void _testFollowsSettingAndVehicle();
    void _testFollowsTelemetryBarEdits();
};
