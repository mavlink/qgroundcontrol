#include "GstVideoReceiverComponentsTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamerTestHelpers.h"
#include "GstBusPump.h"
#include "GstRtpPacketProbe.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "PipelineStatsProbe.h"
#include "RtpPacketCounter.h"
#include "StreamWatchdog.h"
#include "VideoStreamStats.h"

namespace {

// Any monotonic time well clear of 0, which the watchdog reads as "no frame yet".
constexpr qint64 kNowMs = 100000;
constexpr qint64 kNoFrame = -1;

constexpr int kVideoClockRate = 90000;
constexpr quint32 kSsrc = 0x1234ABCD;
// Added to a sequence number of _testRtpPacketCounter: the packet is of another SSRC.
constexpr int kOtherSender = 0x10000;

/// An RTP packet with a one-byte payload, stamped as arrived at @p arrivalMs.
GstBuffer* makeRtpPacket(quint16 sequence, quint32 timestamp, qint64 arrivalMs, quint8 firstByte = 0x80)
{
    guint8 packet[13] = {firstByte, 96};
    GST_WRITE_UINT16_BE(packet + 2, sequence);
    GST_WRITE_UINT32_BE(packet + 4, timestamp);
    GST_WRITE_UINT32_BE(packet + 8, kSsrc);
    GstBuffer* const buffer = gst_buffer_new_memdup(packet, sizeof(packet));
    GST_BUFFER_DTS(buffer) = static_cast<GstClockTime>(arrivalMs) * GST_MSECOND;
    return buffer;
}

/// An RTCP sender report of the same sender, without reception reports.
GstBuffer* makeRtcpSenderReport(qint64 arrivalMs)
{
    guint8 packet[28] = {0x80, 200, 0, 6};
    GST_WRITE_UINT32_BE(packet + 4, kSsrc);
    GstBuffer* const buffer = gst_buffer_new_memdup(packet, sizeof(packet));
    GST_BUFFER_DTS(buffer) = static_cast<GstClockTime>(arrivalMs) * GST_MSECOND;
    return buffer;
}

}  // namespace

void GstVideoReceiverComponentsTest::_testStallDetection_data()
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

    QTest::newRow("source within timeout") << 2u << qint64(2000) << qint64(0) << kNoFrame << true << none;
    QTest::newRow("source past timeout") << 2u << qint64(2001) << qint64(0) << kNoFrame << true << source;
    QTest::newRow("zero timeout floors at 1 s, within") << 0u << qint64(1000) << kNoFrame << kNoFrame << false << none;
    QTest::newRow("zero timeout floors at 1 s, past") << 0u << qint64(1001) << kNoFrame << kNoFrame << false << source;
    QTest::newRow("sink within twice timeout") << 2u << qint64(0) << qint64(4000) << kNoFrame << true << none;
    QTest::newRow("sink past twice timeout") << 2u << qint64(0) << qint64(4001) << kNoFrame << true << decoder;
    QTest::newRow("sink silence ignored when not decoding")
        << 2u << qint64(0) << qint64(60000) << kNoFrame << false << none;
    QTest::newRow("source stall reported first") << 2u << qint64(3000) << qint64(60000) << kNoFrame << true << source;
    // The keyframe gate may keep the stream from the decoder for as long as the shortest timeout gives the decoder.
    QTest::newRow("frames kept from the decoder are not its silence")
        << 1u << qint64(0) << qint64(2500) << qint64(500) << true << none;
    QTest::newRow("sink past twice timeout since the last frame kept from the decoder")
        << 1u << qint64(0) << qint64(4500) << qint64(2001) << true << decoder;
}

void GstVideoReceiverComponentsTest::_testStallDetection()
{
    QFETCH(uint, timeoutSec);
    QFETCH(qint64, sourceFrameAgeMs);
    QFETCH(qint64, sinkFrameAgeMs);
    QFETCH(qint64, withheldFrameAgeMs);
    QFETCH(bool, decoding);
    QFETCH(int, expected);

    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(timeoutSec));
    (void) watchdog.noteSourceFrame(kNowMs - sourceFrameAgeMs);
    if (sinkFrameAgeMs != kNoFrame) {
        (void) watchdog.noteSinkFrame(kNowMs - sinkFrameAgeMs);
    }
    if (withheldFrameAgeMs != kNoFrame) {
        watchdog.noteFrameWithheld(kNowMs - withheldFrameAgeMs);
    }

    QCOMPARE(int(watchdog.check(kNowMs, decoding).stall), expected);
}

void GstVideoReceiverComponentsTest::_testFirstFrameGrace_data()
{
    QTest::addColumn<qint64>("graceMs");        // kNoFrame: none, the first frame may take any time
    QTest::addColumn<qint64>("silentStallMs");  // kNoFrame: never

    // RTSP's UDP to TCP fallback must not be cut short before the first frame.
    QTest::newRow("rtsp fallback") << qint64(5000) << qint64(6001);
    // A UDP port nobody sends to yet hasn't stalled.
    QTest::newRow("listening") << kNoFrame << kNoFrame;
}

void GstVideoReceiverComponentsTest::_testFirstFrameGrace()
{
    QFETCH(qint64, graceMs);
    QFETCH(qint64, silentStallMs);

    constexpr qint64 kHourMs = 3'600'000;
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1),
                   (graceMs == kNoFrame) ? std::nullopt : std::optional(std::chrono::milliseconds(graceMs)));
    QCOMPARE(watchdog.check(kNowMs, false).stall, StreamWatchdog::Stall::None);
    if (silentStallMs == kNoFrame) {
        QCOMPARE(watchdog.check(kNowMs + kHourMs, false).stall, StreamWatchdog::Stall::None);
    } else {
        QCOMPARE(watchdog.check(kNowMs + silentStallMs - 1, false).stall, StreamWatchdog::Stall::None);
        QCOMPARE(watchdog.check(kNowMs + silentStallMs, false).stall, StreamWatchdog::Stall::Source);
    }

    // Once frames flow, the timeout applies.
    (void) watchdog.noteSourceFrame(kNowMs + kHourMs);
    QCOMPARE(watchdog.check(kNowMs + kHourMs + 1000, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + kHourMs + 1001, false).stall, StreamWatchdog::Stall::Source);
}

void GstVideoReceiverComponentsTest::_testDecoderSilenceCountsFromInsertion()
{
    // A decoder that never delivers a frame stalls too, counted from its insertion.
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1));
    watchdog.startSinkClock(kNowMs);
    (void) watchdog.noteSourceFrame(kNowMs + 2000);
    QCOMPARE(watchdog.check(kNowMs + 2000, true).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + 2001, true).stall, StreamWatchdog::Stall::Decoder);

    QVERIFY(watchdog.noteSinkFrame(kNowMs + 2001));
    QVERIFY(!watchdog.noteSinkFrame(kNowMs + 2002));
    watchdog.startSinkClock(kNowMs + 2002);
    QVERIFY2(watchdog.noteSinkFrame(kNowMs + 2003), "A new decoder's first frame must be reported again");
}

void GstVideoReceiverComponentsTest::_testSourceTimeoutStartsAtFirstCheck()
{
    // A source that never delivers a frame must still time out, counted from the first check.
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1));
    QCOMPARE(watchdog.check(kNowMs, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + 1000, false).stall, StreamWatchdog::Stall::None);
    const StreamWatchdog::Result result = watchdog.check(kNowMs + 1001, false);
    QCOMPARE(result.stall, StreamWatchdog::Stall::Source);
    QCOMPARE(result.silentMs, qint64(1001));
}

void GstVideoReceiverComponentsTest::_testPipelineDotStaysInDirectory_data()
{
    QTest::addColumn<QString>("tag");
    QTest::addColumn<QString>("fileStem");  // empty: the dump directory can't be created

    QTest::newRow("receiver snapshot") << QStringLiteral("receiver-videoContent-pipeline-watchdog-timeout")
                                       << QStringLiteral("receiver-videoContent-pipeline-watchdog-timeout");
    QTest::newRow("parent traversal") << QStringLiteral("../../escaped") << QStringLiteral("______escaped");
    QTest::newRow("path separators") << QStringLiteral("a/b\\c") << QStringLiteral("a_b_c");
    QTest::newRow("non-ASCII letters") << QStringLiteral("d\u00e9faut") << QStringLiteral("d\u00e9faut");
    QTest::newRow("empty") << QString() << QStringLiteral("graph");
    QTest::newRow("too long") << QString(100, u'a') << QString(57, u'a');
    QTest::newRow("unwritable directory") << QStringLiteral("pipeline-error") << QString();
}

void GstVideoReceiverComponentsTest::_testPipelineDotStaysInDirectory()
{
    QFETCH(QString, tag);
    QFETCH(QString, fileStem);

    GSTTEST_REQUIRE_GSTREAMER();

    QTemporaryDir parent;
    QVERIFY(parent.isValid());
    QString directory = parent.filePath(QStringLiteral("a/b/dots"));
    if (fileStem.isEmpty()) {
        // A regular file where the dump directory should go.
        QFile blocker(parent.filePath(QStringLiteral("blocker")));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        directory = blocker.fileName() + QStringLiteral("/dots");
        expectLogMessage("Utilities.QGCFileHelper", QtWarningMsg,
                         QRegularExpression(QStringLiteral("failed to create parent directory")));
    }
    const GStreamer::GstElementPtr pipeline =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new("receiver"))));

    const QString path = PipelineDotDumper::writePipelineDot(pipeline.get(), tag, directory);
    if (fileStem.isEmpty()) {
        verifyExpectedLogMessage();
        QVERIFY2(path.isEmpty(), qPrintable(path));
        return;
    }
    QVERIFY(!path.isEmpty());
    const QFileInfo file(path);
    QCOMPARE(file.absolutePath(), QDir(directory).absolutePath());
    QVERIFY2(file.fileName().startsWith(fileStem + u'-'), qPrintable(file.fileName()));
    QVERIFY(file.exists());
}

void GstVideoReceiverComponentsTest::_testPipelineNamedAfterReceiver_data()
{
    QTest::addColumn<QString>("receiverName");
    QTest::addColumn<QByteArray>("pipelineName");

    QTest::newRow("main stream") << QStringLiteral("videoContent") << QByteArray("receiver-videoContent");
    QTest::newRow("unnamed") << QString() << QByteArray("receiver");
    QTest::newRow("unsafe name") << QStringLiteral("../x") << QByteArray("receiver-___x");
}

void GstVideoReceiverComponentsTest::_testPipelineNamedAfterReceiver()
{
    QFETCH(QString, receiverName);
    QFETCH(QByteArray, pipelineName);

    PipelineDotDumper dumper;
    dumper.setReceiverName(receiverName);
    QCOMPARE(dumper.pipelineName(), pipelineName);
}

void GstVideoReceiverComponentsTest::_testPresentedSize_data()
{
    QTest::addColumn<QSize>("frameSize");  // invalid: no frame
    QTest::addColumn<QRect>("viewport");   // null: the format's default, the whole frame
    QTest::addColumn<int>("frameRotation");
    QTest::addColumn<int>("formatRotation");
    QTest::addColumn<QSize>("pictureSize");  // invalid: the viewport's
    QTest::addColumn<QSize>("presented");

    // A decoder's buffer padded to its alignment, with crop meta: the viewport holds the 1080-row picture.
    const QSize padded(1920, 1088);
    const QRect picture(0, 0, 1920, 1080);
    // That buffer squeezed into a texture of the picture's size: the viewport leaves its padding out.
    const QSize squeezed(1920, 1080);
    const QRect unpadded(0, 0, 1920, 1072);
    QTest::newRow("whole frame") << QSize(320, 240) << QRect() << 0 << 0 << QSize() << QSize(320, 240);
    QTest::newRow("padded buffer") << padded << picture << 0 << 0 << QSize() << QSize(1920, 1080);
    QTest::newRow("cropped, upside down")
        << QSize(640, 480) << QRect(8, 8, 600, 400) << 180 << 0 << QSize() << QSize(600, 400);
    QTest::newRow("padded, frame turned 90") << padded << picture << 90 << 0 << QSize() << QSize(1080, 1920);
    QTest::newRow("padded, format turned 270") << padded << picture << 0 << 270 << QSize() << QSize(1080, 1920);
    QTest::newRow("turns adding up to 180") << padded << picture << 90 << 90 << QSize() << QSize(1920, 1080);
    QTest::newRow("squeezed texture") << squeezed << unpadded << 0 << 0 << squeezed << QSize(1920, 1080);
    QTest::newRow("squeezed, frame turned 90") << squeezed << unpadded << 90 << 0 << squeezed << QSize(1080, 1920);
    QTest::newRow("no frame") << QSize() << QRect() << 0 << 0 << QSize(1920, 1080) << QSize();
}

void GstVideoReceiverComponentsTest::_testPresentedSize()
{
    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);
    QFETCH(int, frameRotation);
    QFETCH(int, formatRotation);
    QFETCH(QSize, pictureSize);
    QFETCH(QSize, presented);

    QVideoFrame frame;
    if (frameSize.isValid()) {
        QVideoFrameFormat format(frameSize, QVideoFrameFormat::Format_NV12);
        if (!viewport.isNull()) {
            format.setViewport(viewport);
        }
        format.setRotation(static_cast<QtVideo::Rotation>(formatRotation));
        frame = QVideoFrame(format);
        frame.setRotation(static_cast<QtVideo::Rotation>(frameRotation));
    }

    QCOMPARE(VideoReceiver::presentedSize(frame, pictureSize), presented);
}

namespace {

QString messageName(GstMessage* message)
{
    const GstStructure* const structure = gst_message_get_structure(message);
    return structure ? QString::fromUtf8(gst_structure_get_name(structure)) : QString();
}

void postApplication(GstElement* pipeline, const char* name)
{
    (void) gst_element_post_message(pipeline,
                                    gst_message_new_application(GST_OBJECT(pipeline), gst_structure_new_empty(name)));
}

}  // namespace

void GstVideoReceiverComponentsTest::_testBusPump()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GStreamer::GstElementPtr pipeline =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_pipeline_new("pump"))));
    QVERIFY(pipeline);

    QStringList synced;
    QStringList handled;
    GstBusPump pump;
    QVERIFY(pump.attach(
        pipeline.get(),
        [&synced](GstMessage* message) {
            const QString name = messageName(message);
            synced.append(name);
            if (name == QStringLiteral("taken")) {
                gst_message_unref(message);
                return GST_BUS_DROP;
            }
            return GST_BUS_PASS;
        },
        [&handled](GstMessage* message) { handled.append(messageName(message)); }));

    // The sync handler sees every message as it is posted; one it takes is never queued.
    postApplication(pipeline.get(), "taken");
    postApplication(pipeline.get(), "queued");
    QCOMPARE(synced, (QStringList{QStringLiteral("taken"), QStringLiteral("queued")}));
    QVERIFY(handled.isEmpty());
    pump.drain();
    QCOMPARE(handled, QStringList{QStringLiteral("queued")});

    // The bus's poll fd wakes the pump on its own.
    postApplication(pipeline.get(), "woken");
    QTRY_COMPARE_WITH_TIMEOUT(handled.size(), 2, TestTimeout::shortMs());

    // A teardown handles messages until it has what it waits for.
    postApplication(pipeline.get(), "first");
    postApplication(pipeline.get(), "second");
    pump.pumpUntil(
        GST_MESSAGE_APPLICATION, [&handled]() { return handled.size() == 3; }, QDeadlineTimer(TestTimeout::shortMs()));
    QCOMPARE(handled.last(), QStringLiteral("first"));
    const GStreamer::GstMessagePtr second = pump.pop(GST_MESSAGE_APPLICATION);
    QVERIFY(second);
    QCOMPARE(messageName(second.get()), QStringLiteral("second"));
    QVERIFY(!pump.pop(GST_MESSAGE_APPLICATION));

    // Detached, nothing the pipeline posts reaches either handler.
    pump.detach();
    pump.detach();
    postApplication(pipeline.get(), "late");
    QCoreApplication::processEvents();
    pump.drain();
    QCOMPARE(synced.size(), 5);
    QCOMPARE(handled.size(), 3);
}

void GstVideoReceiverComponentsTest::_testStatsProbe()
{
    GSTTEST_REQUIRE_GSTREAMER();

    PipelineStatsProbe probe;
    const auto note = [&probe](qint64 ptsMs, bool keyframe) {
        GstBuffer* const buffer = gst_buffer_new_allocate(nullptr, 100, nullptr);
        GST_BUFFER_PTS(buffer) = static_cast<GstClockTime>(ptsMs) * GST_MSECOND;
        if (!keyframe) {
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);
        }
        probe.noteBuffer(buffer);
        gst_buffer_unref(buffer);
    };
    const auto sample = [&probe]() {
        VideoStreamStatsMeter::Counters counters;
        VideoStreamStats stats;
        probe.sample(nullptr, nullptr, nullptr, counters, stats);
        return std::pair{counters, stats};
    };

    // Nothing to ask without a pipeline, and nothing measured yet.
    auto [counters, stats] = sample();
    QCOMPARE(counters.bytesReceived, std::optional<quint64>(0));
    QVERIFY(!counters.rtp);
    QCOMPARE(stats.keyframeIntervalMs, -1.0);
    QCOMPARE(stats.latencyMs, -1.0);
    QVERIFY(stats.codec.isEmpty());

    // A 500 ms GOP whose keyframes arrive bunched up, read from arrival times: the average evens it out. Parameter sets
    // in a buffer of their own share their keyframe's timestamp and don't count.
    note(0, true);
    note(0, true);
    note(250, false);
    note(500, true);
    note(1080, true);
    note(1420, true);
    note(2000, true);
    std::tie(counters, stats) = sample();
    QCOMPARE(counters.bytesReceived, std::optional<quint64>(700));
    QCOMPARE(stats.keyframeIntervalMs, 500.0);
    // The window moves on: the average of the last five keyframes.
    note(2700, true);
    std::tie(counters, stats) = sample();
    QCOMPARE(stats.keyframeIntervalMs, 550.0);

    // Timestamps that go back start over.
    note(100, true);
    note(400, true);
    std::tie(counters, stats) = sample();
    QCOMPARE(stats.keyframeIntervalMs, 300.0);
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounter_data()
{
    QTest::addColumn<QList<int>>("sequences");  // Plus kOtherSender for a packet of another SSRC.
    QTest::addColumn<quint64>("received");
    QTest::addColumn<quint64>("lost");

    const auto row = [](const char* name, const QList<int>& sequences, quint64 received, quint64 lost) {
        QTest::newRow(name) << sequences << received << lost;
    };
    const int other = kOtherSender;
    row("first packet", {1000}, 1, 0);
    row("in order", {10, 11, 12, 13}, 4, 0);
    row("loss", {10, 11, 14, 15}, 4, 2);
    row("late packet still missing", {10, 12}, 2, 1);
    row("late packet", {10, 12, 11, 13}, 4, 0);
    row("duplicate", {10, 11, 11, 12}, 3, 0);
    row("duplicate does not hide a loss", {10, 12, 12, 13}, 3, 1);
    row("duplicate of a late packet", {10, 12, 11, 11}, 3, 0);
    row("wraparound", {65534, 65535, 0, 1}, 4, 0);
    row("loss across the wraparound", {65534, 1}, 2, 2);
    row("late packet across the wraparound", {65535, 1, 0}, 3, 0);
    row("before the first packet", {100, 99, 101}, 2, 0);
    const int window = RtpPacketCounter::kReorderWindow;
    row("late at the end of the reorder window", {200, 200 + window, 201}, 3, quint64(window - 2));
    row("too late for the reorder window", {200, 201 + window, 201}, 2, quint64(window));
    row("longest loss", {0, 2999}, 2, 2998);
    // Too far ahead to be loss: counted once a second packet shows the sender went on from there.
    row("jump, unconfirmed", {0, 3000}, 1, 0);
    row("sender restarted", {0, 1, 5000, 5001}, 4, 0);
    row("sender restarted to lower numbers", {30000, 30001, 100, 101}, 4, 0);
    row("sender restarted, next packet lost", {0, 1, 40000, 40002}, 4, 1);
    row("stray packet", {10, 11, 40000, 12, 13}, 4, 0);
    row("stray packets apart", {10, 40000, 11, 40001, 12}, 3, 0);
    row("stray packet twice", {10, 40000, 40000, 11}, 2, 0);
    row("losses add up across a restart", {10, 12, 40000, 40001, 40003}, 5, 2);
    // Another sender's numbers continue nothing, however close they are.
    row("new SSRC", {10, 11, other + 500, other + 501}, 4, 0);
    row("new SSRC, numbers go on after a gap", {10, 12, other + 15, other + 16}, 4, 1);
    row("new SSRC, next packet lost", {10, 11, other + 500, other + 502}, 4, 1);
    // It takes two packets in a row to be the stream: one that strays in leaves the sequence, and its loss, alone.
    row("stray packet of another SSRC", {10, other + 500, 12, 13}, 3, 1);
    row("stray packets of another SSRC apart", {10, other + 500, 11, other + 501, 12}, 3, 0);
    row("another SSRC, unconfirmed", {10, 11, other + 500}, 2, 0);
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounter()
{
    QFETCH(QList<int>, sequences);
    QFETCH(quint64, received);
    QFETCH(quint64, lost);

    RtpPacketCounter counter(kVideoClockRate);
    QCOMPARE(counter.packetsReceived(), quint64(0));
    QCOMPARE(counter.packetsLost(), quint64(0));
    for (const int sequence : sequences) {
        counter.add(static_cast<quint16>(sequence), 0, (sequence >= kOtherSender) ? (kSsrc + 1) : kSsrc, std::nullopt);
    }
    QCOMPARE(counter.packetsReceived(), received);
    QCOMPARE(counter.packetsLost(), lost);
    QVERIFY(!counter.jitterNs());
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounterDropoutFollowsPacketRate_data()
{
    QTest::addColumn<int>("packetsPerSecond");
    QTest::addColumn<int>("packetsMissing");
    QTest::addColumn<bool>("countedAsLost");

    // Up to ten seconds of packets may be lost; a longer jump is a sender that started over.
    QTest::newRow("3 s of a 1500 packets/s stream") << 1500 << 4500 << true;
    QTest::newRow("12 s of a 1500 packets/s stream") << 1500 << 18000 << false;
    QTest::newRow("5 s of a 100 packets/s stream") << 100 << 500 << true;
    QTest::newRow("20 s of a 100 packets/s stream") << 100 << 2000 << false;
    // Not less than RtpPacketCounter::kMinDropout, however few packets a stream has.
    QTest::newRow("20 s of a 1 packet/s stream") << 1 << 20 << true;
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounterDropoutFollowsPacketRate()
{
    QFETCH(int, packetsPerSecond);
    QFETCH(int, packetsMissing);
    QFETCH(bool, countedAsLost);

    // Three seconds of the stream measure its packet rate.
    const int streamed = 3 * packetsPerSecond;
    const auto timestamp = [packetsPerSecond](int packet) {
        return static_cast<quint32>(qint64(packet) * kVideoClockRate / packetsPerSecond);
    };
    RtpPacketCounter counter(kVideoClockRate);
    for (int packet = 0; packet <= streamed; ++packet) {
        counter.add(static_cast<quint16>(packet), timestamp(packet), kSsrc, std::nullopt);
    }
    QCOMPARE(counter.packetsLost(), quint64(0));

    // The stream goes on after the missing packets; a second packet confirms a sender that started over.
    const int resumed = streamed + packetsMissing + 1;
    counter.add(static_cast<quint16>(resumed), timestamp(resumed), kSsrc, std::nullopt);
    counter.add(static_cast<quint16>(resumed + 1), timestamp(resumed + 1), kSsrc, std::nullopt);
    QCOMPARE(counter.packetsReceived(), quint64(streamed + 3));
    QCOMPARE(counter.packetsLost(), quint64(countedAsLost ? packetsMissing : 0));
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounterJitter_data()
{
    QTest::addColumn<QList<quint32>>("timestamps");
    QTest::addColumn<QList<qint64>>("arrivalsMs");  // -1: unknown
    QTest::addColumn<qint64>("jitterNs");           // -1: not measured

    // 9000 ticks of the 90 kHz clock are 100 ms.
    QTest::newRow("one packet") << QList<quint32>{0} << QList<qint64>{0} << qint64(-1);
    QTest::newRow("steady") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, 100, 200} << qint64(0);
    // RFC 3550: a sixteenth of each deviation goes into the estimate.
    QTest::newRow("late by 16 ms") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, 100, 216} << qint64(1'000'000);
    QTest::newRow("decays") << QList<quint32>{0, 9000, 18000, 27000, 36000} << QList<qint64>{0, 100, 216, 316, 416}
                            << qint64(878'906);
    QTest::newRow("packets of one frame")
        << QList<quint32>{0, 0, 9000} << QList<qint64>{0, 16, 100} << qint64(1'937'500);
    QTest::newRow("timestamp wraparound")
        << QList<quint32>{0xFFFFFF00, 0x00002228} << QList<qint64>{0, 100} << qint64(0);
    QTest::newRow("sent before the previous packet")
        << QList<quint32>{9000, 0} << QList<qint64>{100, 100} << qint64(6'250'000);
    QTest::newRow("no arrival times") << QList<quint32>{0, 9000, 18000} << QList<qint64>{-1, -1, -1} << qint64(-1);
    QTest::newRow("arrival time missing") << QList<quint32>{0, 9000, 18000} << QList<qint64>{0, -1, 200} << qint64(0);
}

void GstVideoReceiverComponentsTest::_testRtpPacketCounterJitter()
{
    QFETCH(QList<quint32>, timestamps);
    QFETCH(QList<qint64>, arrivalsMs);
    QFETCH(qint64, jitterNs);

    RtpPacketCounter counter(kVideoClockRate);
    for (qsizetype i = 0; i < timestamps.size(); ++i) {
        const qint64 arrivalMs = arrivalsMs.at(i);
        counter.add(static_cast<quint16>(i), timestamps.at(i), kSsrc,
                    (arrivalMs >= 0) ? std::optional(arrivalMs * 1'000'000) : std::nullopt);
    }
    QCOMPARE(counter.jitterNs().value_or(-1), jitterNs);
}

void GstVideoReceiverComponentsTest::_testStatsProbeCountsRtpPackets()
{
    GSTTEST_REQUIRE_GSTREAMER();

    // A source bin without a jitter buffer, and the pad its packets arrive on.
    const GStreamer::GstElementPtr source =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new(nullptr))));
    const GStreamer::GstPadPtr pad = GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("src", GST_PAD_SRC))));
    QVERIFY(gst_pad_set_active(pad.get(), TRUE));
    const auto push = [&pad](GstBuffer* packet) { return gst_pad_push(pad.get(), packet); };

    // The RTP packets received and lost that a sample hands the meter.
    using Counted = std::pair<qint64, qint64>;
    constexpr Counted kNotCounted{-1, -1};
    PipelineStatsProbe probe;
    VideoStreamStats stats;
    const auto sample = [&probe, &source, &stats, kNotCounted]() {
        VideoStreamStatsMeter::Counters counters;
        probe.sample(nullptr, source.get(), nullptr, counters, stats);
        return counters.rtp ? Counted(counters.rtp->packetsReceived, counters.rtp->packetsLost) : kNotCounted;
    };

    // Nobody counts the packets of a source without a jitter buffer unless it does so itself.
    QVERIFY(!GStreamer::rtpPacketCounts(source.get()));
    QCOMPARE(sample(), kNotCounted);
    QVERIFY(GStreamer::countRtpPackets(source.get(), pad.get(), kVideoClockRate));
    QCOMPARE(sample(), Counted(0, 0));

    // Packets arrive one by one or as a list; 103 is missing, and 104 is 16 ms late.
    QCOMPARE(push(makeRtpPacket(100, 0, 0)), GST_FLOW_NOT_LINKED);
    QCOMPARE(push(makeRtpPacket(101, 9000, 100)), GST_FLOW_NOT_LINKED);
    GstBufferList* const list = gst_buffer_list_new();
    gst_buffer_list_add(list, makeRtpPacket(102, 18000, 200));
    gst_buffer_list_add(list, makeRtpPacket(104, 27000, 316));
    QCOMPARE(gst_pad_push_list(pad.get(), list), GST_FLOW_NOT_LINKED);
    // Neither RTP version 2 nor a whole RTP header: not counted.
    QCOMPARE(push(makeRtpPacket(105, 36000, 400, 0x40)), GST_FLOW_NOT_LINKED);
    QCOMPARE(push(gst_buffer_new_allocate(nullptr, 11, nullptr)), GST_FLOW_NOT_LINKED);

    QCOMPARE(sample(), Counted(4, 1));
    QCOMPARE(stats.jitterMs, 1.0);
    // Only a jitter buffer tells these.
    QCOMPARE(stats.packetsLate, qint64(-1));
    QCOMPARE(stats.packetsDuplicate, qint64(-1));
    QCOMPARE(stats.rtxRequested, qint64(-1));
    QCOMPARE(stats.rtxRecovered, qint64(-1));

    // The missing packet arrives late: it isn't lost, but the meter is never given a count that shrank, which it would
    // take for a counter that restarted. The next loss makes up for it.
    QCOMPARE(push(makeRtpPacket(103, 22500, 420)), GST_FLOW_NOT_LINKED);
    QCOMPARE(GStreamer::rtpPacketCounts(source.get())->packetsLost, quint64(0));
    QCOMPARE(sample(), Counted(5, 1));
    QCOMPARE(push(makeRtpPacket(106, 45000, 500)), GST_FLOW_NOT_LINKED);
    QCOMPARE(sample(), Counted(6, 1));
    // An RTCP packet on the RTP port is neither a packet of the stream nor the start of another one: 107 is lost.
    QCOMPARE(push(makeRtcpSenderReport(550)), GST_FLOW_NOT_LINKED);
    QCOMPARE(push(makeRtpPacket(108, 54000, 600)), GST_FLOW_NOT_LINKED);
    QCOMPARE(sample(), Counted(7, 2));
}

#else

void GstVideoReceiverComponentsTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverComponentsTest, TestLabel::Unit)
