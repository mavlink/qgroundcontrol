#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory's parameter-set gate under GstHarness (gst-check): on a pad fed NAL units by hand, and on
/// a depayloader fed RTP payloaded from the encoded test streams.
class GstSourceFactoryHarnessTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testParameterSetGateHoldsUntilCompleteSet();
    void _testParameterSetGatePassesLengthPrefixedOutput();
    void _testParameterSetGateOnDepayloadedStream_data();
    void _testParameterSetGateOnDepayloadedStream();
};
