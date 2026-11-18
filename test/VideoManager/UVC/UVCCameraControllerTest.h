#pragma once

#include "UnitTest.h"

class UVCCameraControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testScreenshotFromVideoOutput();
    void _testOutputFollowsTransform();
    void _testPhotoFollowsTransform();
    void _testPhotoWithoutCameraIsScreenshot();
    void _testMissingCameraNeverRuns();
    void _testControlsWithoutCamera();
    void _testUnpluggedCameraReported();
    void _testCameraErrorRetries();
    void _testRecordingNeedsRunningCamera();
    void _testStartWhileRecordingIsRefused();
};
