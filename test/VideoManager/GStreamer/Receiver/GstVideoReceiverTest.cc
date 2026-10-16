#include "GstVideoReceiverTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstVideoReceiver.h"
#include "MockVideoStreamServer.h"
#include "VideoReceiver.h"

// Receiver signals are emitted on its worker thread, so tests wait on spy counts: QSignalSpy::wait()
// only sees emissions after it starts and misses one that lands first.

namespace {

constexpr int kFrameRate = 30;
constexpr int kFrameWidth = 320;
constexpr int kFrameHeight = 240;
constexpr int kKeyframeInterval = 5;
// Frames the sender produces while a recording is open. Some may still be in flight when the
// recording ends, but a finalized file holds far more than kMinRecordedFrames; a file cut off
// before its final moov holds only the samples of the first header update (one).
constexpr int kRecordFrames = 15;
constexpr int kMinRecordedFrames = 5;
constexpr uint32_t kWatchdogTimeoutSec = 5;
constexpr int kRestartCycles = 2;
// Receiver reconnect backoff: the first retry follows a failure after 1 s, the second 2 s after that.
constexpr qint64 kFirstReconnectDelayMs = 1000;
constexpr qint64 kSecondReconnectDelayMs = 2000;
// The test thread sees worker-emitted signals late by up to its polling interval, more so on a loaded host.
constexpr qint64 kSignalLatencySlackMs = 200;

quint16 freeLoopbackUdpPort()
{
    QUdpSocket socket;
    return socket.bind(QHostAddress::LocalHost, 0) ? socket.localPort() : 0;
}

quint16 freeLoopbackTcpPort()
{
    QTcpServer server;
    return server.listen(QHostAddress::LocalHost, 0) ? server.serverPort() : 0;
}

QString loopbackUri(QLatin1StringView scheme, quint16 port)
{
    return QStringLiteral("%1://127.0.0.1:%2").arg(scheme).arg(port);
}

VideoReceiver::STATUS lastStatus(const QSignalSpy& spy)
{
    return spy.constLast().constFirst().value<VideoReceiver::STATUS>();
}

bool lastBool(const QSignalSpy& spy)
{
    return spy.constLast().constFirst().toBool();
}

/// Stand-in for the qgcvideosinkbin, owned the way GStreamer::createVideoSink() hands it out.
GStreamer::GstElementPtr makeFakeVideoSink()
{
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    return GStreamer::adoptElement(sink ? GST_ELEMENT(gst_object_ref_sink(sink)) : nullptr);
}

struct VideoSinkReleaser
{
    void operator()(GstElement* sink) const { GStreamer::releaseVideoSink(sink); }
};

using QgcVideoSinkPtr = std::unique_ptr<GstElement, VideoSinkReleaser>;

/// The qgcvideosinkbin VideoManager decodes into.
QgcVideoSinkPtr makeQgcVideoSink()
{
    return QgcVideoSinkPtr(static_cast<GstElement*>(GStreamer::createVideoSink(GStreamer::VideoSinkConfig{})));
}

/// Live H.264 test pattern sent as RTP to 127.0.0.1:port.
class H264RtpSender
{
public:
    bool start(quint16 port)
    {
        const QString description =
            QStringLiteral(
                "videotestsrc is-live=true pattern=ball ! video/x-raw,width=%4,height=%5,framerate=%1/1 ! "
                "x264enc name=encoder tune=zerolatency speed-preset=ultrafast key-int-max=%2 ! "
                "rtph264pay config-interval=-1 pt=96 ! udpsink host=127.0.0.1 port=%3 sync=false async=false")
                .arg(kFrameRate)
                .arg(kKeyframeInterval)
                .arg(port)
                .arg(kFrameWidth)
                .arg(kFrameHeight);
        if (!_pipeline.launch(description)) {
            return false;
        }
        const GStreamer::GstElementPtr encoder = _pipeline.element("encoder");
        const GStreamer::GstPadPtr encoderSrc =
            GStreamer::adoptPad(encoder ? gst_element_get_static_pad(encoder.get(), "src") : nullptr);
        return encoderSrc &&
               gst_pad_add_probe(encoderSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &_frames,
                                 nullptr) &&
               _pipeline.play();
    }

    /// Stops sending, as a camera losing its link would.
    void stop()
    {
        if (_pipeline.get()) {
            (void) gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
        }
    }

    int frames() const { return _frames.load(std::memory_order_relaxed); }

private:
    std::atomic<int> _frames{0};
    // Declared last, so the pipeline stops before the counter its probe writes to goes away.
    GstTest::TestPipeline _pipeline;
};

/// Demuxes the MP4/MOV at @p path to EOS. Returns the number of video samples, or -1 with
/// @p failure set when the file can't be played to the end.
int countPlayableFrames(const QString& path, QString& failure)
{
    std::atomic<int> frames{0};
    GstTest::TestPipeline pipeline;
    if (!pipeline.launch(QStringLiteral("filesrc name=src ! qtdemux ! fakesink name=sink sync=false"))) {
        failure = pipeline.error();
        return -1;
    }
    g_object_set(pipeline.element("src").get(), "location", QFile::encodeName(path).constData(), nullptr);
    const GStreamer::GstPadPtr sinkPad =
        GStreamer::adoptPad(gst_element_get_static_pad(pipeline.element("sink").get(), "sink"));
    (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &frames, nullptr);

    if (!pipeline.play()) {
        failure = QStringLiteral("%1: demux pipeline failed to start").arg(path);
        return -1;
    }
    const QString error = pipeline.runToEos();
    if (!error.isEmpty()) {
        failure = QStringLiteral("%1: %2").arg(path, error);
        return -1;
    }
    return frames.load(std::memory_order_relaxed);
}

/// Why the recording at @p path isn't a finalized file, or an empty string when it is.
QString recordingProblem(const QString& path)
{
    QString failure;
    const int frames = countPlayableFrames(path, failure);
    return (frames >= kMinRecordedFrames)
               ? QString()
               : QStringLiteral("%1: %2 frames recorded %3").arg(path).arg(frames).arg(failure);
}

/// A GstVideoReceiver, with spies on its signals, receiving @p uri or else the loopback port sender sends to once
/// started. The widget (a QQuickItem of its own unless given) must outlive it.
struct StreamingReceiver
{
    explicit StreamingReceiver(const QString& uri = QString(), QQuickItem* widget = nullptr)
        : port(uri.isEmpty() ? freeLoopbackUdpPort() : 0)
        , receiver(GStreamer::createVideoReceiver())
        , gstReceiver(qobject_cast<GstVideoReceiver*>(receiver.get()))
        , startSpy(receiver.get(), &VideoReceiver::onStartComplete)
        , stopSpy(receiver.get(), &VideoReceiver::onStopComplete)
        , streamingSpy(receiver.get(), &VideoReceiver::streamingChanged)
        , timeoutSpy(receiver.get(), &VideoReceiver::timeout)
        , startDecodingSpy(receiver.get(), &VideoReceiver::onStartDecodingComplete)
        , stopDecodingSpy(receiver.get(), &VideoReceiver::onStopDecodingComplete)
        , decodingSpy(receiver.get(), &VideoReceiver::decodingChanged)
        , decoderNameSpy(gstReceiver, &GstVideoReceiver::decoderNameChanged)
        , sizeSpy(receiver.get(), &VideoReceiver::videoSizeChanged)
        , startRecordingSpy(receiver.get(), &VideoReceiver::onStartRecordingComplete)
        , recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted)
        , recordingSpy(receiver.get(), &VideoReceiver::recordingChanged)
        , stopRecordingSpy(receiver.get(), &VideoReceiver::onStopRecordingComplete)
        , screenshotSpy(receiver.get(), &VideoReceiver::onTakeScreenshotComplete)
    {
        receiver->setUri(uri.isEmpty() ? loopbackUri(QLatin1StringView("udp"), port) : uri);
        receiver->setWidget(widget ? widget : &_widget);
        // Reconnect policy isn't under test; don't let a failure restart the pipeline behind the test.
        receiver->setAutoReconnect(false);
    }

    // QSignalSpy takes the receiver's signals on the worker and streaming threads that emit them, so the spies must
    // not be destroyed while one may still arrive: the receiver, and with it those threads, goes first.
    ~StreamingReceiver() { receiver.reset(); }

    StreamingReceiver(const StreamingReceiver&) = delete;
    StreamingReceiver& operator=(const StreamingReceiver&) = delete;

    [[nodiscard]] bool startSending() { return (port != 0) && sender.start(port); }

    /// start(), with decoding into @p videoSink requested right after it as VideoManager does, then waits for the
    /// start to report OK.
    [[nodiscard]] bool start(GstElement* videoSink = nullptr, uint32_t watchdogTimeoutSec = kWatchdogTimeoutSec)
    {
        const int starts = startSpy.count();
        receiver->start(watchdogTimeoutSec);
        if (videoSink) {
            receiver->startDecoding(videoSink);
        }
        return UnitTest::waitForSignalCount(startSpy, starts + 1, TestTimeout::mediumDuration(), u"onStartComplete") &&
               (lastStatus(startSpy) == VideoReceiver::STATUS_OK);
    }

    /// start(), then waits for the stream to flow.
    [[nodiscard]] bool startStreaming(uint32_t watchdogTimeoutSec = kWatchdogTimeoutSec)
    {
        const int streams = streamingSpy.count();
        return start(nullptr, watchdogTimeoutSec) &&
               UnitTest::waitForSignalCount(streamingSpy, streams + 1, TestTimeout::mediumDuration(),
                                            u"streamingChanged");
    }

    /// startRecording(), then waits for its first keyframe and for kRecordFrames more frames sent into it.
    [[nodiscard]] bool record(const QString& path)
    {
        const int started = recordingStartedSpy.count();
        receiver->startRecording(path, VideoReceiver::FILE_FORMAT_MP4);
        if (!UnitTest::waitForSignalCount(recordingStartedSpy, started + 1, TestTimeout::mediumDuration(),
                                          u"recordingStarted")) {
            return false;
        }
        const int first = sender.frames();
        return UnitTest::waitForCondition([this, first] { return sender.frames() >= (first + kRecordFrames); },
                                          TestTimeout::mediumDuration(), u"frames sent while recording");
    }

private:
    QQuickItem _widget;

public:
    const quint16 port;
    H264RtpSender sender;
    std::unique_ptr<VideoReceiver> receiver;
    GstVideoReceiver* const gstReceiver;
    QSignalSpy startSpy;
    QSignalSpy stopSpy;
    QSignalSpy streamingSpy;
    QSignalSpy timeoutSpy;
    QSignalSpy startDecodingSpy;
    QSignalSpy stopDecodingSpy;
    QSignalSpy decodingSpy;
    QSignalSpy decoderNameSpy;
    QSignalSpy sizeSpy;
    QSignalSpy startRecordingSpy;
    QSignalSpy recordingStartedSpy;
    QSignalSpy recordingSpy;
    QSignalSpy stopRecordingSpy;
    QSignalSpy screenshotSpy;
};

}  // namespace

void GstVideoReceiverTest::initTestCase()
{
    UnitTest::initTestCase();

    GStreamer::redirectGLibLogging();
    if (GstTest::initGStreamer().isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
}

void GstVideoReceiverTest::init()
{
    QString unavailable = GstTest::initGStreamer();
    if (unavailable.isEmpty()) {
        const QString missing = GstTest::firstMissingFactory(
            {"videotestsrc", "x264enc", "rtph264pay", "udpsink", "udpsrc", "rtph264depay", "parsebin", "decodebin3",
             "valve", "tee", "splitmuxsink", "mp4mux", "qtmux", "qtdemux", "fakesink"});
        if (!missing.isEmpty()) {
            unavailable = QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing);
        }
    }
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

    UnitTest::init();
}

void GstVideoReceiverTest::_expectFailedStartWarnings()
{
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from ")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to start")));
}

bool GstVideoReceiverTest::_runOnPipeline(VideoReceiver* receiver, const std::function<void(GstElement*)>& task)
{
    auto* const gstReceiver = qobject_cast<GstVideoReceiver*>(receiver);
    return gstReceiver && QMetaObject::invokeMethod(
                              gstReceiver->_workerContext, [gstReceiver, &task] { task(gstReceiver->_pipeline.get()); },
                              Qt::BlockingQueuedConnection);
}

void GstVideoReceiverTest::_testStopAfterUriClearedTearsDown()
{
    StreamingReceiver stream;
    QVERIFY(stream.port != 0);
    QVERIFY(stream.start());

    // VideoManager clears the URI before stopping when the video source is disabled.
    stream.receiver->setUri(QString());
    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.stopSpy), VideoReceiver::STATUS_OK);

    QUdpSocket rebind;
    QVERIFY2(rebind.bind(QHostAddress::LocalHost, stream.port, QAbstractSocket::DontShareAddress),
             "Stopped receiver still holds its UDP port");
}

void GstVideoReceiverTest::_testFailedStartLeavesCleanState()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));

    // udp265 exposes its source pad statically, so the receiver reports streaming before the
    // bind failure surfaces in the READY transition.
    StreamingReceiver stream(loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()));

    _expectFailedStartWarnings();
    stream.receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.startSpy), VideoReceiver::STATUS_FAIL);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(!stream.streamingSpy.isEmpty());
    QVERIFY2(!lastBool(stream.streamingSpy), "Failed start left the receiver streaming");

    // The failed start already reported itself; its ERROR must not queue a teardown of its own.
    // That task would run right after start() on the worker, so a short window suffices.
    QVERIFY_NO_SIGNAL_WAIT(stream.stopSpy, 250);

    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.stopSpy), VideoReceiver::STATUS_INVALID_STATE);
}

void GstVideoReceiverTest::_testVideoSinkRefcountStableAcrossRestarts()
{
    const QgcVideoSinkPtr videoSink = makeQgcVideoSink();
    QVERIFY(videoSink);
    QVERIFY2(!g_object_is_floating(videoSink.get()), "createVideoSink() must hand out an owned reference");
    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(videoSink.get()), 1);

    StreamingReceiver stream;
    QVERIFY(stream.startSending());

    for (int cycle = 1; cycle <= kRestartCycles; ++cycle) {
        // Decoding is requested while the first source pad may be arriving.
        QVERIFY(stream.start(videoSink.get()));
        QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, (2 * cycle) - 1, TestTimeout::mediumMs());
        QVERIFY(lastBool(stream.decodingSpy));

        stream.receiver->stop();
        QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, cycle, TestTimeout::mediumMs());
        QCOMPARE(lastStatus(stream.stopSpy), VideoReceiver::STATUS_OK);
        QVERIFY(!lastBool(stream.decodingSpy));
    }

    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(videoSink.get()), 1);
}

void GstVideoReceiverTest::_testDecodesMockLinkStreams_data()
{
    QTest::addColumn<int>("streamType");

    using StreamType = MockVideoStreamServer::StreamType;
    QTest::newRow("rtp h264 udp") << int(StreamType::RtpUdpH264);
    QTest::newRow("rtp h265 udp") << int(StreamType::RtpUdpH265);
    QTest::newRow("mpeg-ts udp") << int(StreamType::MpegTsUdp);
    QTest::newRow("mpeg-ts tcp") << int(StreamType::MpegTsTcp);
#ifdef QGC_GST_RTSP_SERVER
    QTest::newRow("rtsp") << int(StreamType::RtspH264);
#endif
}

void GstVideoReceiverTest::_testDecodesMockLinkStreams()
{
#ifndef QGC_HAS_MOCK_VIDEO_STREAM_SERVER
    QSKIP("MockVideoStreamServer is only built into single-config Debug builds");
#else
    QFETCH(int, streamType);

    const auto type = static_cast<MockVideoStreamServer::StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }
    const bool tcp =
        (type == MockVideoStreamServer::StreamType::MpegTsTcp) || (type == MockVideoStreamServer::StreamType::RtspH264);
    const quint16 port = tcp ? freeLoopbackTcpPort() : freeLoopbackUdpPort();
    QVERIFY(port != 0);
    MockVideoStreamServer server;
    QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), port));
    if (type == MockVideoStreamServer::StreamType::RtspH264) {
        ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg, GstTest::libproxyIgnoreMaskWarning());
    }

    // The rows cover the stream paths, not decoder choice. CI's macOS VMs can't hardware-decode H.265, and
    // decodebin3 doesn't fall back from a hardware decoder that fails to start. Outlives the receiver.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream(server.servedUri());
    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, 1, TestTimeout::mediumMs());
    QVERIFY(lastBool(stream.decodingSpy));
#endif
}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("suffix");

    QTest::newRow("mp4") << int(VideoReceiver::FILE_FORMAT_MP4) << QStringLiteral("mp4");
    QTest::newRow("mov") << int(VideoReceiver::FILE_FORMAT_MOV) << QStringLiteral("mov");
}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress()
{
    QFETCH(int, format);
    QFETCH(QString, suffix);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start());

    const QString path = tempDir.filePath(QStringLiteral("in-progress.") + suffix);
    stream.receiver->startRecording(path, static_cast<VideoReceiver::FILE_FORMAT>(format));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::mediumMs());

    // A copy taken while recording is what a crash or power loss would leave on disk.
    const QString snapshot = tempDir.filePath(QStringLiteral("snapshot.") + suffix);
    QString failure;
    const bool playable = UnitTest::waitForCondition(
        [&] {
            (void) QFile::remove(snapshot);
            return QFile::copy(path, snapshot) && (countPlayableFrames(snapshot, failure) > 0);
        },
        TestTimeout::mediumDuration(), u"mid-recording copy is playable");
    QVERIFY2(playable, qPrintable(failure));
}

void GstVideoReceiverTest::_testDestroyWhileRecordingFinalizesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start());

    const QString path = tempDir.filePath(QStringLiteral("destroyed.mp4"));
    QVERIFY(stream.record(path));

    QElapsedTimer teardown;
    teardown.start();
    stream.receiver.reset();
    QVERIFY2(teardown.elapsed() < TestTimeout::mediumMs(),
             qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testSecondRecordingIsNotTruncated()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start());

    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    QVERIFY(stream.record(first));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopRecordingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.stopRecordingSpy), VideoReceiver::STATUS_OK);

    // The first recording's fragment-closed message is still queued on the pipeline bus.
    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(second));
    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());

    for (const QString& path : {first, second}) {
        const QString problem = recordingProblem(path);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverTest::_testReconnectBacksOffAfterFailedStart()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));

    StreamingReceiver stream(loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()));
    stream.receiver->setAutoReconnect(true);

    // The first start and its retry both fail on the bound port.
    _expectFailedStartWarnings();
    _expectFailedStartWarnings();

    QElapsedTimer clock;
    clock.start();
    stream.receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startSpy, 1, TestTimeout::mediumMs());
    const qint64 firstFailure = clock.elapsed();
    QCOMPARE(lastStatus(stream.startSpy), VideoReceiver::STATUS_FAIL);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startSpy, 2, TestTimeout::mediumMs());
    const qint64 secondFailure = clock.elapsed();
    QCOMPARE(lastStatus(stream.startSpy), VideoReceiver::STATUS_FAIL);
    for (int attempt = 0; attempt < 4; ++attempt) {
        verifyExpectedLogMessage();
    }

    portBlocker.close();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startSpy, 3, TestTimeout::mediumMs());
    const qint64 recovery = clock.elapsed();
    QCOMPARE(lastStatus(stream.startSpy), VideoReceiver::STATUS_OK);
    QCOMPARE(stream.startSpy.count(), 3);

    QVERIFY2((secondFailure - firstFailure) >= (kFirstReconnectDelayMs - kSignalLatencySlackMs),
             qPrintable(QStringLiteral("first retry after %1 ms").arg(secondFailure - firstFailure)));
    QVERIFY2((recovery - secondFailure) >= (kSecondReconnectDelayMs - kSignalLatencySlackMs),
             qPrintable(QStringLiteral("second retry after %1 ms").arg(recovery - secondFailure)));
}

void GstVideoReceiverTest::_testStreamLossRestartPolicy_data()
{
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("auto-reconnect") << true;
    QTest::newRow("no auto-reconnect") << false;
}

void GstVideoReceiverTest::_testStreamLossRestartPolicy()
{
    QFETCH(bool, autoReconnect);

    StreamingReceiver stream;
    stream.receiver->setAutoReconnect(autoReconnect);
    QVERIFY(stream.startSending());

    // The shortest watchdog timeout, so losing the stream is noticed within about two seconds.
    QVERIFY(stream.startStreaming(1));

    stream.sender.stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.stopSpy), VideoReceiver::STATUS_FAIL);
    QCOMPARE(stream.timeoutSpy.count(), 1);

    // _testReconnectBacksOffAfterFailedStart checks the reconnect delay.
    if (autoReconnect) {
        QVERIFY_SIGNAL_COUNT_WAIT(stream.startSpy, 2, TestTimeout::mediumMs());
        QCOMPARE(lastStatus(stream.startSpy), VideoReceiver::STATUS_OK);
    } else {
        QVERIFY_NO_SIGNAL_WAIT(stream.startSpy, kFirstReconnectDelayMs + kSignalLatencySlackMs);
    }
}

void GstVideoReceiverTest::_testDecodingRequestedBeforeStreamStarts()
{
    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.port != 0);

    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startDecodingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.startDecodingSpy), VideoReceiver::STATUS_OK);

    // Nothing is sent until decoding was requested, so the source pad appears only now.
    QVERIFY(stream.startSending());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, 1, TestTimeout::mediumMs());
    QVERIFY(lastBool(stream.decodingSpy));
}

void GstVideoReceiverTest::_testStopDecodingBeforeFirstFrame()
{
    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.startStreaming());

    // Queued back to back, so the decoder is torn down before it can deliver a frame.
    stream.receiver->startDecoding(videoSink.get());
    stream.receiver->stopDecoding();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopDecodingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.stopDecodingSpy), VideoReceiver::STATUS_OK);

    stream.receiver->startDecoding(videoSink.get());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startDecodingSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.startDecodingSpy), VideoReceiver::STATUS_OK);
    QVERIFY_TRUE_WAIT(!stream.decodingSpy.isEmpty() && lastBool(stream.decodingSpy), TestTimeout::mediumMs());
}

void GstVideoReceiverTest::_testStopWhileStoppingRecordingFinalizesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start());

    const QString path = tempDir.filePath(QStringLiteral("stopped.mp4"));
    QVERIFY(stream.record(path));

    // stop() lands while the muxer is still finalizing the file stopRecording() ended.
    stream.receiver->stopRecording();
    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(stream.stopRecordingSpy.count(), 1);
    QCOMPARE(lastStatus(stream.stopRecordingSpy), VideoReceiver::STATUS_OK);

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testRecordingErrorKeepsStreaming()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.startStreaming());

    // splitmuxsink opens the file once it has a GOP, so the failure surfaces mid-recording. GStreamer adds
    // follow-up errors (failed state change, no output sink) whose number isn't ours to pin.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .*Could not open file")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("missing/unwritable.mp4")),
                                    VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingSpy, 2, TestTimeout::mediumMs());
    QVERIFY(!lastBool(stream.recordingSpy));
    verifyExpectedLogMessage();

    QCOMPARE(stream.streamingSpy.count(), 1);
    QVERIFY(stream.stopSpy.isEmpty());

    // The failed recording had already reported its first keyframe; the muxer only fails after it.
    const QString path = tempDir.filePath(QStringLiteral("recovered.mp4"));
    QVERIFY(stream.record(path));
    QCOMPARE(stream.recordingStartedSpy.count(), 2);
    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testFailedRecordingStartLeavesBranchUsable()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.startStreaming());

    // A child holding the recording sink's name makes the pipeline refuse the sink.
    GstElement* nameTaken = nullptr;
    QVERIFY(_runOnPipeline(stream.receiver.get(), [&nameTaken](GstElement* pipeline) {
        nameTaken = gst_bin_new("sinkbin");
        if (!gst_bin_add(GST_BIN(pipeline), nameTaken)) {
            nameTaken = nullptr;
        }
    }));
    QVERIFY(nameTaken);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to add the recording sink")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("refused.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startRecordingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.startRecordingSpy), VideoReceiver::STATUS_FAIL);
    verifyExpectedLogMessage();
    QVERIFY(stream.recordingSpy.isEmpty());

    QVERIFY(_runOnPipeline(stream.receiver.get(),
                           [nameTaken](GstElement* pipeline) { (void) gst_bin_remove(GST_BIN(pipeline), nameTaken); }));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("accepted.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.startRecordingSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stream.startRecordingSpy), VideoReceiver::STATUS_OK);
}

void GstVideoReceiverTest::_testRecordingStartKeepsPipelinePlaying()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    StreamingReceiver stream;
    QVERIFY(stream.startSending());

    // Read on the worker that changes it; "PAUSED->PLAYING" while a change is pending.
    const auto pipelineState = [&stream]() {
        QString state;
        (void) _runOnPipeline(stream.receiver.get(), [&state](GstElement* pipeline) {
            GstState current = GST_STATE_VOID_PENDING;
            GstState pending = GST_STATE_VOID_PENDING;
            (void) gst_element_get_state(pipeline, &current, &pending, 0);
            state = QString::fromUtf8(gst_element_state_get_name(current));
            if (pending != GST_STATE_VOID_PENDING) {
                state += QStringLiteral("->") + QString::fromUtf8(gst_element_state_get_name(pending));
            }
        });
        return state;
    };

    QVERIFY(stream.startStreaming());
    QCOMPARE_TRUE_WAIT(pipelineState(), QStringLiteral("PLAYING"), TestTimeout::mediumMs());

    // The file sink can't preroll before the next keyframe; the live pipeline must not wait for it meanwhile.
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("recording.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
}

void GstVideoReceiverTest::_testDecoderQosDisabled()
{
    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, 1, TestTimeout::mediumMs());

    // The decoders decodebin3 plugged, inspected on the worker that owns the pipeline.
    int decoders = 0;
    QStringList qosEnabled;
    QVERIFY(_runOnPipeline(stream.receiver.get(), [&decoders, &qosEnabled](GstElement* pipeline) {
        for (const GStreamer::GstElementPtr& element : GstTest::childElements(GST_BIN(pipeline), /*recurse=*/true)) {
            GstElementFactory* const factory = gst_element_get_factory(element.get());
            if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) ||
                !g_object_class_find_property(G_OBJECT_GET_CLASS(element.get()), "qos")) {
                continue;
            }
            ++decoders;
            gboolean qos = TRUE;
            g_object_get(element.get(), "qos", &qos, nullptr);
            if (qos) {
                qosEnabled.append(QString::fromUtf8(GST_OBJECT_NAME(element.get())));
            }
        }
    }));
    QVERIFY(decoders > 0);
    QVERIFY2(qosEnabled.isEmpty(), qPrintable(qosEnabled.join(u',')));
}

void GstVideoReceiverTest::_testDecoderNameReported()
{
    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decoderNameSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!stream.gstReceiver->decoderName().isEmpty());
}

void GstVideoReceiverTest::_testVideoSizeFromNegotiatedCaps()
{
    const QgcVideoSinkPtr videoSink = makeQgcVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.sizeSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(stream.sizeSpy.constLast().constFirst().toSize(), QSize(kFrameWidth, kFrameHeight));
}

void GstVideoReceiverTest::_testScreenshotSavesDisplayedFrame()
{
    const QgcVideoSinkPtr videoSink = makeQgcVideoSink();
    QVERIFY(videoSink);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QQuickVideoOutput videoOutput;
    StreamingReceiver stream(QString(), &videoOutput);
    QVERIFY(stream.startSending());
    // What attachAppSink() does once the widget's window is visible; there is no window here.
    QVERIFY(GStreamer::setupQVideoSinkElement(videoSink.get(), videoOutput.videoSink(), stream.receiver.get()));

    const QString path = tempDir.filePath(QStringLiteral("screenshot.png"));
    expectLogMessage("Video.VideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video frame to take a screenshot from")));
    stream.receiver->takeScreenshot(path);
    verifyExpectedLogMessage();
    QCOMPARE(stream.screenshotSpy.count(), 1);
    QCOMPARE(lastStatus(stream.screenshotSpy), VideoReceiver::STATUS_FAIL);

    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_TRUE_WAIT(videoOutput.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    stream.receiver->takeScreenshot(path);
    QCOMPARE(stream.screenshotSpy.count(), 2);
    QCOMPARE(lastStatus(stream.screenshotSpy), VideoReceiver::STATUS_OK);
    QCOMPARE(QImage(path).size(), QSize(kFrameWidth, kFrameHeight));
}

void GstVideoReceiverTest::_testStopReleasesDisplayedFrame()
{
    const QgcVideoSinkPtr videoSink = makeQgcVideoSink();
    QVERIFY(videoSink);
    QQuickVideoOutput videoOutput;
    StreamingReceiver stream(QString(), &videoOutput);
    QVERIFY(stream.startSending());
    // Bound once per receiver, as VideoManager does through attachAppSink().
    QVERIFY(GStreamer::setupQVideoSinkElement(videoSink.get(), videoOutput.videoSink(), stream.receiver.get()));

    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_TRUE_WAIT(videoOutput.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    stream.receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.stopSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!videoOutput.videoSink()->videoFrame().isValid(), TestTimeout::shortMs());

    // Releasing the frame must not unbind the sink: the next stream is displayed again.
    QVERIFY(stream.start(videoSink.get()));
    QVERIFY_TRUE_WAIT(videoOutput.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());
}

void GstVideoReceiverTest::_testBusDoesNotQueueMessages()
{
    const GStreamer::GstElementPtr videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    StreamingReceiver stream;
    QVERIFY(stream.startSending());
    QVERIFY(stream.startStreaming());
    stream.receiver->startDecoding(videoSink.get());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, 1, TestTimeout::mediumMs());

    // State changes, stream collection, latency and QoS have all been posted by now.
    bool pending = true;
    QVERIFY(_runOnPipeline(stream.receiver.get(), [&pending](GstElement* pipeline) {
        const GStreamer::GstBusPtr bus = GStreamer::adoptBus(gst_pipeline_get_bus(GST_PIPELINE(pipeline)));
        pending = gst_bus_have_pending(bus.get());
    }));
    QVERIFY2(!pending, "Messages are piling up on the pipeline bus");
}

#else

void GstVideoReceiverTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverTest, TestLabel::Integration)
