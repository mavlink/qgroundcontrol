#pragma once

#include "UnitTest.h"

class UVCCameraControllerTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _testScreenshotFromVideoOutput();
    void _testMissingCameraNeverRuns();
    void _testUnpluggedCameraReported();
    void _testCameraErrorReported();
};
