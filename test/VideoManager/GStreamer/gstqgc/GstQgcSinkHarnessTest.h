#pragma once

#include "GStreamerTestBase.h"

/// qgcvideosinkbin under GstHarness (gst-check), as the receiver binds it to a QVideoSink: synthetic frames through the
/// bin's converter, delivered only when the test runs the GUI thread's queued calls.
class GstQgcSinkHarnessTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testBinRenegotiatesMidStream();
    void _testBinAppliesOrientationTag_data();
    void _testBinAppliesOrientationTag();
};
