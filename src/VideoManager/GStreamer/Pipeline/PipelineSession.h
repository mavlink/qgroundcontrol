#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <optional>

#include <QtCore/QObject>
#include <QtCore/QSemaphore>
#include <QtCore/QString>

#include "DecodingBranch.h"
#include "GstScoped.h"
#include "RecordingBranch.h"
#include "VideoStreamStatsMeter.h"

class PipelineDotDumper;
class QDeadlineTimer;
class StreamWatchdog;
struct VideoStreamStats;

namespace GstQgc {
struct PathStats;
}

/// \brief One receiver pipeline: source ! tee, with the display and recording branches behind the tee.
///
/// Lives on the receiver's worker thread and owns everything about its pipeline: the elements, the bus, the probes
/// and the branches. Streaming threads hand their work to the session as queued calls, so once shutdown() has stopped
/// the pipeline and detached the bus, deleting the session drops whatever they left behind.
///
/// Bus handling: a sync handler does only what can't wait (GPU context requests, stream selection, and closing the
/// valve in front of a failing branch); everything else is popped on the worker, where the bus's poll fd wakes it up.
///
/// The session recovers its branches from their errors itself: a failing decoder is replaced, a failing recorder ends
/// the recording. A stalled decoder is the listener's to notice, and to replace through restartDecoder(). Failures of
/// the stream as a whole go to the Listener, which decides whether to reconnect.
class PipelineSession : public QObject
{
    Q_OBJECT

public:
    struct Settings
    {
        QString uri;
        QString redactedUri;
        bool lowLatency = false;
        int rtpJitterLatencyMs = 0;
    };

    struct Failure
    {
        QString reason;          ///< For the log.
        QString message;         ///< For the user.
        bool permanent = false;  ///< Retrying can't help: unsupported URI, missing plugin.
    };

    /// Called on the worker. Only the failure reports (onSessionFailed(), onDecoderFailing()), which come from the
    /// event loop, may change the session; the others can come from inside a call into it.
    class Listener : public DecodingBranch::Listener, public RecordingBranch::Listener
    {
    public:
        /// The first frame reached the tee.
        virtual void onSessionStreaming() = 0;
        /// The stream failed; the session ignores further failures until it is shut down.
        virtual void onSessionFailed(const Failure& failure) = 0;

    protected:
        ~Listener() = default;
    };

    PipelineSession(const Settings& settings, Listener& listener, StreamWatchdog& watchdog,
                    const PipelineDotDumper& dotDumper, QObject* parent = nullptr);
    ~PipelineSession() override;

    /// Builds the pipeline and sets it PLAYING. False with failure() set when it can't; shut the session down then.
    bool start();

    const Failure& failure() const { return _failure; }

    void setVideoSink(GstElement* sink);
    void setRecording(const std::optional<RecordingBranch::Request>& request);
    void restartDecoder(const char* reason);

    bool displaying() const;
    GstQgc::PathStats sinkPathStats() const;

    /// Measures the stream for VideoStreamStats: its cumulative counters into @p counters, for VideoStreamStatsMeter,
    /// and what it is now into @p stats (latency, RTP jitter, codec, decoder and display path). Not const: the display
    /// path is the one most frames took since the previous call.
    void sampleStats(VideoStreamStatsMeter::Counters& counters, VideoStreamStats& stats);

    GstElement* pipeline() const { return _pipeline.get(); }

    /// Writes PipelineDotDumper::snapshot() of the pipeline under @p tag.
    void snapshot(const QString& tag) const;

    /// Drains an open recording into its file (at most RecordingBranch::kFinalizeTimeout), stops the pipeline and
    /// detaches the bus: nothing the pipeline does reaches the session afterwards. Reports the end of decoding and of
    /// the recording to the listener. Idempotent; the destructor calls it too.
    void shutdown();

private:
    void _post(std::function<void()> task);
    void _fail(const Failure& failure);

    bool _attachBus();
    void _detachBus();
    void _drainBus();
    /// Handles the recorder's messages until it is finalized or @p deadline expires.
    void _pumpBusUntilRecordingFinalized(const QDeadlineTimer& deadline);
    void _handleMessage(GstMessage* message);
    void _onBusError(GstMessage* message);
    QString _takeStartError();

    void _linkSourcePad(GstPad* pad);
    void _onFirstFrame();
    void _logLatency();

    static GstBusSyncReply _onBusSync(GstBus* bus, GstMessage* message, gpointer data);
    static void _onSourcePadAdded(GstElement* source, GstPad* pad, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);
    /// Streaming thread: measures the time between keyframes from @p buffer's timestamp.
    void _noteKeyframe(const GstBuffer* buffer);
    static GstPadProbeReturn _sourceEosProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data);

    const Settings _settings;
    Listener& _listener;
    StreamWatchdog& _watchdog;
    const PipelineDotDumper& _dotDumper;

    GStreamer::GstElementPtr _pipeline;
    // Owned by the pipeline.
    GstElement* _tee = nullptr;
    GstElement* _source = nullptr;
    // Fixed once the pipeline runs, so the sync handler may read them.
    GstElement* _displayValve = nullptr;
    GstElement* _recordingValve = nullptr;

    GStreamer::GstBusPtr _bus;
    QObject* _busNotifier = nullptr;
    bool _busHandlerInstalled = false;
    QSemaphore _busHandlerReleased;  ///< Released by GStreamer once the detached sync handler no longer runs.

    std::unique_ptr<DecodingBranch> _decoding;
    std::unique_ptr<RecordingBranch> _recording;
    GStreamer::PadProbe _teeFrameProbe;
    GStreamer::PadProbe _sourceEos;
    GStreamer::ScopedSignal _sourcePadAdded;

    Failure _failure;
    bool _failed = false;
    bool _streaming = false;
    /// Compressed bytes that reached the tee.
    std::atomic<quint64> _bytesReceived = 0;
    /// Keyframes whose timestamps the keyframe interval averages over: on a stream without a jitter buffer they are
    /// arrival times, which a busy sender or network bunches up, and an average across several keyframes evens that
    /// out.
    static constexpr std::size_t kKeyframeWindow = 5;
    /// Timestamps of the last keyframes at the tee, oldest first. Only the tee's streaming thread uses them.
    std::array<GstClockTime, kKeyframeWindow> _keyframeTimes{};
    std::size_t _keyframeTimeCount = 0;
    /// The average time between the keyframes in _keyframeTimes; GST_CLOCK_TIME_NONE before the second.
    std::atomic<GstClockTime> _keyframeInterval = GST_CLOCK_TIME_NONE;
    bool _tearingDown = false;
    bool _shutDown = false;
};
