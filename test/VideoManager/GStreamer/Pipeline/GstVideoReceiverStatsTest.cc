#include "GstVideoReceiverStatsTest.h"

#ifdef QGC_GST_STREAMING

#include <memory>

#include <QtCore/QScopeGuard>

#include "GStreamerDecoderRanks.h"
#include "VideoStreamStats.h"
#include "gstqgc/GstQgcTestSupport.h"

namespace {

using GstReceiverTest::kWatchdogTimeout;
using GstReceiverTest::loopbackUri;
using GstReceiverTest::Receiver;
using GstReceiverTest::State;
using GstReceiverTest::udpListenUri;

}  // namespace

void GstVideoReceiverStatsTest::_testStatsMeasureStream_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("stream");
    QTest::addColumn<QString>("decoderFactory");
    QTest::addColumn<bool>("lowLatency");
    QTest::addColumn<QString>("codec");

    const QString h264 = QString::fromLatin1(GstTest::kH264Stream);
    const QString h265 = QString::fromLatin1(GstTest::kH265Stream);
    QTest::newRow("rtp h264") << udpListenUri() << h264 << QStringLiteral("avdec_h264") << false
                              << QStringLiteral("H.264");
    QTest::newRow("rtp h265") << loopbackUri(QLatin1StringView("udp265"), 0) << h265 << QStringLiteral("avdec_h265")
                              << false << QStringLiteral("H.265");
    // Low-latency UDP has no jitter buffer: the source counts the RTP packets itself.
    QTest::newRow("rtp h264 low latency")
        << udpListenUri() << h264 << QStringLiteral("avdec_h264") << true << QStringLiteral("H.264");
}

void GstVideoReceiverStatsTest::_testStatsMeasureStream()
{
    QFETCH(QString, uri);
    QFETCH(QString, stream);
    QFETCH(QString, decoderFactory);
    QFETCH(bool, lowLatency);
    QFETCH(QString, codec);

    const QByteArray softwareDecoder = decoderFactory.toLatin1();
    GSTTEST_SKIP_UNLESS_FACTORIES(softwareDecoder.constData());
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(VideoDecoderOption::Software);

    Receiver receiving(uri, /*autoReconnect=*/false, kWatchdogTimeout, stream);
    QVERIFY2(receiving.sender.error().isEmpty(), qPrintable(receiving.sender.error()));
    VideoReceiver::Config config = receiving.receiver->config();
    config.lowLatency = lowLatency;
    receiving.receiver->setConfig(config);
    QSignalSpy statsSpy(receiving.receiver.get(), &VideoReceiver::statsChanged);
    QVERIFY(!receiving.receiver->stats().valid);

    QVERIFY(receiving.startStreaming());
    QVERIFY(receiving.display());
    // The first sample once streaming is the baseline for rates; a later one measures the frames on display.
    QVERIFY_TRUE_WAIT(receiving.receiver->stats().valid && (receiving.receiver->stats().fps > 0),
                      TestTimeout::mediumMs());

    const VideoStreamStats stats = receiving.receiver->stats();
    QVERIFY(stats.bitrateKbps > 0);
    QCOMPARE(stats.codec, codec);
    QVERIFY(!stats.decoder.isEmpty());
    QCOMPARE(stats.decoder, receiving.receiver->decoderName());
    QVERIFY(!stats.hardwareDecoder);
    // The fixture's receiver displays through the CPU path.
    QCOMPARE(stats.gpuPath, QStringLiteral("CPU"));
    QVERIFY(stats.latencyMs >= 0);
    QVERIFY(stats.hasRtpStats());
    QVERIFY(stats.lossPercent >= 0);
    QVERIFY(stats.lossPercent <= 100);
    QVERIFY(stats.jitterMs >= 0);
    if (lowLatency) {
        // Counted from the sequence numbers of the packets that arrived, all of them on loopback. What only a jitter
        // buffer tells isn't measured.
        QCOMPARE(stats.packetsLost, qint64(0));
        QCOMPARE(stats.lossPercent, 0.0);
        QCOMPARE(stats.packetsLate, qint64(-1));
        QCOMPARE(stats.packetsDuplicate, qint64(-1));
        QCOMPARE(stats.rtxRequested, qint64(-1));
        QCOMPARE(stats.rtxRecovered, qint64(-1));
    } else {
        QVERIFY(stats.packetsLate >= 0);
        QVERIFY(stats.packetsDuplicate >= 0);
        QVERIFY(stats.rtxRequested >= 0);
        QVERIFY(stats.rtxRecovered >= 0);
    }
    // The fixture streams have a keyframe every 5 frames at 30 fps.
    QVERIFY2((stats.keyframeIntervalMs > 100) && (stats.keyframeIntervalMs < 400),
             qPrintable(QString::number(stats.keyframeIntervalMs)));
    QVERIFY(stats.uptimeMs > 0);
    QCOMPARE(stats.reconnects, 0);
    QCOMPARE(stats.decoderRestarts, qint64(0));
    QCOMPARE(stats.gpuFallbacks, qint64(0));
    QCOMPARE(stats.recordingMs, qint64(-1));
    QCOMPARE(stats.recordingBytes, qint64(-1));

    // A stopped stream has nothing to measure.
    receiving.receiver->setActive(false);
    QVERIFY(receiving.waitForState(State::Stopped));
    QVERIFY_TRUE_WAIT(!receiving.receiver->stats().valid, TestTimeout::shortMs());
    QVERIFY(!statsSpy.constLast().constFirst().value<VideoStreamStats>().valid);
    QVERIFY(receiving.errorSpy.isEmpty());
}

void GstVideoReceiverStatsTest::_testFramesLateForTheClockAreDisplayed()
{
    Receiver receiving;
    QVERIFY2(receiving.sender.error().isEmpty(), qPrintable(receiving.sender.error()));
    QVERIFY(receiving.startStreaming());
    QVERIFY(receiving.display());
    QVERIFY_TRUE_WAIT(receiving.receiver->stats().valid, TestTimeout::mediumMs());

    // From here on every frame is due a second before it arrives, as from a decoder that fell behind the clock. The
    // sink shows each one all the same: a GstVideoSink would drop them, and show one a second.
    GStreamer::GstElementPtr sink;
    QVERIFY(runOnPipeline(receiving.receiver.get(), [&sink](GstElement* pipeline) {
        sink = GstTest::findByFactory(pipeline, "qgcqvideosink");
        if (sink) {
            g_object_set(sink.get(), "ts-offset", -static_cast<gint64>(GST_SECOND), nullptr);
        }
    }));
    QVERIFY(sink);
    const guint64 deliveredBefore = GstQgcTest::framesDelivered(sink.get());
    QVERIFY_TRUE_WAIT(GstQgcTest::framesDelivered(sink.get()) >= (deliveredBefore + (2 * GstReceiverTest::kFrameRate)),
                      TestTimeout::mediumMs());
    QVERIFY(receiving.errorSpy.isEmpty());
}

void GstVideoReceiverStatsTest::_testStatsCountFramesTheDisplayQueueLeaks()
{
    Receiver receiving;
    QVERIFY2(receiving.sender.error().isEmpty(), qPrintable(receiving.sender.error()));
    receiving.window.show();
    receiving.receiver->setVideoOutput(receiving.output);
    const auto gate = std::make_shared<GstReceiverTest::Gate>();
    // Before the receiver stops: its worker waits for the streaming thread the gate holds.
    const auto release = qScopeGuard([&gate] { gate->release(); });
    // A display that has stopped taking frames.
    QVERIFY(GstReceiverTest::holdAtSink(displaySink(receiving.receiver.get()), gate));
    QVERIFY(receiving.startStreaming());

    // The decoder can't hand its frame on, so the stream backs up into the display queue, which leaks rather than
    // hold up the tee: two seconds of frames are far more than decodebin3 buffers ahead of its decoder.
    QVERIFY(receiving.framesSent(2 * GstReceiverTest::kFrameRate));
    gate->release();
    QVERIFY_TRUE_WAIT(receiving.receiver->stats().framesDropped >= (GstReceiverTest::kFrameRate / 2),
                      TestTimeout::mediumMs());
    QVERIFY(receiving.errorSpy.isEmpty());
}

void GstVideoReceiverStatsTest::_testStatsCountLostPacketsWithoutJitterBuffer()
{
    // One buffer or buffer list of RTP packets in seven never arrives.
    constexpr int kDropOneIn = 7;

    Receiver receiving;
    QVERIFY2(receiving.sender.error().isEmpty(), qPrintable(receiving.sender.error()));
    VideoReceiver::Config config = receiving.receiver->config();
    config.lowLatency = true;
    receiving.receiver->setConfig(config);

    QVERIFY(receiving.startStreaming());
    receiving.sender.dropPackets(kDropOneIn);
    // The first sample is the baseline; each later one tells the loss of its interval.
    QVERIFY_TRUE_WAIT(receiving.receiver->stats().lossPercent > 0, TestTimeout::mediumMs());

    const VideoStreamStats stats = receiving.receiver->stats();
    QVERIFY(stats.valid);
    QVERIFY(stats.packetsLost > 0);
    QVERIFY2(stats.lossPercent < 100, qPrintable(QString::number(stats.lossPercent)));
    QVERIFY(receiving.errorSpy.isEmpty());
}

#endif

UT_REGISTER_TEST(GstVideoReceiverStatsTest, TestLabel::Integration)
