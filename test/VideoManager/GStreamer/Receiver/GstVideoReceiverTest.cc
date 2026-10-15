#include "GstVideoReceiverTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <initializer_list>
#include <memory>

#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QUdpSocket>
#include <QtQuick/QQuickItem>
#include <QtTest/QSignalSpy>

#include "GStreamer.h"
#include "GStreamerLogging.h"
#include "GstVideoReceiver.h"
#include "VideoReceiver.h"

// Receiver signals are emitted on its worker thread, so tests wait on spy counts: QSignalSpy::wait()
// only sees emissions after it starts and misses one that lands first.

namespace {

constexpr int kFrameRate = 30;
constexpr int kFrameWidth = 320;
constexpr int kFrameHeight = 240;
constexpr int kKeyframeInterval = 15;
// Frames the sender produces while a recording is open. Some may still be in flight when the
// recording ends, but a finalized file holds far more than kMinRecordedFrames; a file cut off
// before its final moov holds only the samples of the first header update (one).
constexpr int kRecordFrames = 30;
constexpr int kMinRecordedFrames = 10;
constexpr uint32_t kWatchdogTimeoutSec = 5;
constexpr int kRestartCycles = 3;
// Receiver reconnect backoff: the first retry follows a failure after 1 s, the second 2 s after that.
constexpr qint64 kFirstReconnectDelayMs = 1000;
constexpr qint64 kSecondReconnectDelayMs = 2000;
// The test thread sees worker-emitted signals late by up to its polling interval, more so on a loaded host.
constexpr qint64 kSignalLatencySlackMs = 200;

GstPadProbeReturn countBuffers([[maybe_unused]] GstPad* pad, [[maybe_unused]] GstPadProbeInfo* info, gpointer userData)
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1, std::memory_order_relaxed);
    return GST_PAD_PROBE_OK;
}

quint16 freeLoopbackUdpPort()
{
    QUdpSocket socket;
    return socket.bind(QHostAddress::LocalHost, 0) ? socket.localPort() : 0;
}

QString loopbackUri(QLatin1StringView scheme, quint16 port)
{
    return QStringLiteral("%1://127.0.0.1:%2").arg(scheme).arg(port);
}

QString firstMissingFactory(std::initializer_list<const char*> names)
{
    for (const char* name : names) {
        GstElementFactory* factory = gst_element_factory_find(name);
        if (!factory) {
            return QString::fromLatin1(name);
        }
        gst_object_unref(factory);
    }
    return {};
}

VideoReceiver::STATUS lastStatus(const QSignalSpy& spy)
{
    return spy.constLast().constFirst().value<VideoReceiver::STATUS>();
}

bool lastBool(const QSignalSpy& spy)
{
    return spy.constLast().constFirst().toBool();
}

std::unique_ptr<VideoReceiver> makeReceiver(const QString& uri, QQuickItem* widget)
{
    std::unique_ptr<VideoReceiver> receiver(GStreamer::createVideoReceiver());
    receiver->setUri(uri);
    receiver->setWidget(widget);
    // Reconnect policy isn't under test; don't let a failure restart the pipeline behind the test.
    receiver->setAutoReconnect(false);
    return receiver;
}

/// Stand-in for the qgcvideosinkbin, owned the way GStreamer::createVideoSink() hands it out.
GstElement* makeFakeVideoSink()
{
    GstElement* sink = gst_element_factory_make("fakesink", nullptr);
    return sink ? GST_ELEMENT(gst_object_ref_sink(sink)) : nullptr;
}

/// Live H.264 test pattern sent as RTP to 127.0.0.1:port.
class H264RtpSender
{
public:
    H264RtpSender() = default;

    ~H264RtpSender() { stop(); }
    Q_DISABLE_COPY_MOVE(H264RtpSender)

    bool start(quint16 port)
    {
        const QByteArray description =
            QStringLiteral(
                "videotestsrc is-live=true pattern=ball ! video/x-raw,width=%4,height=%5,framerate=%1/1 ! "
                "x264enc name=encoder tune=zerolatency speed-preset=ultrafast key-int-max=%2 ! "
                "rtph264pay config-interval=-1 pt=96 ! udpsink host=127.0.0.1 port=%3 sync=false async=false")
                .arg(kFrameRate)
                .arg(kKeyframeInterval)
                .arg(port)
                .arg(kFrameWidth)
                .arg(kFrameHeight)
                .toUtf8();
        GError* error = nullptr;
        _pipeline = gst_parse_launch(description.constData(), &error);
        if (error || !_pipeline) {
            g_clear_error(&error);
            return false;
        }

        GstElement* encoder = gst_bin_get_by_name(GST_BIN(_pipeline), "encoder");
        GstPad* encoderSrc = encoder ? gst_element_get_static_pad(encoder, "src") : nullptr;
        const bool probed =
            encoderSrc && gst_pad_add_probe(encoderSrc, GST_PAD_PROBE_TYPE_BUFFER, countBuffers, &_frames, nullptr);
        gst_clear_object(&encoderSrc);
        gst_clear_object(&encoder);

        return probed && (gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    }

    void stop()
    {
        if (_pipeline) {
            (void) gst_element_set_state(_pipeline, GST_STATE_NULL);
            gst_clear_object(&_pipeline);
        }
    }

    int frames() const { return _frames.load(std::memory_order_relaxed); }

private:
    GstElement* _pipeline = nullptr;
    std::atomic<int> _frames{0};
};

/// Demuxes the MP4/MOV at @p path to EOS. Returns the number of video samples, or -1 with
/// @p failure set when the file can't be played to the end.
int countPlayableFrames(const QString& path, QString& failure)
{
    std::atomic<int> frames{0};

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch("filesrc name=src ! qtdemux ! fakesink name=sink sync=false", &error);
    if (error || !pipeline) {
        failure = error ? QString::fromUtf8(error->message) : QStringLiteral("gst_parse_launch failed");
        g_clear_error(&error);
        gst_clear_object(&pipeline);
        return -1;
    }
    const auto pipelineCleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    GstElement* src = gst_bin_get_by_name(GST_BIN(pipeline), "src");
    const QByteArray location = QFile::encodeName(path);
    g_object_set(src, "location", location.constData(), nullptr);
    gst_object_unref(src);

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstPad* sinkPad = gst_element_get_static_pad(sink, "sink");
    (void) gst_pad_add_probe(sinkPad, GST_PAD_PROBE_TYPE_BUFFER, countBuffers, &frames, nullptr);
    gst_object_unref(sinkPad);
    gst_object_unref(sink);

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        failure = QStringLiteral("%1: demux pipeline failed to start").arg(path);
        return -1;
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* message =
        gst_bus_timed_pop_filtered(bus, static_cast<GstClockTime>(TestTimeout::mediumMs()) * GST_MSECOND,
                                   static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    gst_object_unref(bus);
    if (!message) {
        failure = QStringLiteral("%1: timed out demuxing").arg(path);
        return -1;
    }
    const bool reachedEos = (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS);
    if (!reachedEos) {
        GError* demuxError = nullptr;
        gst_message_parse_error(message, &demuxError, nullptr);
        failure = QStringLiteral("%1: %2").arg(
            path, demuxError ? QString::fromUtf8(demuxError->message) : QStringLiteral("demux error"));
        g_clear_error(&demuxError);
    }
    gst_message_unref(message);

    (void) gst_element_set_state(pipeline, GST_STATE_NULL);
    return reachedEos ? frames.load(std::memory_order_relaxed) : -1;
}

}  // namespace

void GstVideoReceiverTest::init()
{
    // Every skip comes before UnitTest::init(): skipping in init() also skips cleanup(), which UnitTest requires
    // after its init().
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
    QSKIP("GStreamer init deadlocks under AddressSanitizer (bindtextdomain lock)");
#endif

    if (!gst_is_initialized()) {
        (void) GStreamer::prepareEnvironment();
        GStreamer::redirectGLibLogging();

        GError* error = nullptr;
        if (!gst_init_check(nullptr, nullptr, &error)) {
            const QString msg = error ? QString::fromUtf8(error->message) : QStringLiteral("unknown error");
            g_clear_error(&error);
            QSKIP(qPrintable(QStringLiteral("GStreamer unavailable: %1").arg(msg)));
        }
    }

    const QString missing =
        firstMissingFactory({"videotestsrc", "x264enc", "rtph264pay", "udpsink", "udpsrc", "rtph264depay", "parsebin",
                             "decodebin3", "valve", "tee", "splitmuxsink", "mp4mux", "qtmux", "qtdemux", "fakesink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("Required GStreamer factory is unavailable: %1").arg(missing)));
    }

    UnitTest::init();
}

void GstVideoReceiverTest::_testStartDecodeStop()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    GstElement* videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { gst_object_unref(videoSink); });

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    // Decoding is requested while the first source pad may be arriving, as VideoManager does.
    receiver->start(kWatchdogTimeoutSec);
    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(decodingSpy, 1, TestTimeout::mediumMs());
    QVERIFY(lastBool(decodingSpy));

    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stopSpy), VideoReceiver::STATUS_OK);
    QVERIFY(!lastBool(decodingSpy));
}

void GstVideoReceiverTest::_testStopAfterUriClearedTearsDown()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);

    // VideoManager clears the URI before stopping when the video source is disabled.
    receiver->setUri(QString());
    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stopSpy), VideoReceiver::STATUS_OK);

    QUdpSocket rebind;
    QVERIFY2(rebind.bind(QHostAddress::LocalHost, port, QAbstractSocket::DontShareAddress),
             "Stopped receiver still holds its UDP port");
}

void GstVideoReceiverTest::_testFailedStartLeavesCleanState()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));

    QQuickItem widget;
    // udp265 exposes its source pad statically, so the receiver reports streaming before the
    // bind failure surfaces in the READY transition.
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp265"), portBlocker.localPort()), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^GStreamer error:")));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to start")));
    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_FAIL);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();

    QVERIFY(!streamingSpy.isEmpty());
    QVERIFY2(!lastBool(streamingSpy), "Failed start left the receiver streaming");

    // The failed start already reported itself; its ERROR must not queue a teardown of its own.
    // That task would run right after start() on the worker, so a short window suffices.
    QVERIFY_NO_SIGNAL_WAIT(stopSpy, 250);

    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stopSpy), VideoReceiver::STATUS_INVALID_STATE);
}

void GstVideoReceiverTest::_testVideoSinkRefcountStableAcrossRestarts()
{
    if (!firstMissingFactory({"qgcvideosinkbin"}).isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }

    auto* videoSink = static_cast<GstElement*>(GStreamer::createVideoSink(GStreamer::VideoSinkConfig{}));
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { GStreamer::releaseVideoSink(videoSink); });
    QVERIFY2(!g_object_is_floating(videoSink), "createVideoSink() must hand out an owned reference");
    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(videoSink), 1);

    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    for (int cycle = 1; cycle <= kRestartCycles; ++cycle) {
        receiver->start(kWatchdogTimeoutSec);
        receiver->startDecoding(videoSink);
        QVERIFY_SIGNAL_COUNT_WAIT(startSpy, cycle, TestTimeout::mediumMs());
        QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
        QVERIFY_SIGNAL_COUNT_WAIT(decodingSpy, (2 * cycle) - 1, TestTimeout::mediumMs());

        receiver->stop();
        QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, cycle, TestTimeout::mediumMs());
        QCOMPARE(lastStatus(stopSpy), VideoReceiver::STATUS_OK);
    }

    QCOMPARE(GST_OBJECT_REFCOUNT_VALUE(videoSink), 1);
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
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);

    const QString path = tempDir.filePath(QStringLiteral("in-progress.") + suffix);
    receiver->startRecording(path, static_cast<VideoReceiver::FILE_FORMAT>(format));
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 1, TestTimeout::mediumMs());

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
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);

    const QString path = tempDir.filePath(QStringLiteral("destroyed.mp4"));
    receiver->startRecording(path, VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 1, TestTimeout::mediumMs());
    const int recordStart = sender.frames();
    QVERIFY_TRUE_WAIT(sender.frames() >= (recordStart + kRecordFrames), TestTimeout::mediumMs());

    QElapsedTimer teardown;
    teardown.start();
    receiver.reset();
    QVERIFY2(teardown.elapsed() < TestTimeout::mediumMs(),
             qPrintable(QStringLiteral("Destructor took %1 ms").arg(teardown.elapsed())));

    QString failure;
    const int frames = countPlayableFrames(path, failure);
    QVERIFY2(frames >= kMinRecordedFrames,
             qPrintable(QStringLiteral("%1 frames recorded %2").arg(frames).arg(failure)));
}

void GstVideoReceiverTest::_testSecondRecordingIsNotTruncated()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy stopRecordingSpy(receiver.get(), &VideoReceiver::onStopRecordingComplete);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);

    const QString first = tempDir.filePath(QStringLiteral("first.mp4"));
    receiver->startRecording(first, VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 1, TestTimeout::mediumMs());
    int recordStart = sender.frames();
    QVERIFY_TRUE_WAIT(sender.frames() >= (recordStart + kRecordFrames), TestTimeout::mediumMs());
    receiver->stopRecording();
    QVERIFY_SIGNAL_COUNT_WAIT(stopRecordingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stopRecordingSpy), VideoReceiver::STATUS_OK);

    // The first recording's fragment-closed message is still queued on the pipeline bus.
    const QString second = tempDir.filePath(QStringLiteral("second.mp4"));
    receiver->startRecording(second, VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 2, TestTimeout::mediumMs());
    recordStart = sender.frames();
    QVERIFY_TRUE_WAIT(sender.frames() >= (recordStart + kRecordFrames), TestTimeout::mediumMs());
    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());

    QString failure;
    const int firstFrames = countPlayableFrames(first, failure);
    QVERIFY2(firstFrames >= kMinRecordedFrames,
             qPrintable(QStringLiteral("first: %1 frames %2").arg(firstFrames).arg(failure)));
    const int secondFrames = countPlayableFrames(second, failure);
    QVERIFY2(secondFrames >= kMinRecordedFrames,
             qPrintable(QStringLiteral("second: %1 frames %2").arg(secondFrames).arg(failure)));
}

void GstVideoReceiverTest::_testReconnectBacksOffAfterFailedStart()
{
    QUdpSocket portBlocker;
    QVERIFY(portBlocker.bind(QHostAddress::LocalHost, 0, QAbstractSocket::DontShareAddress));
    const quint16 port = portBlocker.localPort();

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp265"), port), &widget);
    receiver->setAutoReconnect(true);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);

    // The first start and its retry both fail on the bound port.
    for (int attempt = 0; attempt < 2; ++attempt) {
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^GStreamer error:")));
        expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                         QRegularExpression(QStringLiteral("^Failed to start")));
    }

    QElapsedTimer clock;
    clock.start();
    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    const qint64 firstFailure = clock.elapsed();
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_FAIL);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 2, TestTimeout::mediumMs());
    const qint64 secondFailure = clock.elapsed();
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_FAIL);
    for (int attempt = 0; attempt < 4; ++attempt) {
        verifyExpectedLogMessage();
    }

    portBlocker.close();
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 3, TestTimeout::mediumMs());
    const qint64 recovery = clock.elapsed();
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QCOMPARE(startSpy.count(), 3);

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

    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    receiver->setAutoReconnect(autoReconnect);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy timeoutSpy(receiver.get(), &VideoReceiver::timeout);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    // The shortest watchdog timeout, so losing the stream is noticed within about two seconds.
    receiver->start(1);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());

    sender.stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());
    QElapsedTimer sinceStop;
    sinceStop.start();
    QCOMPARE(lastStatus(stopSpy), VideoReceiver::STATUS_FAIL);
    QCOMPARE(timeoutSpy.count(), 1);

    if (autoReconnect) {
        QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 2, TestTimeout::mediumMs());
        QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
        QVERIFY2(sinceStop.elapsed() >= (kFirstReconnectDelayMs - kSignalLatencySlackMs),
                 qPrintable(QStringLiteral("reconnected after %1 ms").arg(sinceStop.elapsed())));
    } else {
        // Longer than the first reconnect delay.
        QVERIFY_NO_SIGNAL_WAIT(startSpy, kFirstReconnectDelayMs + 500);
    }
}

void GstVideoReceiverTest::_testDecodingRequestedBeforeStreamStarts()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);

    GstElement* videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { gst_object_unref(videoSink); });

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy startDecodingSpy(receiver.get(), &VideoReceiver::onStartDecodingComplete);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);

    receiver->start(kWatchdogTimeoutSec);
    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(startDecodingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startDecodingSpy), VideoReceiver::STATUS_OK);

    // Nothing is sent until decoding was requested, so the source pad appears only now.
    H264RtpSender sender;
    QVERIFY(sender.start(port));
    QVERIFY_SIGNAL_COUNT_WAIT(decodingSpy, 1, TestTimeout::mediumMs());
    QVERIFY(lastBool(decodingSpy));
}

void GstVideoReceiverTest::_testStopDecodingBeforeFirstFrame()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    GstElement* videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { gst_object_unref(videoSink); });

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy startDecodingSpy(receiver.get(), &VideoReceiver::onStartDecodingComplete);
    QSignalSpy stopDecodingSpy(receiver.get(), &VideoReceiver::onStopDecodingComplete);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());

    // Queued back to back, so the decoder is torn down before it can deliver a frame.
    receiver->startDecoding(videoSink);
    receiver->stopDecoding();
    QVERIFY_SIGNAL_COUNT_WAIT(stopDecodingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(stopDecodingSpy), VideoReceiver::STATUS_OK);

    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(startDecodingSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startDecodingSpy), VideoReceiver::STATUS_OK);
    QVERIFY_TRUE_WAIT(!decodingSpy.isEmpty() && lastBool(decodingSpy), TestTimeout::mediumMs());
}

void GstVideoReceiverTest::_testStopWhileStoppingRecordingFinalizesFile()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy stopRecordingSpy(receiver.get(), &VideoReceiver::onStopRecordingComplete);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);

    const QString path = tempDir.filePath(QStringLiteral("stopped.mp4"));
    receiver->startRecording(path, VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 1, TestTimeout::mediumMs());
    const int recordStart = sender.frames();
    QVERIFY_TRUE_WAIT(sender.frames() >= (recordStart + kRecordFrames), TestTimeout::mediumMs());

    // stop() lands while the muxer is still finalizing the file stopRecording() ended.
    receiver->stopRecording();
    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(stopRecordingSpy.count(), 1);
    QCOMPARE(lastStatus(stopRecordingSpy), VideoReceiver::STATUS_OK);

    QString failure;
    const int frames = countPlayableFrames(path, failure);
    QVERIFY2(frames >= kMinRecordedFrames,
             qPrintable(QStringLiteral("%1 frames recorded %2").arg(frames).arg(failure)));
}

void GstVideoReceiverTest::_testRecordingErrorKeepsStreaming()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy recordingSpy(receiver.get(), &VideoReceiver::recordingChanged);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);
    QSignalSpy stopSpy(receiver.get(), &VideoReceiver::onStopComplete);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());

    // splitmuxsink opens the file once it has a GOP, so the failure surfaces mid-recording. GStreamer adds
    // follow-up errors (failed state change, no output sink) whose number isn't ours to pin.
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error: .*Could not open file")));
    ignoreLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Recording error:")));
    receiver->startRecording(tempDir.filePath(QStringLiteral("missing/unwritable.mp4")),
                             VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(recordingSpy, 2, TestTimeout::mediumMs());
    QVERIFY(!lastBool(recordingSpy));
    verifyExpectedLogMessage();

    QCOMPARE(streamingSpy.count(), 1);
    QVERIFY(stopSpy.isEmpty());

    const QString path = tempDir.filePath(QStringLiteral("recovered.mp4"));
    receiver->startRecording(path, VideoReceiver::FILE_FORMAT_MP4);
    // The failed recording had already reported its first keyframe; the muxer only fails after it.
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 2, TestTimeout::mediumMs());
    const int recordStart = sender.frames();
    QVERIFY_TRUE_WAIT(sender.frames() >= (recordStart + kRecordFrames), TestTimeout::mediumMs());
    receiver->stop();
    QVERIFY_SIGNAL_COUNT_WAIT(stopSpy, 1, TestTimeout::mediumMs());

    QString failure;
    const int frames = countPlayableFrames(path, failure);
    QVERIFY2(frames >= kMinRecordedFrames,
             qPrintable(QStringLiteral("%1 frames recorded %2").arg(frames).arg(failure)));
}

void GstVideoReceiverTest::_testFailedRecordingStartLeavesBranchUsable()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    auto* gstReceiver = qobject_cast<GstVideoReceiver*>(receiver.get());
    QVERIFY(gstReceiver);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy startRecordingSpy(receiver.get(), &VideoReceiver::onStartRecordingComplete);
    QSignalSpy recordingSpy(receiver.get(), &VideoReceiver::recordingChanged);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());

    // A child holding the recording sink's name makes the pipeline refuse the sink.
    GstElement* nameTaken = nullptr;
    QVERIFY(QMetaObject::invokeMethod(
        gstReceiver->_workerContext,
        [gstReceiver, &nameTaken]() {
            nameTaken = gst_bin_new("sinkbin");
            if (!gst_bin_add(GST_BIN(gstReceiver->_pipeline.get()), nameTaken)) {
                nameTaken = nullptr;
            }
        },
        Qt::BlockingQueuedConnection));
    QVERIFY(nameTaken);

    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("^Failed to add the recording sink")));
    receiver->startRecording(tempDir.filePath(QStringLiteral("refused.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(startRecordingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startRecordingSpy), VideoReceiver::STATUS_FAIL);
    verifyExpectedLogMessage();
    QVERIFY(recordingSpy.isEmpty());

    QVERIFY(QMetaObject::invokeMethod(
        gstReceiver->_workerContext,
        [gstReceiver, nameTaken]() { (void) gst_bin_remove(GST_BIN(gstReceiver->_pipeline.get()), nameTaken); },
        Qt::BlockingQueuedConnection));
    receiver->startRecording(tempDir.filePath(QStringLiteral("accepted.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_COUNT_WAIT(startRecordingSpy, 2, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startRecordingSpy), VideoReceiver::STATUS_OK);
}

void GstVideoReceiverTest::_testRecordingStartKeepsPipelinePlaying()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    auto* gstReceiver = qobject_cast<GstVideoReceiver*>(receiver.get());
    QVERIFY(gstReceiver);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy recordingStartedSpy(receiver.get(), &VideoReceiver::recordingStarted);

    // Read on the worker that changes it; "PAUSED->PLAYING" while a change is pending.
    const auto pipelineState = [gstReceiver]() {
        QString state;
        (void) QMetaObject::invokeMethod(
            gstReceiver->_workerContext,
            [gstReceiver, &state]() {
                GstState current = GST_STATE_VOID_PENDING;
                GstState pending = GST_STATE_VOID_PENDING;
                (void) gst_element_get_state(gstReceiver->_pipeline.get(), &current, &pending, 0);
                state = QString::fromUtf8(gst_element_state_get_name(current));
                if (pending != GST_STATE_VOID_PENDING) {
                    state += QStringLiteral("->") + QString::fromUtf8(gst_element_state_get_name(pending));
                }
            },
            Qt::BlockingQueuedConnection);
        return state;
    };

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());
    QCOMPARE_TRUE_WAIT(pipelineState(), QStringLiteral("PLAYING"), TestTimeout::mediumMs());

    // The file sink can't preroll before the next keyframe; the live pipeline must not wait for it meanwhile.
    receiver->startRecording(tempDir.filePath(QStringLiteral("recording.mp4")), VideoReceiver::FILE_FORMAT_MP4);
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
    QVERIFY_SIGNAL_COUNT_WAIT(recordingStartedSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(pipelineState(), QStringLiteral("PLAYING"));
}

void GstVideoReceiverTest::_testDecoderQosDisabledAndReported()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    GstElement* videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { gst_object_unref(videoSink); });

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    auto* gstReceiver = qobject_cast<GstVideoReceiver*>(receiver.get());
    QVERIFY(gstReceiver);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);
    QSignalSpy decoderNameSpy(gstReceiver, &GstVideoReceiver::decoderNameChanged);

    receiver->start(kWatchdogTimeoutSec);
    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(decodingSpy, 1, TestTimeout::mediumMs());
    QVERIFY_SIGNAL_COUNT_WAIT(decoderNameSpy, 1, TestTimeout::mediumMs());
    QVERIFY(!gstReceiver->decoderName().isEmpty());

    // The decoders decodebin3 plugged, inspected on the worker that owns the pipeline.
    int decoders = 0;
    QStringList qosEnabled;
    QVERIFY(QMetaObject::invokeMethod(
        gstReceiver->_workerContext,
        [gstReceiver, &decoders, &qosEnabled]() {
            GstIterator* it = gst_bin_iterate_recurse(GST_BIN(gstReceiver->_decodingBranch.decoder()));
            GValue item = G_VALUE_INIT;
            while (gst_iterator_next(it, &item) == GST_ITERATOR_OK) {
                auto* element = GST_ELEMENT(g_value_get_object(&item));
                GstElementFactory* factory = gst_element_get_factory(element);
                if (factory && gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER) &&
                    g_object_class_find_property(G_OBJECT_GET_CLASS(element), "qos")) {
                    ++decoders;
                    gboolean qos = TRUE;
                    g_object_get(element, "qos", &qos, nullptr);
                    if (qos) {
                        qosEnabled.append(QString::fromUtf8(GST_OBJECT_NAME(element)));
                    }
                }
                g_value_reset(&item);
            }
            g_value_unset(&item);
            gst_iterator_free(it);
        },
        Qt::BlockingQueuedConnection));
    QVERIFY(decoders > 0);
    QVERIFY2(qosEnabled.isEmpty(), qPrintable(qosEnabled.join(u',')));
}

void GstVideoReceiverTest::_testVideoSizeFromNegotiatedCaps()
{
    if (!firstMissingFactory({"qgcvideosinkbin"}).isEmpty()) {
        QVERIFY2(GStreamer::completeInit(), "GStreamer::completeInit() failed");
    }
    auto* videoSink = static_cast<GstElement*>(GStreamer::createVideoSink(GStreamer::VideoSinkConfig{}));
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { GStreamer::releaseVideoSink(videoSink); });

    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    QSignalSpy sizeSpy(receiver.get(), &VideoReceiver::videoSizeChanged);

    receiver->start(kWatchdogTimeoutSec);
    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(sizeSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(sizeSpy.constLast().constFirst().toSize(), QSize(kFrameWidth, kFrameHeight));
}

void GstVideoReceiverTest::_testBusDoesNotQueueMessages()
{
    const quint16 port = freeLoopbackUdpPort();
    QVERIFY(port != 0);
    H264RtpSender sender;
    QVERIFY(sender.start(port));

    GstElement* videoSink = makeFakeVideoSink();
    QVERIFY(videoSink);
    const auto sinkCleanup = qScopeGuard([videoSink] { gst_object_unref(videoSink); });

    QQuickItem widget;
    const auto receiver = makeReceiver(loopbackUri(QLatin1StringView("udp"), port), &widget);
    auto* gstReceiver = qobject_cast<GstVideoReceiver*>(receiver.get());
    QVERIFY(gstReceiver);
    QSignalSpy startSpy(receiver.get(), &VideoReceiver::onStartComplete);
    QSignalSpy streamingSpy(receiver.get(), &VideoReceiver::streamingChanged);
    QSignalSpy decodingSpy(receiver.get(), &VideoReceiver::decodingChanged);

    receiver->start(kWatchdogTimeoutSec);
    QVERIFY_SIGNAL_COUNT_WAIT(startSpy, 1, TestTimeout::mediumMs());
    QCOMPARE(lastStatus(startSpy), VideoReceiver::STATUS_OK);
    QVERIFY_SIGNAL_COUNT_WAIT(streamingSpy, 1, TestTimeout::mediumMs());
    receiver->startDecoding(videoSink);
    QVERIFY_SIGNAL_COUNT_WAIT(decodingSpy, 1, TestTimeout::mediumMs());

    // State changes, stream collection, latency and QoS have all been posted by now.
    bool pending = true;
    QVERIFY(QMetaObject::invokeMethod(
        gstReceiver->_workerContext,
        [gstReceiver, &pending]() {
            GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(gstReceiver->_pipeline.get()));
            pending = gst_bus_have_pending(bus);
            gst_object_unref(bus);
        },
        Qt::BlockingQueuedConnection));
    QVERIFY2(!pending, "Messages are piling up on the pipeline bus");
}

#else

// Skipping in init() would skip cleanup() too, which UnitTest requires after its init().
void GstVideoReceiverTest::init()
{
    UnitTest::init();
}

void GstVideoReceiverTest::_testStartDecodeStop()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testStopAfterUriClearedTearsDown()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testFailedStartLeavesCleanState()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testVideoSinkRefcountStableAcrossRestarts()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress_data() {}

void GstVideoReceiverTest::_testRecordingPlayableWhileInProgress()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testDestroyWhileRecordingFinalizesFile()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testSecondRecordingIsNotTruncated()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testReconnectBacksOffAfterFailedStart()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testStreamLossRestartPolicy_data() {}

void GstVideoReceiverTest::_testStreamLossRestartPolicy()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testDecodingRequestedBeforeStreamStarts()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testStopDecodingBeforeFirstFrame()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testStopWhileStoppingRecordingFinalizesFile()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testRecordingErrorKeepsStreaming()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testFailedRecordingStartLeavesBranchUsable()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testRecordingStartKeepsPipelinePlaying()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testDecoderQosDisabledAndReported()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testVideoSizeFromNegotiatedCaps()
{
    QSKIP("GStreamer not enabled");
}

void GstVideoReceiverTest::_testBusDoesNotQueueMessages()
{
    QSKIP("GStreamer not enabled");
}

#endif

UT_REGISTER_TEST(GstVideoReceiverTest, TestLabel::Integration)
