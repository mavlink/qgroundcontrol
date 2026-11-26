#pragma once

#include "GStreamerTestBase.h"

/// Measures how long a frame takes from the sender to the output's QVideoSink, for each receiver: GStreamer's, Qt
/// Multimedia's and the experimental FFmpeg one, each in its default and its low-latency mode. A GStreamer pipeline in
/// the test writes the time into each frame's picture just before x264enc (zerolatency) encodes it, and sends it as
/// H.264 RTP over UDP on loopback; the test reads the time back from each frame the sink is given, on the thread that
/// gives it. Reports, on standard output and to the file QGC_DELAY_RESULTS names, the time to the first frame and the
/// median delay of the first second of six starts, and the median and 95th percentile of the delay over the last
/// start's next 300 frames.
///
/// The decoders are the receivers' own choice, hardware where they find it; QGC_DELAY_DECODING=software forces
/// software decoding, which Qt Multimedia's player takes from QT_FFMPEG_DECODING_HW_DEVICE_TYPES set empty as the
/// process starts. The checks are only that the frames arrive and carry readable times: the numbers depend on the
/// machine and its load.
class VideoReceiverDelayTest : public GStreamerTestBase
{
    Q_OBJECT

private slots:
    void cleanup() override;
    void _testDelay_data();
    void _testDelay();
};
