#include "GstVideoReceiverComponentsTest.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "BranchContext.h"
#include "GStreamerTestHelpers.h"
#include "GstBusPump.h"
#include "GstRtpPacketProbe.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "PipelineStatsProbe.h"
#include "RecordingBranch.h"
#include "RecordingSink.h"
#include "StreamWatchdog.h"
#include "VideoStreamStats.h"

namespace {

// Any monotonic time well clear of 0, which the watchdog reads as "no frame yet".
constexpr qint64 kNowMs = 100000;
constexpr qint64 kNoFrame = -1;

constexpr int kVideoClockRate = 90000;
constexpr quint32 kSsrc = 0x1234ABCD;
// In place of a VideoReceiver::FileFormat in _testRecordingContainerHoldsCodec: no container holds the codec.
constexpr int kNoContainer = -1;

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
    // A source that never delivers a frame must still time out, counted from the first check.
    QTest::newRow("no grace") << qint64(0) << qint64(1001);
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
        const StreamWatchdog::Result result = watchdog.check(kNowMs + silentStallMs, false);
        QCOMPARE(result.stall, StreamWatchdog::Stall::Source);
        QCOMPARE(result.silentMs, silentStallMs);
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
    const GStreamer::GstElementPtr pipeline = GStreamer::adoptFloating(gst_pipeline_new("receiver"));

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

void GstVideoReceiverComponentsTest::_testPipelineDotOmitsElementProperties()
{
    GSTTEST_REQUIRE_GSTREAMER();

    constexpr auto kSecretLocation = "/tmp/qgc-dot-secret-token";
    GstTest::TestPipeline pipeline;
    const QString description = QStringLiteral("filesrc name=source location=%1 ! fakesink name=sink");
    QVERIFY2(pipeline.launch(description.arg(QLatin1String(kSecretLocation))), qPrintable(pipeline.error()));

    const GStreamer::GCharPtr dotData =
        GStreamer::adoptGChar(gst_debug_bin_to_dot_data(pipeline.bin(), PipelineDotDumper::kDiagnosticDotGraphDetails));
    QVERIFY(dotData);
    const QByteArray dot(dotData.get());

    QVERIFY(dot.contains("source"));
    QVERIFY(dot.contains("sink"));
    QVERIFY(!dot.contains(kSecretLocation));
    QVERIFY(!dot.contains("qgc-dot-secret-token"));
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

    const GStreamer::GstElementPtr pipeline = GStreamer::adoptFloating(gst_pipeline_new("pump"));
    QVERIFY(pipeline);

    QStringList synced;
    QStringList handled;
    GstBusPump pump;
    pump.attach(
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
        [&handled](GstMessage* message) { handled.append(messageName(message)); });

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

void GstVideoReceiverComponentsTest::_testStatsProbeCountsRtpPackets()
{
    GSTTEST_REQUIRE_GSTREAMER();

    // A source bin without a jitter buffer, and the pad its packets arrive on.
    const GStreamer::GstElementPtr source = GStreamer::adoptFloating(gst_bin_new(nullptr));
    const GStreamer::GstPadPtr pad = GStreamer::adoptFloating(gst_pad_new("src", GST_PAD_SRC));
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

void GstVideoReceiverComponentsTest::_testRecordingContainerHoldsCodec_data()
{
    QTest::addColumn<QString>("caps");
    QTest::addColumn<int>("format");
    QTest::addColumn<int>("container");

    const int mkv = int(VideoReceiver::FileFormat::Mkv);
    const int mov = int(VideoReceiver::FileFormat::Mov);
    const int mp4 = int(VideoReceiver::FileFormat::Mp4);

    struct Codec
    {
        const char* name;
        const char* caps;
        int inMov;
        int inMp4;
    };

    // The codecs a stream can bring, as the source bins hand them on: H.264 and H.265 in the byte-stream layout,
    // which no muxer takes as it is, the others as their parsers give them.
    const Codec codecs[] = {
        {"H.264", "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au", mov, mp4},
        {"H.265", "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au", mov, mp4},
        {"AV1", "video/x-av1, stream-format=(string)obu-stream, alignment=(string)tu", mov, mp4},
        {"VP9", "video/x-vp9, profile=(string)0, chroma-format=(string)4:2:0", mov, mp4},
        {"MPEG-4", "video/mpeg, mpegversion=(int)4, systemstream=(boolean)false, parsed=(boolean)true", mov, mp4},
        {"VP8", "video/x-vp8, width=(int)320, height=(int)240", mov, mkv},
        {"MJPEG", "image/jpeg, parsed=(boolean)true, width=(int)320, height=(int)240", mov, mkv},
        {"MPEG-2", "video/mpeg, mpegversion=(int)2, systemstream=(boolean)false, parsed=(boolean)true", mkv, mkv},
        {"a codec no container holds", "video/x-qgc-test", kNoContainer, kNoContainer},
    };
    for (const Codec& codec : codecs) {
        const QString caps = QString::fromLatin1(codec.caps);
        const int inMkv = (codec.inMp4 == kNoContainer) ? kNoContainer : mkv;
        QTest::addRow("%s asked for in MKV", codec.name) << caps << mkv << inMkv;
        QTest::addRow("%s asked for in MOV", codec.name) << caps << mov << codec.inMov;
        QTest::addRow("%s asked for in MP4", codec.name) << caps << mp4 << codec.inMp4;
    }
    // Nothing known of the stream yet: as asked for.
    QTest::newRow("any caps") << QStringLiteral("ANY") << mp4 << mp4;
}

void GstVideoReceiverComponentsTest::_testRecordingContainerHoldsCodec()
{
    QFETCH(QString, caps);
    QFETCH(int, format);
    QFETCH(int, container);

    GSTTEST_REQUIRE_GSTREAMER();
    GSTTEST_SKIP_UNLESS_FACTORIES("matroskamux", "qtmux", "mp4mux");

    const GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_caps_from_string(caps.toLatin1().constData()));
    QVERIFY(inputCaps);
    const std::optional<VideoReceiver::FileFormat> chosen =
        GStreamer::RecordingSink::containerFor(static_cast<VideoReceiver::FileFormat>(format), inputCaps.get());
    QCOMPARE(chosen ? int(*chosen) : kNoContainer, container);

    // The file sink of the container chosen links to a stream of these caps, as the recording will.
    if (!chosen || gst_caps_is_any(inputCaps.get())) {
        return;
    }
    const GStreamer::GstElementPtr source = GStreamer::makeElement("capsfilter");
    QVERIFY(source);
    g_object_set(source.get(), "caps", inputCaps.get(), nullptr);
    const GStreamer::GstElementPtr sink =
        GStreamer::RecordingSink::create(QStringLiteral("recording-test"), *chosen, inputCaps.get());
    QVERIFY(sink);
    QVERIFY(gst_element_link(source.get(), sink.get()));
}

void GstVideoReceiverComponentsTest::_testRecordingNamesCodecNoContainerHolds()
{
    GSTTEST_REQUIRE_GSTREAMER();
    GSTTEST_SKIP_UNLESS_FACTORIES("appsrc", "matroskamux", "mp4mux");

    struct Listener final : RecordingBranch::Listener
    {
        void onRecordingFileOpened(const QString& /*file*/) override {}

        void onRecordingFileClosed(const QString& /*file*/, const QString& /*error*/) override {}

        void onRecordingFailed(const QString& /*file*/, const QString& error, bool /*fileOpened*/) override
        {
            failures.append(error);
        }

        void onRecordingNeedsNewFile(const QString& /*file*/) override {}

        void onRecordingNeedsFormat(const QString& /*file*/, VideoReceiver::FileFormat /*format*/) override
        {
            ++formatsAsked;
        }

        QStringList failures;
        int formatsAsked = 0;
    } listener;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    GstTest::TestPipeline pipeline;
    QVERIFY2(pipeline.launch(QStringLiteral("appsrc name=src caps=video/x-qgc-test ! tee name=tee")),
             qPrintable(pipeline.error()));
    const GStreamer::GstElementPtr tee = pipeline.element("tee");
    const PipelineDotDumper dotDumper;
    const BranchContext context{
        .pipeline = pipeline.get(),
        .tee = tee.get(),
        .redactedUri = QStringLiteral("test"),
        .dotDumper = &dotDumper,
        .watchdog = nullptr,
        .post = [](std::function<void()> task) { task(); },
    };
    RecordingBranch branch(context, listener);
    // Before the branch goes, which takes its pipeline to be stopped.
    const auto stopPipeline =
        qScopeGuard([&pipeline] { (void) gst_element_set_state(pipeline.get(), GST_STATE_NULL); });
    QVERIFY(branch.build());
    QVERIFY(pipeline.play());
    // The stream's first frame brings its caps to the branch's closed valve.
    QCOMPARE(gst_app_src_push_buffer(GST_APP_SRC(pipeline.element("src").get()),
                                     gst_buffer_new_allocate(nullptr, 16, nullptr)),
             GST_FLOW_OK);
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(branch.valve(), "src"));
    QVERIFY_TRUE_WAIT(gst_pad_has_current_caps(valveSrc.get()), TestTimeout::mediumMs());
    branch.onSourceStreaming();

    // Told as it is, where the muxer would only refuse the stream: no other container is worth asking for.
    const QString file = dir.filePath(QStringLiteral("unknown.mp4"));
    branch.setRequest(RecordingBranch::Request{.file = file, .format = VideoReceiver::FileFormat::Mp4});
    QCOMPARE(listener.failures.size(), 1);
    QVERIFY2(listener.failures.constFirst().contains(QLatin1String("video/x-qgc-test")),
             qPrintable(listener.failures.constFirst()));
    QCOMPARE(listener.formatsAsked, 0);
    QVERIFY(!QFile::exists(file));
}

UT_REGISTER_TEST(GstVideoReceiverComponentsTest, TestLabel::Unit)
