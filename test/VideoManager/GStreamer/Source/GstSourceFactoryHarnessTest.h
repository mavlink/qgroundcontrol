#pragma once

#include "GStreamerTestBase.h"

/// GStreamer::SourceFactory's pieces under GstHarness (gst-check), fed RTP payloaded from the encoded test streams.
class GstSourceFactoryHarnessTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void _testParameterSetGateOnDepayloadedStream_data();
    void _testParameterSetGateOnDepayloadedStream();
};
