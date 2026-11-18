#pragma once

#include "QmlUITestBase.h"

class MavlinkCameraControlInterface;

/// UI tests for the camera zoom in the Fly View. The zoom slider (PhotoVideoControl) follows a camera-reported zoom
/// level without echoing it back as a new zoom command, and user interaction with the slider still commands the
/// camera. Two fingers pinching the full-size video step the zoom, while one finger is still a click or a drag there
/// and the touches on other controls stay theirs.
class FlyViewCameraZoomUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testZoomSliderTracksCameraWithoutEcho();
    void _testPinchOnFullSizeVideoStepsZoom();

private:
    /// Makes MockLink's zoom camera the current camera of @p vehicle, once it has announced itself, and returns it in
    /// @p camera. Fails the test otherwise.
    void _selectZoomCamera(Vehicle* vehicle, MavlinkCameraControlInterface*& camera);
};
