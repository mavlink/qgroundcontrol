#pragma once

#include "UnitTest.h"

class UVCCameraController;

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
    void _testFramesBecomeStats();
    void _testStatsRestartWithoutOutput();
    void _testStatsGroupShowsCamera();

private:
    /// Counts @p count frames of a 640 × 480 MJPEG camera set up for 30 fps, as @p controller does for those it shows.
    static void _countCameraFrames(UVCCameraController& controller, int count);
    /// A baseline sample at @p nowMs, then a second of 30 frames and their sample.
    static void _measureSecond(UVCCameraController& controller, qint64& nowMs);
};
