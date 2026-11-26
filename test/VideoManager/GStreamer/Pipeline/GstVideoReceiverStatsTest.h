#pragma once

#include "GstVideoReceiverTestBase.h"

/// The stream statistics GstVideoReceiver measures from real H.264 and H.265 streams.
class GstVideoReceiverStatsTest : public GstVideoReceiverTestBase
{
    Q_OBJECT

private slots:
    void _testStatsMeasureStream_data();
    void _testStatsMeasureStream();
    void _testFramesLateForTheClockAreDisplayed();
    void _testStatsCountFramesTheDisplayQueueLeaks();
    void _testStatsCountLostPacketsWithoutJitterBuffer();
};
