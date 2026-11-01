#include "GstVideoReceiverStatsTest.h"

#ifdef QGC_GST_STREAMING

#include "GStreamerDecoderRanks.h"
#include "VideoStreamStats.h"

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
    // Low-latency UDP has no jitter buffer, which is what measures RTP loss and jitter.
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
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

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
    QCOMPARE(stats.hasRtpStats(), !lowLatency);
    if (stats.hasRtpStats()) {
        QVERIFY(stats.lossPercent >= 0);
        QVERIFY(stats.lossPercent <= 100);
        QVERIFY(stats.jitterMs >= 0);
        QVERIFY(stats.packetsLate >= 0);
        QVERIFY(stats.packetsDuplicate >= 0);
        QVERIFY(stats.rtxRequested >= 0);
        QVERIFY(stats.rtxRecovered >= 0);
    } else {
        QCOMPARE(stats.lossPercent, -1.0);
        QCOMPARE(stats.jitterMs, -1.0);
        QCOMPARE(stats.packetsLate, qint64(-1));
        QCOMPARE(stats.rtxRequested, qint64(-1));
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

#endif

UT_REGISTER_TEST(GstVideoReceiverStatsTest, TestLabel::Integration)
