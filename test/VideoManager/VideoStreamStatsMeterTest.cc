#include "VideoStreamStatsMeterTest.h"

#include "VideoStreamStats.h"
#include "VideoStreamStatsMeter.h"

namespace {

VideoStreamStatsMeter::Counters meterCounters(quint64 framesDisplayed, quint64 bytesReceived, quint64 framesDropped = 0)
{
    VideoStreamStatsMeter::Counters counters;
    counters.framesDisplayed = framesDisplayed;
    counters.bytesReceived = bytesReceived;
    counters.framesDropped = framesDropped;
    return counters;
}

VideoStreamStatsMeter::Counters meterRtpCounters(quint64 packetsReceived, quint64 packetsLost)
{
    VideoStreamStatsMeter::Counters counters;
    counters.rtp = VideoStreamStatsMeter::RtpCounters{.packetsReceived = packetsReceived, .packetsLost = packetsLost};
    return counters;
}

}  // namespace

void VideoStreamStatsMeterTest::_testFirstSampleIsBaseline()
{
    qint64 nowMs = 1000;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    stats.decoder = QStringLiteral("avdec_h264");

    QVERIFY(!meter.update(meterCounters(10, 5000, 2), stats));
    QVERIFY(!stats.valid);
    // A sample no later than the previous one measures nothing.
    QVERIFY(!meter.update(meterCounters(12, 6000, 2), stats));
    QVERIFY(!stats.valid);

    nowMs += 1000;
    QVERIFY(meter.update(meterCounters(40, 130000, 3), stats));
    QVERIFY(stats.valid);
    QCOMPARE(stats.fps, 30.0);
    QCOMPARE(stats.bitrateKbps, 1000.0);
    // Totals count from the start of the stream, the drops before the baseline included; the rate only the interval's.
    QCOMPARE(stats.framesDropped, qint64(3));
    QCOMPARE(stats.droppedFps, 1.0);
    QVERIFY(!stats.hasRtpStats());
    QCOMPARE(stats.packetsLost, qint64(-1));
    QCOMPARE(stats.lossPercent, -1.0);
    // What the meter doesn't measure is left alone.
    QCOMPARE(stats.decoder, QStringLiteral("avdec_h264"));
}

void VideoStreamStatsMeterTest::_testRatesOverInterval_data()
{
    QTest::addColumn<qint64>("intervalMs");
    QTest::addColumn<quint64>("frames");
    QTest::addColumn<quint64>("bytes");
    QTest::addColumn<double>("fps");
    QTest::addColumn<double>("kbps");

    QTest::newRow("one second") << qint64(1000) << quint64(30) << quint64(125000) << 30.0 << 1000.0;
    QTest::newRow("half a second") << qint64(500) << quint64(15) << quint64(62500) << 30.0 << 1000.0;
    QTest::newRow("late tick") << qint64(1250) << quint64(30) << quint64(250000) << 24.0 << 1600.0;
    QTest::newRow("stalled") << qint64(1000) << quint64(0) << quint64(0) << 0.0 << 0.0;
}

void VideoStreamStatsMeterTest::_testRatesOverInterval()
{
    QFETCH(qint64, intervalMs);
    QFETCH(quint64, frames);
    QFETCH(quint64, bytes);
    QFETCH(double, fps);
    QFETCH(double, kbps);

    qint64 nowMs = 0;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    QVERIFY(!meter.update(meterCounters(100, 1000000), stats));
    nowMs += intervalMs;
    QVERIFY(meter.update(meterCounters(100 + frames, 1000000 + bytes), stats));
    QCOMPARE(stats.fps, fps);
    QCOMPARE(stats.bitrateKbps, kbps);
}

void VideoStreamStatsMeterTest::_testRtpLoss()
{
    qint64 nowMs = 0;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    QVERIFY(!meter.update(meterRtpCounters(100, 2), stats));

    nowMs += 1000;
    QVERIFY(meter.update(meterRtpCounters(190, 12), stats));
    QVERIFY(stats.hasRtpStats());
    QCOMPARE(stats.lossPercent, 10.0);
    QCOMPARE(stats.packetsLost, qint64(12));

    // Loss is the last interval's; the lost packets are counted from the start of the stream.
    nowMs += 1000;
    QVERIFY(meter.update(meterRtpCounters(290, 12), stats));
    QCOMPARE(stats.lossPercent, 0.0);
    QCOMPARE(stats.packetsLost, qint64(12));

    // No packet due in the interval is no loss.
    nowMs += 1000;
    QVERIFY(meter.update(meterRtpCounters(290, 12), stats));
    QCOMPARE(stats.lossPercent, 0.0);
}

void VideoStreamStatsMeterTest::_testCounterRestartKeepsTotalsGrowing()
{
    qint64 nowMs = 0;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    VideoStreamStatsMeter::Counters counters = meterRtpCounters(500, 4);
    counters.framesDisplayed = 100;
    counters.bytesReceived = 1000;
    counters.framesDropped = 5;
    QVERIFY(!meter.update(counters, stats));

    nowMs += 1000;
    counters = meterRtpCounters(600, 6);
    counters.framesDisplayed = 130;
    counters.bytesReceived = 126000;
    counters.framesDropped = 7;
    QVERIFY(meter.update(counters, stats));
    QCOMPARE(stats.framesDropped, qint64(7));
    QCOMPARE(stats.droppedFps, 2.0);
    QCOMPARE(stats.packetsLost, qint64(6));

    // The counting elements were replaced: their counters start again from zero.
    nowMs += 1000;
    counters = meterRtpCounters(50, 1);
    counters.framesDisplayed = 20;
    counters.bytesReceived = 62500;
    counters.framesDropped = 1;
    QVERIFY(meter.update(counters, stats));
    // The rates average both intervals: 30 + 20 frames and 125000 + 62500 bytes in 2 s.
    QCOMPARE(stats.fps, 25.0);
    QCOMPARE(stats.bitrateKbps, 750.0);
    QCOMPARE(stats.framesDropped, qint64(8));
    QCOMPARE(stats.droppedFps, 1.0);
    QCOMPARE(stats.packetsLost, qint64(7));
    QCOMPARE(stats.lossPercent, 100.0 / 51.0);
}

void VideoStreamStatsMeterTest::_testResetStartsNewStream()
{
    qint64 nowMs = 0;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    QVERIFY(!meter.update(meterRtpCounters(0, 0), stats));
    nowMs += 1000;
    VideoStreamStatsMeter::Counters counters = meterRtpCounters(100, 3);
    counters.framesDropped = 5;
    QVERIFY(meter.update(counters, stats));
    QCOMPARE(stats.framesDropped, qint64(5));

    // The next stream counts from zero, and needs a baseline again.
    meter.reset();
    nowMs += 1000;
    counters = meterRtpCounters(10, 0);
    counters.framesDisplayed = 10;
    counters.framesDropped = 1;
    QVERIFY(!meter.update(counters, stats));
    nowMs += 1000;
    counters = meterRtpCounters(40, 0);
    counters.framesDisplayed = 40;
    counters.framesDropped = 1;
    QVERIFY(meter.update(counters, stats));
    QCOMPARE(stats.fps, 30.0);
    QCOMPARE(stats.framesDropped, qint64(1));
    QCOMPARE(stats.packetsLost, qint64(0));
}

void VideoStreamStatsMeterTest::_testRatesAverageOverWindow()
{
    qint64 nowMs = 0;
    VideoStreamStatsMeter meter([&nowMs] { return nowMs; });
    VideoStreamStats stats;
    quint64 frames = 0;
    quint64 dropped = 0;
    QVERIFY(!meter.update(meterCounters(frames, 0, dropped), stats));

    // A steady 15 fps stream whose frames arrive in bursts, then one that speeds up. From the fourth sample on, the
    // window no longer holds the first interval.
    const struct
    {
        quint64 frames;
        quint64 dropped;
        double fps;
        double droppedFps;
    } samples[] = {
        {12, 0, 12.0, 0.0}, {18, 0, 15.0, 0.0}, {15, 0, 15.0, 0.0}, {15, 0, 16.0, 0.0}, {30, 2, 20.0, 2.0},
    };

    for (const auto& sample : samples) {
        nowMs += 1000;
        frames += sample.frames;
        dropped += sample.dropped;
        QVERIFY(meter.update(meterCounters(frames, 0, dropped), stats));
        QCOMPARE(stats.fps, sample.fps);
        QCOMPARE(stats.droppedFps, sample.droppedFps);
    }
}

UT_REGISTER_TEST(VideoStreamStatsMeterTest, TestLabel::Unit)
