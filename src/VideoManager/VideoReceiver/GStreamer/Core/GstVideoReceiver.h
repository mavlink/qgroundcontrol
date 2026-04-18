#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtMultimedia/QMediaFormat>
#include <atomic>
#include <functional>
#include <glib.h>
#include <gst/gstelement.h>
#include <gst/gstpad.h>
#include <memory>

#include "GstDecoderFallbackController.h"
#include "GstDecodingBranch.h"
#include "GstPipelineController.h"
#include "GstRecordingBranch.h"
#include "VideoReceiver.h"

class StreamHealthMonitor;

Q_DECLARE_LOGGING_CATEGORY(GstVideoReceiverLog)

typedef struct _GstElement GstElement;

class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT

public:
    explicit GstVideoReceiver(QObject* parent = nullptr);
    ~GstVideoReceiver();

    [[nodiscard]] Capabilities capabilities() const override;

    [[nodiscard]] BackendKind kind() const override { return BackendKind::GStreamer; }

    [[nodiscard]] bool isStreaming() const override { return _streaming.load(std::memory_order_relaxed); }
    [[nodiscard]] bool isDecoding() const override { return _decoderActive.load(std::memory_order_relaxed); }

    [[nodiscard]] VideoRecorder* createRecorder(VideoFrameDelivery* delivery, QObject* parent) override;

    /// Pause/resume the GStreamer pipeline (reconnect cycles).
    void pause() override;
    void resume() override;

    /// Apply low-latency mode to a running pipeline (rtspsrc + rtpjitterbuffer).
    /// Stores the value for the next pipeline build if not currently streaming.
    void setLowLatency(bool lowLatency) override;

signals:
    /// Emitted on the worker thread when a recording branch EOS is drained.
    void recordingBranchEosDrained(bool success);

    /// Emitted on the worker thread once the first keyframe reaches the recording branch.
    void recordingBranchStarted(const QString& path);

    /// Emitted on the worker thread just before the pipeline is set to NULL.
    void pipelineStopping();

    /// Emitted when a QVideoSink is attached AFTER the pipeline is already
    /// streaming. VideoStream should restart the pipeline so rtspsrc re-SETUPs
    /// and the decoder starts from a fresh IDR rather than waiting tens of
    /// seconds for the next keyframe in the camera's GOP.
    void sinkAttachedToLivePipeline();

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding() override;
    void stopDecoding() override;

    void startRecordingBranch(const QString& path, QMediaFormat::FileFormat format);
    void stopRecordingBranch();

protected:
    void onSinkAboutToChange() override;
    void onSinkChanged(QVideoSink* newSink) override;

private slots:
    void _handleEOS();
    /// Called on the worker thread after construction to set up the health monitor.
    void _initTimers();
    /// Bridge the monitor's sourceTimeout/decoderTimeout signals to the public
    /// timeout() signal that VideoStream consumes.
    void _onHealthTimeout(qint64 elapsedSec);

private:
    void _onNewSourcePad(GstPad* pad);
    void _onNewDecoderPad(GstPad* pad);
    void _noteTeeFrame();
    void _noteVideoSinkFrame();
    bool _unlinkBranch(GstElement* from);

    void _flushPendingDecoding();
    /// Returns true if the caller is NOT on the worker thread.
    bool _needDispatch() const;

    /// Re-post `fn` to the worker thread if the caller isn't there already.
    /// Returns true when the hop was dispatched (caller should return
    /// immediately); false when execution may continue inline. Collapses the
    /// recurring `if (_needDispatch()) { _dispatch(...); return; }` prologue
    /// into a single line at each entry point.
    template <typename Fn>
    [[nodiscard]] bool _reinvokeIfOffThread(Fn&& fn)
    {
        if (!_needDispatch())
            return false;
        _dispatch(std::forward<Fn>(fn));
        return true;
    }

    /// Set `_decoderActive` and emit `decodingChanged` on change. Callers are
    /// already on the worker thread via `_reinvokeIfOffThread` / `_dispatch`.
    void _setDecoderActive(bool active);

    /// Queue a lambda onto the worker thread. Replaces the old
    /// GstVideoWorker::dispatch pattern after moveToThread migration.
    template <typename Fn>
    void _dispatch(Fn&& fn)
    {
        QMetaObject::invokeMethod(this, std::forward<Fn>(fn), Qt::QueuedConnection);
    }

    void _completeDecoderStop(STATUS status);

    static gboolean _onBusMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    static void _onNewPad(GstElement* element, GstPad* pad, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

    bool _tryDecoderFallback(GstMessage* msg);
    bool _activateDecodingBranch();
    void _clearPipelineElements();
    static GstVideoReceiver* self(gpointer data) { return static_cast<GstVideoReceiver*>(data); }

    // --- Worker thread (replaces GstVideoWorker) ---
    QThread* _thread = nullptr;  ///< Owned; GstVideoReceiver lives on this thread.

    // --- State ---
    /// Tracks whether source frames are flowing through the tee. Flipped to
    /// true when the first source pad links in _onNewSourcePad(), and back to
    /// false when stop() begins tearing down a live pipeline. Sole source of
    /// truth for isStreaming(). Pipeline-alive state is conveyed by
    /// GstPipelineController::hasPipeline().
    /// Atomic so isStreaming() may be read from any thread without a race.
    std::atomic<bool> _streaming{false};
    /// Tracks decoder-branch activation: flipped whenever the pipeline wires
    /// (or tears down) the decoder after a sink has been attached. Emits
    /// `decodingChanged` on change via `_setDecoderActive`.
    /// Atomic so isDecoding() may be read from any thread without a race.
    std::atomic<bool> _decoderActive{false};
    std::atomic<bool> _endOfStream{false};
    int _buffer = 0;
    uint32_t _timeout = 0;

    /// Watchdog + jitter-buffer tuner. Lives on `_thread`; created in _initTimers.
    StreamHealthMonitor* _healthMonitor = nullptr;

    // --- Controllers ---
    GstDecoderFallbackController _fallbackController;

    // --- Pipeline ---
    GstPipelineController _pipelineController;

    /// Sentinel guarding GLib sync-message callbacks against use-after-free.
    /// Held by shared_ptr so queued lambdas keep the flag alive after *this is
    /// destroyed. Set to true in the destructor before _thread->wait() so any
    /// lambda that fires in the drain window sees the sentinel and skips.
    std::shared_ptr<std::atomic<bool>> _destroyed{std::make_shared<std::atomic<bool>>(false)};

    // --- Branch managers ---
    GstDecodingBranch _decodingBranch;
    GstRecordingBranch _recordingBranch;

};
