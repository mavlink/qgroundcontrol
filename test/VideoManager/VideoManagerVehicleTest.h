#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// VideoManager with a vehicle's cameras: the streams they advertise, and what the cameras are asked to transmit.
class VideoManagerVehicleTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testTilesFollowCameraStreams();
};
