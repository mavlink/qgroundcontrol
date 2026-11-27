#include "StreamWatchdogTest.h"

#include "StreamWatchdog.h"

namespace {

// Any monotonic time well clear of 0, which the watchdog reads as "no frame yet".
constexpr qint64 kWatchdogNowMs = 100000;
constexpr qint64 kWatchdogNoFrame = -1;

}  // namespace

void StreamWatchdogTest::_testStallDetection_data()
{
    QTest::addColumn<uint>("timeoutSec");
    QTest::addColumn<qint64>("sourceFrameAgeMs");
    QTest::addColumn<qint64>("sinkFrameAgeMs");
    QTest::addColumn<qint64>("withheldFrameAgeMs");
    QTest::addColumn<bool>("decoding");
    QTest::addColumn<int>("expected");

    const int none = int(StreamWatchdog::Stall::None);
    const int source = int(StreamWatchdog::Stall::Source);
    const int decoder = int(StreamWatchdog::Stall::Decoder);

    QTest::newRow("source within timeout") << 2u << qint64(2000) << qint64(0) << kWatchdogNoFrame << true << none;
    QTest::newRow("source past timeout") << 2u << qint64(2001) << qint64(0) << kWatchdogNoFrame << true << source;
    QTest::newRow("zero timeout floors at 1 s, within")
        << 0u << qint64(1000) << kWatchdogNoFrame << kWatchdogNoFrame << false << none;
    QTest::newRow("zero timeout floors at 1 s, past")
        << 0u << qint64(1001) << kWatchdogNoFrame << kWatchdogNoFrame << false << source;
    QTest::newRow("sink within twice timeout") << 2u << qint64(0) << qint64(4000) << kWatchdogNoFrame << true << none;
    QTest::newRow("sink past twice timeout") << 2u << qint64(0) << qint64(4001) << kWatchdogNoFrame << true << decoder;
    QTest::newRow("sink silence ignored when not decoding")
        << 2u << qint64(0) << qint64(60000) << kWatchdogNoFrame << false << none;
    QTest::newRow("source stall reported first")
        << 2u << qint64(3000) << qint64(60000) << kWatchdogNoFrame << true << source;
    // The keyframe gate may keep the stream from the decoder for as long as the shortest timeout gives the decoder.
    QTest::newRow("frames kept from the decoder are not its silence")
        << 1u << qint64(0) << qint64(2500) << qint64(500) << true << none;
    QTest::newRow("sink past twice timeout since the last frame kept from the decoder")
        << 1u << qint64(0) << qint64(4500) << qint64(2001) << true << decoder;
}

void StreamWatchdogTest::_testStallDetection()
{
    QFETCH(uint, timeoutSec);
    QFETCH(qint64, sourceFrameAgeMs);
    QFETCH(qint64, sinkFrameAgeMs);
    QFETCH(qint64, withheldFrameAgeMs);
    QFETCH(bool, decoding);
    QFETCH(int, expected);

    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(timeoutSec));
    (void) watchdog.noteSourceFrame(kWatchdogNowMs - sourceFrameAgeMs);
    if (sinkFrameAgeMs != kWatchdogNoFrame) {
        (void) watchdog.noteSinkFrame(kWatchdogNowMs - sinkFrameAgeMs);
    }
    if (withheldFrameAgeMs != kWatchdogNoFrame) {
        watchdog.excuseSinkSilence(kWatchdogNowMs - withheldFrameAgeMs);
    }

    QCOMPARE(int(watchdog.check(kWatchdogNowMs, decoding).stall), expected);
}

void StreamWatchdogTest::_testFirstFrameGrace_data()
{
    QTest::addColumn<qint64>("graceMs");        // kWatchdogNoFrame: none, the first frame may take any time
    QTest::addColumn<qint64>("silentStallMs");  // kWatchdogNoFrame: never

    // RTSP's UDP to TCP fallback must not be cut short before the first frame.
    QTest::newRow("rtsp fallback") << qint64(5000) << qint64(6001);
    // A UDP port nobody sends to yet hasn't stalled.
    QTest::newRow("listening") << kWatchdogNoFrame << kWatchdogNoFrame;
    // A source that never delivers a frame must still time out, counted from the first check.
    QTest::newRow("no grace") << qint64(0) << qint64(1001);
}

void StreamWatchdogTest::_testFirstFrameGrace()
{
    QFETCH(qint64, graceMs);
    QFETCH(qint64, silentStallMs);

    constexpr qint64 kHourMs = 3'600'000;
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1),
                   (graceMs == kWatchdogNoFrame) ? std::nullopt : std::optional(std::chrono::milliseconds(graceMs)));
    QCOMPARE(watchdog.check(kWatchdogNowMs, false).stall, StreamWatchdog::Stall::None);
    if (silentStallMs == kWatchdogNoFrame) {
        QCOMPARE(watchdog.check(kWatchdogNowMs + kHourMs, false).stall, StreamWatchdog::Stall::None);
    } else {
        QCOMPARE(watchdog.check(kWatchdogNowMs + silentStallMs - 1, false).stall, StreamWatchdog::Stall::None);
        const StreamWatchdog::Result result = watchdog.check(kWatchdogNowMs + silentStallMs, false);
        QCOMPARE(result.stall, StreamWatchdog::Stall::Source);
        QCOMPARE(result.silentMs, silentStallMs);
    }

    // Once frames flow, the timeout applies.
    (void) watchdog.noteSourceFrame(kWatchdogNowMs + kHourMs);
    QCOMPARE(watchdog.check(kWatchdogNowMs + kHourMs + 1000, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + kHourMs + 1001, false).stall, StreamWatchdog::Stall::Source);
}

void StreamWatchdogTest::_testSinkSilenceCountsFromItsClock()
{
    // A decoder that never delivers a frame stalls too, counted from its insertion.
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1));
    watchdog.startSinkClock(kWatchdogNowMs);
    (void) watchdog.noteSourceFrame(kWatchdogNowMs + 2000);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 2000, true).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 2001, true).stall, StreamWatchdog::Stall::Decoder);

    QVERIFY(watchdog.noteSinkFrame(kWatchdogNowMs + 2001));
    QVERIFY(!watchdog.noteSinkFrame(kWatchdogNowMs + 2002));
    watchdog.startSinkClock(kWatchdogNowMs + 2002);
    QVERIFY2(watchdog.noteSinkFrame(kWatchdogNowMs + 2003), "A new decoder's first frame must be reported again");
}

void StreamWatchdogTest::_testSinkLimits()
{
    using namespace std::chrono_literals;

    // A receiver whose pictures are its only sign of the stream: the source never stalls, and the first picture has
    // 5 s more than the timeout.
    StreamWatchdog watchdog;
    watchdog.start(2s, std::nullopt, {.timeouts = 1, .firstFrameGrace = 5s});
    watchdog.startSinkClock(kWatchdogNowMs);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 7000, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 7000, true).stall, StreamWatchdog::Stall::None);
    const StreamWatchdog::Result first = watchdog.check(kWatchdogNowMs + 7001, true);
    QCOMPARE(first.stall, StreamWatchdog::Stall::Decoder);
    QCOMPARE(first.silentMs, qint64(7001));

    // Once pictures flow, the timeout applies.
    QVERIFY(watchdog.noteSinkFrame(kWatchdogNowMs + 8000));
    QCOMPARE(watchdog.check(kWatchdogNowMs + 10000, true).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 10001, true).stall, StreamWatchdog::Stall::Decoder);

    // Silence that is excused counts from there, without the first picture's grace again.
    watchdog.excuseSinkSilence(kWatchdogNowMs + 20000);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 22000, true).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 22001, true).stall, StreamWatchdog::Stall::Decoder);

    // A new sink clock gives its first picture the grace again.
    watchdog.startSinkClock(kWatchdogNowMs + 30000);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 37000, true).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kWatchdogNowMs + 37001, true).stall, StreamWatchdog::Stall::Decoder);
}

UT_REGISTER_TEST(StreamWatchdogTest, TestLabel::Unit)
