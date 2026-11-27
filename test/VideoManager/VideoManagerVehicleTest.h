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
    void _testThermalStreamOfAnyCameraIsShown_data();
    void _testThermalStreamOfAnyCameraIsShown();
    void _testThermalCameraIsMainPictureWhileCurrent();

private:
    /// Makes the mock vehicle's first camera the current one, once its two streams, which share an address, are
    /// announced. Its second camera has none. Null if they aren't in time.
    MavlinkCameraControlInterface* _selectStreamingCamera();
    /// Connects a mock vehicle whose cameras advertise @p thermalStream beside the first camera's visible-light
    /// stream, and makes that camera the current one. False if the streams aren't announced in time.
    bool _connectThermalCameras(MockConfiguration::CameraThermalStream thermalStream);
    /// The mock vehicle's camera @p compId; null without one.
    MavlinkCameraControlInterface* _camera(int compId) const;
};
