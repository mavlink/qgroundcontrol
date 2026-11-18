#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

class MavlinkCameraControlInterface;

/// VideoManager with a vehicle's cameras: the streams they advertise, and what the cameras are asked to transmit.
class VideoManagerVehicleTest : public VehicleTestManualConnect
{
    Q_OBJECT

private slots:
    void _testTilesFollowCameraStreams();
    void _testVideoOffStopsCameraStream();

private:
    /// Makes the mock vehicle's first camera the current one, once its two streams, which share an address, are
    /// announced. Its second camera has none. Null if they aren't in time.
    MavlinkCameraControlInterface* _selectStreamingCamera();
};
