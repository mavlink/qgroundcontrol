#include "GstVideoReceiverTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <condition_variable>
#include <gst/gst.h>
#include <memory>
#include <mutex>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QUdpSocket>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "BranchContext.h"
#include "GStreamer.h"
#include "GStreamerHelpers.h"
#include "GStreamerLogging.h"
#include "GStreamerTestHelpers.h"
#include "GstVideoReceiver.h"
#include "MockVideoStreamServer.h"
#include "RecordingBranch.h"
#include "SettingsManager.h"
#include "VideoReceiver.h"
#include "VideoSettings.h"

namespace {

using State = VideoReceiver::State;
using FileFormat = VideoReceiver::FileFormat;

// The stream GstTest::FixtureRtpSender sends: 320x240 at 30 fps, a keyframe every 5 frames.
constexpr int kFrameRate = GstTest::kStreamFrameRate;
constexpr int kFrameWidth = 320;
constexpr int kFrameHeight = 240;
// Frames the sender produces while a recording is open. Some may still be in flight when the
// recording ends, but a finalized file holds far more than kMinRecordedFrames; a file cut off
// before its final moov holds only the samples of the first header update (one).
constexpr int kRecordFrames = 15;
constexpr int kMinRecordedFrames = 5;
constexpr std::chrono::seconds kWatchdogTimeout{5};
// The shortest watchdog timeout, so losing the stream is noticed within about two seconds.
constexpr std::chrono::seconds kShortWatchdogTimeout{1};
// The first reconnect follows a failure after 1 s.
constexpr qint64 kFirstReconnectDelayMs = 1000;
// GUI-thread delivery of the worker's reports lags by up to an event-loop pass, more so on a loaded host.
constexpr qint64 kSignalLatencySlackMs = 200;
// A window that shows a reconnect would have started: the first one follows a failure after kFirstReconnectDelayMs.
constexpr int kNoReconnectWindowMs = static_cast<int>(kFirstReconnectDelayMs + kSignalLatencySlackMs);
// A recording that waits this long to close was closed by the finalize timeout, not by its recorder's EOS.
constexpr qint64 kFinalizeTimeoutMs = std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout).count();

enum class RecordingEnd
{
    Stopped,
    RecorderError,
    Deactivated,
};

QString loopbackUri(QLatin1StringView scheme, quint16 port)
{
    return QStringLiteral("%1://127.0.0.1:%2").arg(scheme).arg(port);
}

QString udpListenUri()
{
    return loopbackUri(QLatin1StringView("udp"), 0);
}

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

/// Accepts RTSP connections and never answers, as a camera whose RTSP service hung (after Qt's UnResponsiveRtspServer,
/// qtmultimedia/tests/auto/integration/qmediaplayerbackend/server.h).
class UnresponsiveRtspServer
{
public:
    UnresponsiveRtspServer()
    {
        QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] {
            while (_server.nextPendingConnection()) {
                ++_connections;
            }
        });
    }

    [[nodiscard]] bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    QString uri() const { return QStringLiteral("rtsp://127.0.0.1:%1/test").arg(_server.serverPort()); }

    int connections() const { return _connections; }

private:
    QTcpServer _server;
    int _connections = 0;
};

/// Holds a streaming thread inside a pad probe until released; shared with the probe, which may outlive the test.
struct Gate
{
    void wait()
    {
        std::unique_lock lock(mutex);
        released.wait(lock, [this] { return open; });
    }

    void release()
    {
        {
            const std::lock_guard lock(mutex);
            open = true;
        }
        released.notify_all();
    }

    std::mutex mutex;
    std::condition_variable released;
    bool open = false;
};

/// The pipeline's first element (recursively) that @p match accepts.
GStreamer::GstElementPtr findElement(GstElement* pipeline, const std::function<bool(GstElement*)>& match)
{
    if (!pipeline) {
        return nullptr;
    }
    for (GStreamer::GstElementPtr& element : GstTest::childElements(GST_BIN(pipeline), /*recurse=*/true)) {
        if (match(element.get())) {
            return std::move(element);
        }
    }
    return nullptr;
}

bool isFactory(GstElement* element, const char* factoryName)
{
    GstElementFactory* const factory = gst_element_get_factory(element);
    return factory && (g_strcmp0(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)), factoryName) == 0);
}

/// The head (queue or valve) of the tee branch @p branch.
GStreamer::GstElementPtr branchHead(GstElement* pipeline, GStreamer::PipelineBranch branch, const char* factoryName)
{
    return findElement(pipeline, [branch, factoryName](GstElement* element) {
        return isFactory(element, factoryName) && (GStreamer::branchOf(GST_OBJECT(element)) == branch);
    });
}

/// A GstVideoReceiver receiving @p uri, with spies on its signals, a VideoOutput in a shown window, and a sender of the
/// test stream @p stream for its UDP port.
struct Receiver
{
    explicit Receiver(const QString& uri = udpListenUri(), bool autoReconnect = false,
                      std::chrono::seconds timeout = kWatchdogTimeout,
                      const QString& stream = QString::fromLatin1(GstTest::kH264Stream))
        : output(new QQuickVideoOutput(window.contentItem()))
        , receiver(new GstVideoReceiver())
        , stateSpy(receiver.get(), &VideoReceiver::stateChanged)
        , streamingSpy(receiver.get(), &VideoReceiver::streamingChanged)
        , decodingSpy(receiver.get(), &VideoReceiver::decodingChanged)
        , recordingSpy(receiver.get(), &VideoReceiver::recordingChanged)
        , recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted)
        , recordingFinishedSpy(receiver.get(), &VideoReceiver::recordingFinished)
        , sizeSpy(receiver.get(), &VideoReceiver::videoSizeChanged)
        , errorSpy(receiver.get(), &VideoReceiver::errorOccurred)
        , sender(stream)
    {
        window.resize(kFrameWidth, kFrameHeight);
        output->setSize(QSizeF(kFrameWidth, kFrameHeight));
        receiver->setName(QStringLiteral("test"));
        receiver->setConfig({.uri = uri, .timeout = timeout, .autoReconnect = autoReconnect});
    }

    ~Receiver() { receiver.reset(); }

    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;

    [[nodiscard]] bool waitForState(State state) const
    {
        return UnitTest::waitForCondition([this, state] { return receiver->state() == state; },
                                          TestTimeout::mediumDuration(), u"receiver state");
    }

    /// The UDP port the receiver's source listens on, 0 without one.
    quint16 udpPort() const
    {
        gint port = 0;
        (void) GstVideoReceiverTest::runOnPipeline(receiver.get(), [&port](GstElement* pipeline) {
            if (const GStreamer::GstElementPtr source =
                    findElement(pipeline, [](GstElement* element) { return isFactory(element, "udpsrc"); })) {
                g_object_get(source.get(), "port", &port, nullptr);
            }
        });
        return static_cast<quint16>(port);
    }

    /// Activates the receiver and, once its pipeline listens, starts sending to it.
    [[nodiscard]] bool startStreaming()
    {
        receiver->setActive(true);
        return waitForState(State::Connecting) && sender.start(udpPort()) && waitForState(State::Streaming);
    }

    /// Sends to the port of the receiver's current pipeline, once it listens again.
    [[nodiscard]] bool resumeSending()
    {
        return waitForState(State::Connecting) && sender.start(udpPort()) && waitForState(State::Streaming);
    }

    /// Displays the stream in the visible window's VideoOutput.
    [[nodiscard]] bool display()
    {
        window.show();
        receiver->setVideoOutput(output);
        return UnitTest::waitForCondition([this] { return receiver->decoding(); }, TestTimeout::mediumDuration(),
                                          u"decoding");
    }

    QVideoSink* videoSink() const { return output->videoSink(); }

    /// A ref on the current pipeline, which keeps its address from being reused by the next one.
    GStreamer::GstElementPtr pipeline() const
    {
        GStreamer::GstElementPtr current;
        (void) GstVideoReceiverTest::runOnPipeline(receiver.get(), [&current](GstElement* pipeline) {
            current = GStreamer::adoptElement(pipeline ? GST_ELEMENT(gst_object_ref(pipeline)) : nullptr);
        });
        return current;
    }

    /// startRecording(), then waits for the file to open and for kRecordFrames more frames sent into it.
    [[nodiscard]] bool record(const QString& path, FileFormat format = FileFormat::Mp4)
    {
        const int started = recordingStartedSpy.count();
        receiver->startRecording(path, format);
        if (!UnitTest::waitForSignalCount(recordingStartedSpy, started + 1, TestTimeout::mediumDuration(),
                                          u"recordingStarted")) {
            return false;
        }
        return framesSent(kRecordFrames);
    }

    [[nodiscard]] bool framesSent(int count)
    {
        const int first = sender.frames();
        return UnitTest::waitForCondition([this, first, count] { return sender.frames() >= (first + count); },
                                          TestTimeout::mediumDuration(), u"frames sent");
    }

    /// Holds the recorder's filesink until @p gate opens.
    [[nodiscard]] bool blockRecorder(const std::shared_ptr<Gate>& gate) const
    {
        bool blocked = false;
        (void) GstVideoReceiverTest::runOnPipeline(receiver.get(), [&blocked, gate](GstElement* pipeline) {
            const GStreamer::GstElementPtr fileSink =
                findElement(pipeline, [](GstElement* element) { return isFactory(element, "filesink"); });
            const GStreamer::GstPadPtr pad =
                GStreamer::adoptPad(fileSink ? gst_element_get_static_pad(fileSink.get(), "sink") : nullptr);
            blocked = pad && gst_pad_add_probe(
                                 pad.get(), GST_PAD_PROBE_TYPE_BUFFER,
                                 [](GstPad*, GstPadProbeInfo*, gpointer data) {
                                     (*static_cast<std::shared_ptr<Gate>*>(data))->wait();
                                     return GST_PAD_PROBE_OK;
                                 },
                                 new std::shared_ptr<Gate>(gate),
                                 [](gpointer data) { delete static_cast<std::shared_ptr<Gate>*>(data); });
        });
        return blocked;
    }

    static QString lastFinished(const QSignalSpy& spy, int argument) { return spy.constLast().at(argument).toString(); }

    QQuickWindow window;
    QQuickVideoOutput* const output;
    std::unique_ptr<GstVideoReceiver> receiver;
    QSignalSpy stateSpy;
    QSignalSpy streamingSpy;
    QSignalSpy decodingSpy;
    QSignalSpy recordingSpy;
    QSignalSpy recordingStartedSpy;
    QSignalSpy recordingFinishedSpy;
    QSignalSpy sizeSpy;
    QSignalSpy errorSpy;
    GstTest::FixtureRtpSender sender;
};

}  // namespace

bool GstVideoReceiverTest::runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement*)>& task)
{
    return receiver && receiver->_runOnPipeline(task);
}

GstElement* GstVideoReceiverTest::displaySink(const GstVideoReceiver* receiver)
{
    return receiver->_displaySink.get();
}

void GstVideoReceiverTest::initTestCase()
{
    UnitTest::initTestCase();

    GStreamer::redirectGLibLogging();
    if (GstTest::initGStreamer().isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
    // The display sink follows VideoSettings; the CPU path needs no GPU on the test host.
    SettingsManager::instance()->videoSettings()->forceCpuVideoPath()->setRawValue(true);
}

void GstVideoReceiverTest::init()
{
    QString unavailable = GstTest::initGStreamer();
    if (unavailable.isEmpty()) {
        const QString missing = GstTest::firstMissingFactory(
            {"appsrc", "h264parse", "rtph264pay", "udpsink", "udpsrc", "rtph264depay", "parsebin", "decodebin3",
             "valve", "tee", "filesink", "mp4mux", "qtmux", "qtdemux", "qgcvideosinkbin"});
        if (!missing.isEmpty()) {
            unavailable = QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing);
        }
    }
    if (!unavailable.isEmpty()) {
        QSKIP(qPrintable(unavailable));
    }

    UnitTest::init();
}

void GstVideoReceiverTest::_testStreamingStatesAndDisplay()
{
    Receiver stream;
    QCOMPARE(stream.receiver->state(), State::Stopped);
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // Every state on the way, each once, and streaming exactly while Streaming.
    QCOMPARE(stream.stateSpy.count(), 2);
    QCOMPARE(stream.stateSpy.at(0).constFirst().value<State>(), State::Connecting);
    QCOMPARE(stream.stateSpy.at(1).constFirst().value<State>(), State::Streaming);
    QCOMPARE(stream.streamingSpy.count(), 1);
    QVERIFY(stream.receiver->streaming());

    // The QVideoSink's size, so it is the displayed one.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.sizeSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(stream.receiver->videoSize(), QSize(kFrameWidth, kFrameHeight));
    QVERIFY_TRUE_WAIT(!stream.receiver->decoderName().isEmpty(), TestTimeout::mediumMs());

    const quint16 port = stream.udpPort();
    stream.receiver->setActive(false);
    QVERIFY(stream.waitForState(State::Stopped));
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::shortMs());
    QVERIFY(!stream.receiver->streaming());
    QVERIFY(stream.errorSpy.isEmpty());

    QUdpSocket rebind;
    QVERIFY2(rebind.bind(QHostAddress::LocalHost, port, QAbstractSocket::DontShareAddress),
             "Stopped receiver still holds its UDP port");
}

void GstVideoReceiverTest::_testStaticSourcePadWaitsForData()
{
    // udp265 exposes its source pad before any data, which used to be reported as streaming at once.
    Receiver stream(loopbackUri(QLatin1StringView("udp265"), 0));
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    QVERIFY(stream.streamingSpy.isEmpty());
    QVERIFY(!stream.receiver->streaming());
}

void GstVideoReceiverTest::_testUdpSourceStopsRightAfterPlaying()
{
    // An unfixed udpsrc corrupts the heap in most runs of a few thousand cycles; each takes well under a millisecond.
    constexpr int kImmediateStopCycles = 10000;

    // The receiver's UDP sources are udpsrc. Leaving PLAYING while its streaming thread still clears the previous
    // unlock used to cancel a freed GCancellable and corrupt the heap; GStreamer::completeInit() serializes the two.
    // Downstream elements delay the source's own state change and hide the race, so the source runs bare.
    for (int cycle = 0; cycle < kImmediateStopCycles; ++cycle) {
        GstTest::TestPipeline pipeline;
        QVERIFY2(pipeline.launch(QStringLiteral("udpsrc address=127.0.0.1 port=0 ! fakesink")),
                 qPrintable(pipeline.error()));
        QVERIFY(pipeline.play());
        QCOMPARE(gst_element_set_state(pipeline.get(), GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
    }
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

    using StreamType = MockVideoStreamServer::StreamType;
    const auto type = static_cast<StreamType>(streamType);
    if (!MockVideoStreamServer::isStreamTypeAvailable(type)) {
        QSKIP("This GStreamer install lacks the stream type's encoder or muxer");
    }
    if (type == StreamType::RtspH264) {
        ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg, GstTest::libproxyIgnoreMaskWarning());
    }

    // The rows cover the stream paths, not decoder choice. CI's macOS VMs can't hardware-decode H.265, and
    // decodebin3 doesn't fall back from a hardware decoder that fails to start. Outlives the receiver.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    // Servers listen on a free port; for UDP the receiver does, and the server sends to it.
    MockVideoStreamServer server;
    const bool receiverListens =
        (type == StreamType::RtpUdpH264) || (type == StreamType::RtpUdpH265) || (type == StreamType::MpegTsUdp);
    QString uri;
    if (receiverListens) {
        const QLatin1StringView scheme = (type == StreamType::RtpUdpH264)   ? QLatin1StringView("udp")
                                         : (type == StreamType::RtpUdpH265) ? QLatin1StringView("udp265")
                                                                            : QLatin1StringView("mpegts");
        uri = loopbackUri(scheme, 0);
    } else {
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), 0));
        uri = server.servedUri();
    }

    Receiver stream(uri);
    stream.receiver->setActive(true);
    if (receiverListens) {
        // Connecting lasts until the server sends. A server that already does can take the receiver on to
        // Streaming before this thread sees Connecting.
        QVERIFY(stream.waitForState(State::Connecting));
        QVERIFY(server.start(type, QStringLiteral("127.0.0.1"), stream.udpPort()));
    }
    QVERIFY(stream.waitForState(State::Streaming));
    QVERIFY(stream.display());
#endif
}

void GstVideoReceiverTest::_testConfigChangeRestarts()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const GStreamer::GstElementPtr first = stream.pipeline();

    // Live: no restart.
    VideoReceiver::Config config = stream.receiver->config();
    config.autoReconnect = !config.autoReconnect;
    stream.receiver->setConfig(config);
    QCOMPARE(stream.pipeline().get(), first.get());
    QCOMPARE(stream.receiver->state(), State::Streaming);

    // Anything else applies to a new pipeline at once.
    stream.stateSpy.clear();
    config.lowLatency = !config.lowLatency;
    stream.receiver->setConfig(config);
    QVERIFY(stream.resumeSending());
    QVERIFY(stream.pipeline().get() != first.get());
    QCOMPARE(stream.stateSpy.constFirst().constFirst().value<State>(), State::Connecting);
    QVERIFY(stream.errorSpy.isEmpty());

    // An empty URI stops the stream until one is set again.
    config.uri.clear();
    stream.receiver->setConfig(config);
    QVERIFY(stream.waitForState(State::Stopped));
    config.uri = udpListenUri();
    stream.receiver->setConfig(config);
    QVERIFY(stream.resumeSending());
}

void GstVideoReceiverTest::_testOutputDetachKeepsPipeline()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const GStreamer::GstElementPtr pipeline = stream.pipeline();
    stream.stateSpy.clear();

    stream.receiver->setVideoOutput(nullptr);
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::mediumMs());

    // A destroyed output detaches too; a new one shows the stream again.
    auto* const replacement = new QQuickVideoOutput(stream.window.contentItem());
    replacement->setSize(QSizeF(kFrameWidth, kFrameHeight));
    stream.receiver->setVideoOutput(replacement);
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(replacement->videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());
    delete replacement;
    QCOMPARE(stream.receiver->videoOutput(), nullptr);
    QVERIFY_TRUE_WAIT(!stream.receiver->decoding(), TestTimeout::mediumMs());

    QVERIFY(stream.display());
    QCOMPARE(stream.pipeline().get(), pipeline.get());
    QVERIFY2(stream.stateSpy.isEmpty(), "Changing the output restarted the stream");
}

void GstVideoReceiverTest::_testRestartRebuildsVideoSink()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const GStreamer::GstElementPtr firstSink =
        GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(displaySink(stream.receiver.get()))));
    const GStreamer::GstElementPtr firstPipeline = stream.pipeline();

    stream.receiver->restart();
    QVERIFY(displaySink(stream.receiver.get()) != firstSink.get());
    QVERIFY(stream.resumeSending());
    QVERIFY(stream.pipeline().get() != firstPipeline.get());
    QVERIFY_TRUE_WAIT(stream.receiver->decoding(), TestTimeout::mediumMs());
    // Only the receiver's ref and this one were ever taken on the replaced sink; the pipeline let go of it.
    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(firstSink.get()), 1);
}

void GstVideoReceiverTest::_testDecoderFailureKeepsRecording()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    const QString path = tempDir.filePath(QStringLiteral("decoder-failure.mp4"));
    QVERIFY(stream.record(path));
    const int decodingChanges = stream.decodingSpy.count();

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Decoder error from .*injected decoder failure")));
    QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
        const GStreamer::GstElementPtr decoder = GstTest::findByFactory(GST_BIN(pipeline), "decodebin3");
        GError* const error =
            g_error_new_literal(GST_STREAM_ERROR, GST_STREAM_ERROR_DECODE, "injected decoder failure");
        (void) gst_element_post_message(decoder.get(),
                                        gst_message_new_error(GST_OBJECT(decoder.get()), error, nullptr));
        g_error_free(error);
    }));

    // A new decoder takes over; the recording and the stream go on.
    QVERIFY_SIGNAL_COUNT_WAIT(stream.decodingSpy, decodingChanges + 2, TestTimeout::mediumMs());
    QVERIFY(stream.receiver->decoding());
    verifyExpectedLogMessage();
    QVERIFY(stream.framesSent(kRecordFrames));
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.recordingFinishedSpy.isEmpty());

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testReconnectResumesRecordingInNextPart()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("resumed.mp4"));
    QVERIFY(stream.record(path));

    // The camera drops out: the first part is closed and the recording waits for the stream to come back.
    stream.sender.stop();
    QVERIFY(stream.waitForState(State::Reconnecting));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY(stream.receiver->recording());

    QVERIFY(stream.resumeSending());
    const QString part2 = tempDir.filePath(QStringLiteral("resumed_part2.mp4"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(), part2);
    QVERIFY(stream.framesSent(kRecordFrames));

    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), part2);
    QVERIFY(stream.errorSpy.isEmpty());
    for (const QString& file : {path, part2}) {
        const QString problem = recordingProblem(file);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverTest::_testSlowRecordingDoesNotStallDisplay()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    // Opened before the receiver goes, so a failing test can't leave the recorder held.
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // A small backlog, so storage that stops taking data shows quickly.
    constexpr guint kBacklogBytes = 16 * 1024;
    std::atomic<int> displayed{0};
    GStreamer::PadProbe displayProbe;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&displayed, &displayProbe](GstElement* pipeline) {
        const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
        g_object_set(queue.get(), "max-size-bytes", kBacklogBytes, nullptr);
        const GStreamer::GstElementPtr valve = branchHead(pipeline, GStreamer::PipelineBranch::Decoding, "valve");
        const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(valve.get(), "src"));
        (void) displayProbe.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &displayed);
    }));

    const QString path = tempDir.filePath(QStringLiteral("slow.mp4"));
    QVERIFY(stream.record(path));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording storage too slow")));
    // Whether the drained file closes before the finalize timeout depends on when the gate opens.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording not finalized within")));
    QVERIFY(stream.blockRecorder(gate));

    // The backlog fills; the recording gives up on it instead of holding up the tee.
    const auto recorderLeaks = [&stream] {
        gint leaky = 0;
        (void) runOnPipeline(stream.receiver.get(), [&leaky](GstElement* pipeline) {
            const GStreamer::GstElementPtr queue = branchHead(pipeline, GStreamer::PipelineBranch::Recording, "queue");
            g_object_get(queue.get(), "leaky", &leaky, nullptr);
        });
        return leaky != 0;
    };
    QVERIFY_TRUE_WAIT(recorderLeaks(), TestTimeout::mediumMs());

    // The display keeps getting frames while the storage stays stuck.
    const int displayedBefore = displayed.load(std::memory_order_relaxed);
    QVERIFY_TRUE_WAIT(displayed.load(std::memory_order_relaxed) >= (displayedBefore + kFrameRate),
                      TestTimeout::mediumMs());
    QCOMPARE(stream.receiver->state(), State::Streaming);
    verifyExpectedLogMessage();

    // The file had frames, so its recordingFinished() carries the error, and nothing else does.
    gate->release();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!Receiver::lastFinished(stream.recordingFinishedSpy, 1).isEmpty());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.errorSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);
}

void GstVideoReceiverTest::_testPermanentFailureStops()
{
    Receiver stream(QStringLiteral("ftp://127.0.0.1:5600/video"), /*autoReconnect=*/true);
    expectLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Unsupported URI scheme")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Video stream stopped:")));
    stream.receiver->setActive(true);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QCOMPARE(stream.receiver->state(), State::Stopped);

    // Recording needs a stream that runs.
    stream.receiver->startRecording(QStringLiteral("unused.mp4"), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 2, TestTimeout::mediumMs());
    QVERIFY(stream.recordingSpy.isEmpty());

    // Not even an attempt to reconnect, though autoReconnect is on.
    QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kNoReconnectWindowMs);
    QCOMPARE(stream.errorSpy.count(), 2);
}

void GstVideoReceiverTest::_testReconnectBacksOffAfterFailedStart()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));
    Receiver stream(loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()), /*autoReconnect=*/true);

    // The start fails on the bound port; the retry, once it is free, succeeds.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from ")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to start")));
    // Timed from the start, which fails at once: the retry comes at least the first reconnect delay later however
    // late this thread sees the state changes.
    QElapsedTimer clock;
    clock.start();
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Reconnecting));
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    portBlocker.close();
    QVERIFY(stream.waitForState(State::Connecting));
    const qint64 retry = clock.elapsed();
    QVERIFY2(retry >= kFirstReconnectDelayMs, qPrintable(QStringLiteral("retried after %1 ms").arg(retry)));
    QVERIFY(stream.errorSpy.isEmpty());
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

    Receiver stream(udpListenUri(), autoReconnect, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());

    if (!autoReconnect) {
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Video stream stopped:")));
    }
    stream.sender.stop();

    if (autoReconnect) {
        QVERIFY(stream.waitForState(State::Reconnecting));
        QVERIFY(stream.resumeSending());
        QVERIFY(stream.errorSpy.isEmpty());
    } else {
        QVERIFY(stream.waitForState(State::Stopped));
        QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::shortMs());
        verifyExpectedLogMessage();
        stream.stateSpy.clear();
        QVERIFY_NO_SIGNAL_WAIT(stream.stateSpy, kNoReconnectWindowMs);
    }
}

void GstVideoReceiverTest::_testDeactivateFinishesRecording()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("deactivated.mp4"));
    QVERIFY(stream.record(path));
    QVERIFY(stream.receiver->recording());

    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    QVERIFY(stream.waitForState(State::Stopped));

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // Activating again doesn't bring the recording back.
    QVERIFY(stream.startStreaming());
    QVERIFY(!stream.receiver->recording());
    QCOMPARE(stream.recordingStartedSpy.count(), 1);
}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress_data()
{
    QTest::addColumn<int>("format");
    QTest::addColumn<QString>("suffix");

    QTest::newRow("mp4") << int(FileFormat::Mp4) << QStringLiteral("mp4");
    QTest::newRow("mov") << int(FileFormat::Mov) << QStringLiteral("mov");
}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress()
{
    QFETCH(int, format);
    QFETCH(QString, suffix);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString path = tempDir.filePath(QStringLiteral("in-progress.") + suffix);
    QVERIFY(stream.record(path, static_cast<FileFormat>(format)));

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

void GstVideoReceiverTest::_testSecondRecordingIsNotTruncated()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    QVERIFY(stream.record(first));
    stream.receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(second));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());

    for (const QString& path : {first, second}) {
        const QString problem = recordingProblem(path);
        QVERIFY2(problem.isEmpty(), qPrintable(problem));
    }
}

void GstVideoReceiverTest::_testRecordingSwitchWithoutDisplay_data()
{
    QTest::addColumn<int>("ending");

    QTest::newRow("stopped") << int(RecordingEnd::Stopped);
    QTest::newRow("recorder error") << int(RecordingEnd::RecorderError);
    QTest::newRow("deactivated") << int(RecordingEnd::Deactivated);
}

void GstVideoReceiverTest::_testRecordingSwitchWithoutDisplay()
{
    QFETCH(int, ending);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    // Without a display sink the first recorder's EOS ends the whole pipeline too. That EOS can still be queued when
    // the second recording starts, and must not be taken for the second recorder's.
    Receiver stream;
    QVERIFY(stream.startStreaming());
    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    QVERIFY(stream.record(first));
    // Switched without a stop: the second file starts once the first one is finalized.
    QVERIFY(stream.record(second));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), first);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());

    QElapsedTimer clock;
    clock.start();
    switch (static_cast<RecordingEnd>(ending)) {
        case RecordingEnd::Stopped:
            stream.receiver->stopRecording();
            break;
        case RecordingEnd::RecorderError:
            expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                             QRegularExpression(QStringLiteral("^Recording error: .*injected recorder failure")));
            QVERIFY(runOnPipeline(stream.receiver.get(), [](GstElement* pipeline) {
                const GStreamer::GstElementPtr fileSink = GstTest::findByFactory(GST_BIN(pipeline), "filesink");
                GError* const error =
                    g_error_new_literal(GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE, "injected recorder failure");
                (void) gst_element_post_message(fileSink.get(),
                                                gst_message_new_error(GST_OBJECT(fileSink.get()), error, nullptr));
                g_error_free(error);
            }));
            break;
        case RecordingEnd::Deactivated:
            stream.receiver->setActive(false);
            break;
    }
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 2, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The second recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), second);
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    // A file that opened reports its error through recordingFinished() only.
    QVERIFY(stream.errorSpy.isEmpty());

    if (static_cast<RecordingEnd>(ending) == RecordingEnd::RecorderError) {
        verifyExpectedLogMessage();
        QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QStringLiteral("injected recorder failure"));
        return;
    }
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(second);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testStreamEndClosesRecordingAtOnce()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream(udpListenUri(), /*autoReconnect=*/true, kShortWatchdogTimeout);
    QVERIFY(stream.startStreaming());
    const QString path = tempDir.filePath(QStringLiteral("ended.mp4"));
    QVERIFY(stream.record(path));

    // The stream's EOS reaches the recorder before the receiver acts on the end of the stream, as an RTSP/TCP source's
    // EOS can. Sent past the source, which is cut off so only the watchdog ends the session, a second or two later.
    QElapsedTimer clock;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&clock](GstElement* pipeline) {
        const GStreamer::GstElementPtr tee = GstTest::findByFactory(GST_BIN(pipeline), "tee");
        const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(tee.get(), "sink"));
        const GStreamer::GstPadPtr sourcePad = GStreamer::adoptPad(gst_pad_get_peer(teeSink.get()));
        (void) gst_pad_add_probe(
            sourcePad.get(), GST_PAD_PROBE_TYPE_DATA_DOWNSTREAM,
            [](GstPad*, GstPadProbeInfo*, gpointer) { return GST_PAD_PROBE_DROP; }, nullptr, nullptr);
        clock.start();
        (void) gst_pad_send_event(teeSink.get(), gst_event_new_eos());
    }));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());
    QVERIFY2(clock.elapsed() < kFinalizeTimeoutMs,
             qPrintable(QStringLiteral("The recording closed after %1 ms").arg(clock.elapsed())));
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 0), path);
    QCOMPARE(Receiver::lastFinished(stream.recordingFinishedSpy, 1), QString());
    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // The closed file is the first part; the recording goes on in the next one once the stream is back.
    QVERIFY(stream.receiver->recording());
    QVERIFY(stream.resumeSending());
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(stream.recordingStartedSpy.constLast().constFirst().toString(),
             tempDir.filePath(QStringLiteral("ended_part2.mp4")));
    QCOMPARE(stream.recordingFinishedSpy.count(), 1);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverTest::_testRecordingErrorKeepsStreaming()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // GStreamer adds follow-up errors (failed state change) whose number isn't ours to pin.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .*Could not open file")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("missing/unwritable.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();
    // Never opened, so never finished.
    QVERIFY(stream.recordingFinishedSpy.isEmpty());
    QCOMPARE(stream.receiver->state(), State::Streaming);

    const QString path = tempDir.filePath(QStringLiteral("recovered.mp4"));
    QVERIFY(stream.record(path));
    stream.receiver->setActive(false);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingFinishedSpy, 1, TestTimeout::mediumMs());

    const QString problem = recordingProblem(path);
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

void GstVideoReceiverTest::_testFailedRecordingStartLeavesBranchUsable()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

    // A child holding the recording sink's name makes the pipeline refuse the sink.
    GstElement* nameTaken = nullptr;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&nameTaken](GstElement* pipeline) {
        nameTaken = gst_bin_new("sinkbin");
        if (!gst_bin_add(GST_BIN(pipeline), nameTaken)) {
            nameTaken = nullptr;
        }
    }));
    QVERIFY(nameTaken);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to add the recording sink")));
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("refused.mp4")), FileFormat::Mp4);
    QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(!stream.receiver->recording(), TestTimeout::shortMs());
    verifyExpectedLogMessage();

    QVERIFY(runOnPipeline(stream.receiver.get(),
                          [nameTaken](GstElement* pipeline) { (void) gst_bin_remove(GST_BIN(pipeline), nameTaken); }));
    QVERIFY(stream.record(tempDir.filePath(QStringLiteral("accepted.mp4"))));
}

void GstVideoReceiverTest::_testRecordingStartKeepsPipelinePlaying()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;

    // Read on the worker that changes it; "PAUSED->PLAYING" while a change is pending.
    const auto pipelineState = [&stream]() {
        QString state;
        (void) runOnPipeline(stream.receiver.get(), [&state](GstElement* pipeline) {
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
    stream.receiver->startRecording(tempDir.filePath(QStringLiteral("recording.mp4")), FileFormat::Mp4);
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
    QVERIFY_SIGNAL_COUNT_WAIT(stream.recordingStartedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
}

void GstVideoReceiverTest::_testDecoderQosDisabled()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());

    // The decoders decodebin3 plugged, inspected on the worker that owns the pipeline.
    int decoders = 0;
    QStringList qosEnabled;
    QVERIFY(runOnPipeline(stream.receiver.get(), [&decoders, &qosEnabled](GstElement* pipeline) {
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

void GstVideoReceiverTest::_testScreenshotSavesDisplayedFrame()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    const QString path = tempDir.filePath(QStringLiteral("screenshot.png"));

    expectLogMessage("Video.VideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^No video output to take a screenshot from")));
    QVERIFY(!stream.receiver->takeScreenshot(path));
    verifyExpectedLogMessage();

    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    // Qt Multimedia converts on the CPU when the platform (offscreen here) has no RHI, and says so.
    ignoreLogMessage("default", QtWarningMsg, QRegularExpression(QStringLiteral("No RHI backend")));
    QVERIFY(stream.receiver->takeScreenshot(path));
    QCOMPARE(QImage(path).size(), QSize(kFrameWidth, kFrameHeight));
}

void GstVideoReceiverTest::_testDeactivateReleasesDisplayedFrame()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    stream.receiver->setActive(false);
    QVERIFY_TRUE_WAIT(!stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());

    // Releasing the frame keeps the output bound: the next stream shows again.
    QVERIFY(stream.startStreaming());
    QVERIFY_TRUE_WAIT(stream.videoSink()->videoFrame().isValid(), TestTimeout::mediumMs());
}

void GstVideoReceiverTest::_testDestroyWhileRecordingFinalizesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    Receiver stream;
    QVERIFY(stream.startStreaming());

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

void GstVideoReceiverTest::_testDestroyWithHungRecorderIsBounded()
{
    // The finalize timeout, then the receiver's margin for the pipeline to stop, then scheduling slack (more on CI).
    const qint64 kMaxDestructionMs =
        std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2}).count() +
        (2 * TestTimeout::shortMs());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto gate = std::make_shared<Gate>();
    const auto releaseGate = qScopeGuard([gate] { gate->release(); });
    GWeakRef pipelineRef;
    g_weak_ref_init(&pipelineRef, nullptr);
    const auto clearRef = qScopeGuard([&pipelineRef] { g_weak_ref_clear(&pipelineRef); });

    {
        Receiver stream;
        QVERIFY(stream.startStreaming());
        QVERIFY(stream.record(tempDir.filePath(QStringLiteral("hung.mp4"))));
        g_weak_ref_set(&pipelineRef, stream.pipeline().get());
        // Storage that never returns, as a hung network mount.
        QVERIFY(stream.blockRecorder(gate));

        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Recording not finalized within")));
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Worker still busy after")));
        QElapsedTimer teardown;
        teardown.start();
        stream.receiver.reset();
        QVERIFY2(teardown.elapsed() < kMaxDestructionMs,
                 qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));
        verifyExpectedLogMessage();
        verifyExpectedLogMessage();
    }

    // The abandoned worker finishes on its own once the storage returns.
    gate->release();
    QVERIFY(UnitTest::waitForCondition(
        [&pipelineRef] {
            GstElement* const pipeline = static_cast<GstElement*>(g_weak_ref_get(&pipelineRef));
            if (pipeline) {
                gst_object_unref(pipeline);
            }
            return pipeline == nullptr;
        },
        TestTimeout::mediumDuration(), u"abandoned pipeline released"));
}

void GstVideoReceiverTest::_testDecodesH265Stream()
{
    const QString missing = GstTest::firstMissingFactory({"rtph265pay", "rtph265depay", "h265parse", "avdec_h265"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing)));
    }
    // The stream path, not decoder choice, is under test; a hardware decoder that fails to start isn't replaced.
    const GstTest::DecoderRankSnapshot decoderRanks;
    GStreamer::setCodecPriorities(GStreamer::ForceVideoDecoderSoftware);

    Receiver stream(loopbackUri(QLatin1StringView("udp265"), 0), /*autoReconnect=*/false, kWatchdogTimeout,
                    QString::fromLatin1(GstTest::kH265Stream));
    QVERIFY2(stream.sender.error().isEmpty(), qPrintable(stream.sender.error()));
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(kFrameWidth, kFrameHeight), TestTimeout::mediumMs());
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverTest::_testResolutionChangeUpdatesVideoSize()
{
    Receiver stream;
    QVERIFY(stream.startStreaming());
    QVERIFY(stream.display());
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(kFrameWidth, kFrameHeight), TestTimeout::mediumMs());
    const qsizetype stateChanges = stream.stateSpy.count();
    const qsizetype sizeChanges = stream.sizeSpy.count();

    // A camera switching resolution mid-stream: its next keyframe carries the new parameter sets.
    QVERIFY2(stream.sender.switchTo(QString::fromLatin1(GstTest::kH264SmallStream)), qPrintable(stream.sender.error()));
    QVERIFY_TRUE_WAIT(stream.receiver->videoSize() == QSize(160, 120), TestTimeout::mediumMs());
    QVERIFY(stream.sizeSpy.count() > sizeChanges);

    // Taken in stride: no reconnect, no error.
    QCOMPARE(stream.stateSpy.count(), stateChanges);
    QCOMPARE(stream.receiver->state(), State::Streaming);
    QVERIFY(stream.errorSpy.isEmpty());
}

void GstVideoReceiverTest::_testUnresponsiveRtspServer_data()
{
    QTest::addColumn<bool>("autoReconnect");

    QTest::newRow("auto-reconnect") << true;
    QTest::newRow("no auto-reconnect") << false;
}

void GstVideoReceiverTest::_testUnresponsiveRtspServer()
{
    QFETCH(bool, autoReconnect);

    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }
    UnresponsiveRtspServer server;
    QVERIFY(server.listen());
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg, GstTest::libproxyIgnoreMaskWarning());

    // Either rtspsrc gives up on the unanswered OPTIONS after its TCP timeout or the first-frame watchdog fires
    // first; which one warns depends on the host's timing, and the receiver's reaction is what is checked.
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error from |^Video stream stopped:")));
    Receiver stream(server.uri(), autoReconnect, kShortWatchdogTimeout);
    stream.receiver->setActive(true);
    QVERIFY(stream.waitForState(State::Connecting));
    QVERIFY_TRUE_WAIT(server.connections() == 1, TestTimeout::mediumMs());

    if (autoReconnect) {
        QVERIFY_TRUE_WAIT(server.connections() >= 2, TestTimeout::longMs());
        QVERIFY(stream.errorSpy.isEmpty());
    } else {
        QVERIFY_SIGNAL_COUNT_WAIT(stream.errorSpy, 1, TestTimeout::longMs());
        QVERIFY(stream.waitForState(State::Stopped));
        QCOMPARE(server.connections(), 1);
    }
    QVERIFY(stream.streamingSpy.isEmpty());
}

#else

void GstVideoReceiverTest::_testGStreamerDisabled()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverTest, TestLabel::Integration)
