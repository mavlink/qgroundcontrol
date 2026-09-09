#pragma once

#include "QmlUITestBase.h"

/// UI test for the FlyView camera zoom slider (PhotoVideoControl). Verifies the
/// slider follows a camera-reported zoom level without echoing it back as a new
/// zoom command, and that user interaction with the slider still commands the camera.
class FlyViewCameraZoomUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testZoomSliderTracksCameraWithoutEcho();
};
