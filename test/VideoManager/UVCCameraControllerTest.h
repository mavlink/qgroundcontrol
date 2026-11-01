#pragma once

#include "UnitTest.h"

class UVCCameraControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testScreenshotFromVideoOutput();
    void _testScreenshotFollowsOutputTransform();
    void _testPhotoFollowsOutputTransform();
    void _testPhotoWithoutCameraIsScreenshot();
    void _testMissingCameraNeverRuns();
    void _testUnpluggedCameraReported();
    void _testCameraErrorRetries();
    void _testRecordingFormat_data();
    void _testRecordingFormat();
    void _testRecordingFormatTriesCodecFirst();
    void _testRecordingCodecs();
    void _testRecordingNeedsRunningCamera();
};
