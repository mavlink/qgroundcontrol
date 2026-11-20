#pragma once

#include "GStreamerTestBase.h"

/// QtMultimediaVideoReceiver against MockVideoStreamServer's streams, which need GStreamer to serve them: the kinds of
/// stream it plays, and what it does differently from the GStreamer receiver. What the two have in common is
/// VideoReceiverContractTest's.
class QtMultimediaVideoReceiverStreamTest : public GStreamerTestBase
{
    Q_OBJECT

private:
    void _ignorePlayerLogs();
    void _ignoreEncoderFallbacks();
    void _ignoreRtspServerLogs();

private slots:
    void _testPlaysMockStreams_data();
    void _testPlaysMockStreams();
    void _testDetachingOutputDoesNotBlock();
    void _testPlaysRtspOverTcp();
    void _testRecordsStream();
};
