#pragma once

#include "QmlUITestBase.h"

/// UI test for the FlyView shutter button (PhotoVideoControl) in timelapse mode.
/// Verifies that a second click stops an interval capture (issue #15064).
class FlyViewCameraCaptureUITest : public QmlUITestBase
{
    Q_OBJECT

private slots:
    void _testTimelapseShutterStopsCapture();
};
