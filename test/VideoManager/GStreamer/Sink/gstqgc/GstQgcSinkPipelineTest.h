#pragma once

#include "GStreamerTestBase.h"

/// videotestsrc pipelines played to EOS into the sink bin bound to a QVideoSink, as the receiver binds it.
class GstQgcSinkPipelineTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testFrameDelivery();
    void _testPtsAndColorimetry();
    void _testGpuZeroCopyFallback_data();
    void _testGpuZeroCopyFallback();
    void _testQgcVideoSinkBinScalesNonSquarePixels_data();
    void _testQgcVideoSinkBinScalesNonSquarePixels();
    void _testTeardownUnderLoad();
    void _testInactiveQgcQVideoSinkDropsAndCounts();
    void _testLateFramesAreShown();
};
