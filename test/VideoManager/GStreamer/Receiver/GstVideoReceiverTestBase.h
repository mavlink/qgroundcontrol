#pragma once

#include <functional>

#include "GStreamerTestBase.h"

#ifdef QGC_GST_STREAMING

#include <chrono>
#include <condition_variable>
#include <gst/gst.h>
#include <memory>
#include <mutex>

#include <QtCore/QString>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#include <QtQuick/QQuickWindow>
#include <QtTest/QSignalSpy>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstVideoReceiver.h"
#include "RecordingBranch.h"
#include "VideoReceiver.h"

#endif

/// Base of the tests that drive GstVideoReceiver end to end against in-process H.264 and H.265 streams on loopback.
class GstVideoReceiverTestBase : public GStreamerTestBase
{
    Q_OBJECT

public:
#ifdef QGC_GST_STREAMING
    /// Runs @p task with @p receiver's pipeline (null when none runs) on the worker thread that owns it, and waits.
    static bool runOnPipeline(GstVideoReceiver* receiver, const std::function<void(GstElement* pipeline)>& task);
    static GstElement* displaySink(const GstVideoReceiver* receiver);
#endif

protected slots:
#ifdef QGC_GST_STREAMING
    void initTestCase() override;
#endif
};

#ifdef QGC_GST_STREAMING

namespace GstReceiverTest {

using State = VideoReceiver::State;
using FileFormat = VideoReceiver::FileFormat;

// The stream GstTest::FixtureRtpSender sends: 320x240 at 30 fps, a keyframe every 5 frames.
inline constexpr int kFrameRate = GstTest::kStreamFrameRate;
inline constexpr int kFrameWidth = 320;
inline constexpr int kFrameHeight = 240;
// Frames the sender produces while a recording is open. Some may still be in flight when the
// recording ends, but a finalized file holds far more than kMinRecordedFrames; a file cut off
// before its final moov holds only the samples of the first header update (one).
inline constexpr int kRecordFrames = 15;
inline constexpr int kMinRecordedFrames = 5;
inline constexpr std::chrono::seconds kWatchdogTimeout{5};
// The shortest watchdog timeout, so losing the stream is noticed within about two seconds.
inline constexpr std::chrono::seconds kShortWatchdogTimeout{1};
// The first reconnect follows a failure after 1 s.
inline constexpr qint64 kFirstReconnectDelayMs = 1000;
// GUI-thread delivery of the worker's reports lags by up to an event-loop pass, more so on a loaded host.
inline constexpr qint64 kSignalLatencySlackMs = 200;
// A window that shows a reconnect would have started: the first one follows a failure after kFirstReconnectDelayMs.
inline constexpr int kNoReconnectWindowMs = static_cast<int>(kFirstReconnectDelayMs + kSignalLatencySlackMs);
// A recording that waits this long to close was closed by the finalize timeout, not by its recorder's EOS.
inline constexpr qint64 kFinalizeTimeoutMs = std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout).count();

inline QString loopbackUri(QLatin1StringView scheme, quint16 port)
{
    return QStringLiteral("%1://127.0.0.1:%2").arg(scheme).arg(port);
}

inline QString udpListenUri()
{
    return loopbackUri(QLatin1StringView("udp"), 0);
}

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
        // The CPU display path needs no GPU on the test host.
        receiver->setDisplayConfig({.conversionElement = {}, .disablePixelAspectRatio = false, .gpuZeroCopy = false});
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
        (void) GstVideoReceiverTestBase::runOnPipeline(receiver.get(), [&port](GstElement* pipeline) {
            if (const GStreamer::GstElementPtr source = GstTest::findByFactory(pipeline, "udpsrc")) {
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
        (void) GstVideoReceiverTestBase::runOnPipeline(receiver.get(), [&current](GstElement* pipeline) {
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
        (void) GstVideoReceiverTestBase::runOnPipeline(receiver.get(), [&blocked, gate](GstElement* pipeline) {
            const GStreamer::GstElementPtr fileSink = GstTest::findByFactory(pipeline, "filesink");
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

}  // namespace GstReceiverTest

#endif
