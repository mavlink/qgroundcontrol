#include "GstVideoReceiverComponentsTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>

#include "GStreamerTestHelpers.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "RecordingFinalizer.h"
#include "StreamWatchdog.h"

namespace {

// Any monotonic time well clear of 0, which the watchdog reads as "no frame yet".
constexpr qint64 kNowMs = 100000;
constexpr qint64 kNoFrame = -1;

qint64 nextDelaySec(StreamWatchdog& watchdog)
{
    return watchdog.nextReconnectDelay().count();
}

GStreamer::GstElementPtr ownedBin(const char* name)
{
    return GStreamer::adoptElement(GST_ELEMENT(gst_object_ref_sink(gst_bin_new(name))));
}

}  // namespace

void GstVideoReceiverComponentsTest::_testReconnectBackoffSequence()
{
    StreamWatchdog watchdog;
    for (const qint64 expectedSec : {1, 2, 4, 8, 16, 30, 30}) {
        QCOMPARE(nextDelaySec(watchdog), expectedSec);
    }
}

void GstVideoReceiverComponentsTest::_testReconnectBackoffRestartsAfterRecovery()
{
    StreamWatchdog watchdog;
    for (int failedStart = 0; failedStart < 3; ++failedStart) {
        (void) nextDelaySec(watchdog);
    }

    // The reconnect worked and delivered frames before the stream failed again.
    watchdog.start(std::chrono::seconds(1));
    (void) watchdog.noteSourceFrame(kNowMs);
    QCOMPARE(nextDelaySec(watchdog), qint64(1));
    // Its own retry failing to start backs off from there.
    watchdog.start(std::chrono::seconds(1));
    QCOMPARE(nextDelaySec(watchdog), qint64(2));
}

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

void GstVideoReceiverComponentsTest::_testFirstFrameGrace()
{
    // RTSP's UDP to TCP fallback must not be cut short before the first frame; once frames flow, the timeout applies.
    StreamWatchdog watchdog;
    watchdog.start(std::chrono::seconds(1), std::chrono::milliseconds(5000));
    QCOMPARE(watchdog.check(kNowMs, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + 6000, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + 6001, false).stall, StreamWatchdog::Stall::Source);

    (void) watchdog.noteSourceFrame(kNowMs + 7000);
    QCOMPARE(watchdog.check(kNowMs + 8000, false).stall, StreamWatchdog::Stall::None);
    QCOMPARE(watchdog.check(kNowMs + 8001, false).stall, StreamWatchdog::Stall::Source);
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

void GstVideoReceiverComponentsTest::_testFinalizerSettlesOnlyFromArmedRecording_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("settles");

    QTest::newRow("armed file sink") << QStringLiteral("sinkbin") << true;
    QTest::newRow("element inside the file sink") << QStringLiteral("muxer") << true;
    QTest::newRow("element outside the file sink") << QStringLiteral("videosink") << false;
    QTest::newRow("previous recording's file sink") << QStringLiteral("oldsinkbin") << false;
}

void GstVideoReceiverComponentsTest::_testFinalizerSettlesOnlyFromArmedRecording()
{
    QFETCH(QString, source);
    QFETCH(bool, settles);

    GSTTEST_REQUIRE_GSTREAMER();

    const GStreamer::GstElementPtr oldSinkBin = ownedBin("oldsinkbin");
    const GStreamer::GstElementPtr sinkBin = ownedBin("sinkbin");
    GstElement* const muxer = gst_bin_new("muxer");
    (void) gst_bin_add(GST_BIN(sinkBin.get()), muxer);
    const GStreamer::GstElementPtr videoSink = ownedBin("videosink");

    RecordingFinalizer finalizer;
    finalizer.arm(oldSinkBin.get());
    (void) finalizer.disarm();
    finalizer.arm(sinkBin.get());

    GstElement* const sources[] = {oldSinkBin.get(), sinkBin.get(), muxer, videoSink.get()};
    GstElement* sourceElement = nullptr;
    for (GstElement* element : sources) {
        if (source == QString::fromUtf8(GST_OBJECT_NAME(element))) {
            sourceElement = element;
        }
    }
    QVERIFY(sourceElement);

    bool belongs = !settles;
    QCOMPARE(finalizer.settleFrom(GST_OBJECT(sourceElement), RecordingFinalizer::Outcome::Failed, &belongs), settles);
    QCOMPARE(belongs, settles);
    QCOMPARE(finalizer.outcome(), settles ? RecordingFinalizer::Outcome::Failed : RecordingFinalizer::Outcome::Pending);
}

void GstVideoReceiverComponentsTest::_testFinalizerKeepsFirstOutcome()
{
    GSTTEST_REQUIRE_GSTREAMER();

    const GStreamer::GstElementPtr sinkBin = ownedBin("sinkbin");

    // The file closed, then its muxer failed on the way down: the stop still succeeded.
    RecordingFinalizer finalizer;
    finalizer.arm(sinkBin.get());
    QVERIFY(finalizer.settle(RecordingFinalizer::Outcome::Closed));
    bool belongs = false;
    QVERIFY(!finalizer.settleFrom(GST_OBJECT(sinkBin.get()), RecordingFinalizer::Outcome::Failed, &belongs));
    QVERIFY(belongs);
    QCOMPARE(finalizer.disarm(), RecordingFinalizer::Outcome::Closed);

    // Nothing is armed until the next recording.
    QVERIFY(!finalizer.settle(RecordingFinalizer::Outcome::Closed));
    QCOMPARE(finalizer.outcome(), RecordingFinalizer::Outcome::Pending);
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

#else

void GstVideoReceiverComponentsTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverComponentsTest, TestLabel::Unit)
