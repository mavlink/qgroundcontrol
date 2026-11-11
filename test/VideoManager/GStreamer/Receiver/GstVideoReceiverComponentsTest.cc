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
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "PipelineStatsProbe.h"
#include "StreamWatchdog.h"
#include "VideoStreamStats.h"

namespace {

// Any monotonic time well clear of 0, which the watchdog reads as "no frame yet".
constexpr qint64 kNowMs = 100000;
constexpr qint64 kNoFrame = -1;

}  // namespace

void GstVideoReceiverComponentsTest::_testStallDetection_data()
{
    QTest::addColumn<uint>("timeoutSec");
    QTest::addColumn<qint64>("sourceFrameAgeMs");
    QTest::addColumn<qint64>("sinkFrameAgeMs");
    QTest::addColumn<bool>("decoding");
    QTest::addColumn<int>("expected");

    const int none = int(StreamWatchdog::Stall::None);
    const int source = int(StreamWatchdog::Stall::Source);
    const int decoder = int(StreamWatchdog::Stall::Decoder);

    QTest::newRow("source within timeout") << 2u << qint64(2000) << qint64(0) << true << none;
    QTest::newRow("source past timeout") << 2u << qint64(2001) << qint64(0) << true << source;
    QTest::newRow("zero timeout floors at 1 s, within") << 0u << qint64(1000) << kNoFrame << false << none;
    QTest::newRow("zero timeout floors at 1 s, past") << 0u << qint64(1001) << kNoFrame << false << source;
    QTest::newRow("sink within twice timeout") << 2u << qint64(0) << qint64(4000) << true << none;
    QTest::newRow("sink past twice timeout") << 2u << qint64(0) << qint64(4001) << true << decoder;
    QTest::newRow("sink silence ignored when not decoding") << 2u << qint64(0) << qint64(60000) << false << none;
    QTest::newRow("source stall reported first") << 2u << qint64(3000) << qint64(60000) << true << source;
}

void GstVideoReceiverComponentsTest::_testStallDetection()
{
    QFETCH(uint, timeoutSec);
    QFETCH(qint64, sourceFrameAgeMs);
    QFETCH(qint64, sinkFrameAgeMs);
    QFETCH(bool, decoding);
    QFETCH(int, expected);

    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(timeoutSec));
    (void) watchdog.noteSourceFrame(kNowMs - sourceFrameAgeMs);
    if (sinkFrameAgeMs != kNoFrame) {
        (void) watchdog.noteSinkFrame(kNowMs - sinkFrameAgeMs);
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
    QTest::addColumn<QSize>("presented");

    // A decoder's buffer padded to its alignment, with crop meta: the viewport holds the 1080-row picture.
    const QSize padded(1920, 1088);
    const QRect picture(0, 0, 1920, 1080);
    QTest::newRow("whole frame") << QSize(320, 240) << QRect() << 0 << 0 << QSize(320, 240);
    QTest::newRow("padded buffer") << padded << picture << 0 << 0 << QSize(1920, 1080);
    QTest::newRow("cropped, upside down") << QSize(640, 480) << QRect(8, 8, 600, 400) << 180 << 0 << QSize(600, 400);
    QTest::newRow("padded, frame turned 90") << padded << picture << 90 << 0 << QSize(1080, 1920);
    QTest::newRow("padded, format turned 270") << padded << picture << 0 << 270 << QSize(1080, 1920);
    QTest::newRow("turns adding up to 180") << padded << picture << 90 << 90 << QSize(1920, 1080);
    QTest::newRow("no frame") << QSize() << QRect() << 0 << 0 << QSize();
}

void GstVideoReceiverComponentsTest::_testPresentedSize()
{
    QFETCH(QSize, frameSize);
    QFETCH(QRect, viewport);
    QFETCH(int, frameRotation);
    QFETCH(int, formatRotation);
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

    QCOMPARE(VideoReceiver::presentedSize(frame), presented);
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

#else

void GstVideoReceiverComponentsTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverComponentsTest, TestLabel::Unit)
