#pragma once

#include "QmlUITestBase.h"

/// UI test that boots the full QML UI with a PX4 MockLink vehicle, opens the
/// 3D View from the Fly View tool strip, and verifies the vehicle-related
/// pieces: the drone model tracks real vehicle telemetry, the scale bar is
/// shown, and camera input handlers drive the camera controller.
class Viewer3DUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _test3DViewShowsVehicle();
    void _test3DViewCameraGestures();
};
