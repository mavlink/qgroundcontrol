#pragma once

#include <atomic>
#include <glib.h>
#include <gst/gstbus.h>
#include <gst/gstelement.h>
#include <gst/gstpad.h>
#include <memory>
#include <utility>

#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtCore/QString>

#include "VideoReceiver.h"

class QThread;
class QTimer;
class GStreamerTest;
class GstVideoReceiverTest;

class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT
    Q_PROPERTY(QString decoderName       READ decoderName       NOTIFY decoderStatsChanged)
    Q_PROPERTY(quint64 processedFrames   READ processedFrames   NOTIFY decoderStatsChanged)
    Q_PROPERTY(quint64 droppedFrames     READ droppedFrames     NOTIFY decoderStatsChanged)
    Q_PROPERTY(qint64  currentJitterNs   READ currentJitterNs   NOTIFY decoderStatsChanged)
    Q_PROPERTY(double  qosProportion     READ qosProportion     NOTIFY decoderStatsChanged)
    Q_PROPERTY(int     qosQuality        READ qosQuality        NOTIFY decoderStatsChanged)

public:
    explicit GstVideoReceiver(QObject *parent = nullptr);
    ~GstVideoReceiver();

    QString decoderName()     const { QMutexLocker locker(&_decoderNameMutex); return _decoderName; }
    quint64 processedFrames() const { return _processedFrames.load(std::memory_order_relaxed); }
    quint64 droppedFrames()   const { return _droppedFrames.load(std::memory_order_relaxed); }
    qint64  currentJitterNs() const { return _currentJitterNs.load(std::memory_order_relaxed); }
    double  qosProportion()   const { return _qosProportion.load(std::memory_order_relaxed); }
    int     qosQuality()      const { return _qosQuality.load(std::memory_order_relaxed); }

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    void takeScreenshot(const QString &imageFile) override;

    /// Dump the current pipeline graph to GST_DEBUG_DUMP_DOT_DIR (if set) plus
    /// CacheLocation/qgc-pipeline-dot for field-bug-report bundles. No-op when
    /// the pipeline isn't running. Callable from QML for a debug menu.
    Q_INVOKABLE void dumpPipelineGraph(const QString &tag = QStringLiteral("manual"));

signals:
    void decoderStatsChanged();

private:
    friend class GStreamerTest;
    friend class GstVideoReceiverTest;

    /// Settings a pipeline is started with, captured on the caller's thread so the worker and
    /// streaming threads never read members the GUI thread writes.
    struct StartConfig
    {
        QString uri;
        QString redactedUri;
        uint32_t timeoutSec = 0;
        bool lowLatency = false;
        int rtpJitterLatencyMs = 0;
    };

    enum class RecordingState
    {
        Idle,
        Recording,
        Stopping,  ///< stopRecording() sent EOS into the branch; waiting for the file to be finalized
    };

    class RecordingFinalizer;

    /// Queues @p task on the worker thread, which owns the pipeline and all receiver state not marked atomic.
    template <typename Task>
    void _runOnWorker(Task&& task)
    {
        (void) QMetaObject::invokeMethod(_workerContext, std::forward<Task>(task), Qt::QueuedConnection);
    }

    /// Like _runOnWorker(), but @p task is dropped if the current pipeline is torn down before it runs.
    /// Pipeline callbacks use it so work queued by an old pipeline never acts on its successor.
    template <typename Task>
    void _runOnWorkerForPipeline(Task&& task)
    {
        const quint64 generation = _pipelineGeneration.load(std::memory_order_relaxed);
        _runOnWorker([this, generation, task = std::forward<Task>(task)]() mutable {
            if (generation == _pipelineGeneration.load(std::memory_order_relaxed)) {
                task();
            }
        });
    }

    bool _onWorkerThread() const;

    QString _redactedUri() const;
    GstElement *_makeDecoder();
    static GstElement* _makeFileSink(const QString& videoFile, FILE_FORMAT format, const GstCaps* inputCaps);

    void _startPipeline(const StartConfig& config);
    void _startDecoding(GstElement* videoSink, bool hasWidget);
    void _onNewSourcePad(GstPad *pad);
    void _onNewDecoderPad(GstPad *pad);
    bool _addDecoder(GstElement *src);
    void _ensureVideoSinkInPipeline();
    bool _addVideoSink(GstPad *pad);
    void _onPadAdded(GstElement* element, GstPad* pad);
    void _noteTeeFrame();
    void _noteVideoSinkFrame();
    void _onFirstVideoFrame();
    /// -Unlink the branch from the src pad
    /// -Send an EOS event at the beginning of that branch
    bool _unlinkBranch(GstElement *from);
    void _shutdownDecodingBranch();
    void _shutdownRecordingBranch();
    void _removeSourceEosProbe();
    /// Tears down the running pipeline (finalizing an active recording first) and resets all
    /// pipeline state. Returns false when nothing was running.
    bool _stopPipeline();
    /// Releases every pipeline-scoped element, probe and flag, emitting the matching change
    /// signals. Safe on a partially built pipeline (failed start()).
    void _resetPipelineState();
    void _logDecodebin3SelectedCodec(GstElement *decodebin3);
    void _onWatchdogTick();
    void _attachBusHandler();
    /// Returns once no thread is inside the bus handler any more, so no bus callback can reach `this`.
    void _detachBusHandler();
    void _onPipelineError();
    void _onPipelinePlaying();
    void _onSourceEndOfStream();
    void _onRecordingFinalized();
    void _onRecorderError();
    void _onRecordingStopTimeout();

    /// Stop the pipeline and queue a delayed restart with exponential backoff.
    /// `reason` is logged so reconnect storms are diagnosable. No-op when
    /// autoReconnect() is disabled.
    void _scheduleReconnect(const char *reason);
    void _reconnect();

    /// The pipeline's only bus consumer. Runs on whichever thread posts: lets the HwBuffers context bridges answer
    /// NEED_CONTEXT, does the cheap per-type work inline, queues the rest on the worker, and drops every message.
    static GstBusSyncReply _onBusSyncMessage(GstBus* bus, GstMessage* message, gpointer user_data);
    void _onBusError(GstMessage* message);
    void _onBusElement(GstMessage* message);
    void _onBusQos(GstMessage* message);
    void _onBusStateChanged(GstMessage* message);
    static void _onNewPad(GstElement *element, GstPad *pad, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

    QThread* _workerThread = nullptr;
    /// Lives on _workerThread; tasks and the timers below run there. Deleted when the thread finishes.
    QObject* _workerContext = nullptr;
    QTimer* _watchdogTimer = nullptr;
    QTimer* _reconnectTimer = nullptr;
    /// Bounds stopRecording(): completes it when the file isn't reported finalized in time.
    QTimer* _recordingStopTimer = nullptr;

    StartConfig _config;
    GstElement *_decoder = nullptr;
    GstElement *_decoderValve = nullptr;
    GstElement *_fileSink = nullptr;
    GstElement *_pipeline = nullptr;
    GstElement *_recorderValve = nullptr;
    GstElement *_source = nullptr;
    GstElement *_tee = nullptr;
    GstElement *_videoSink = nullptr;
    int _reconnectAttempts = 0;
    /// Identifies the current pipeline; bumped on every teardown (stop() or failed start()) once its callbacks have
    /// stopped. Written on the worker, read by callbacks on streaming threads.
    std::atomic<quint64> _pipelineGeneration = 0;
    bool _busHandlerAttached = false;
    QSemaphore _busHandlerReleased;  ///< Released by GStreamer once the detached bus handler is no longer running.
    std::atomic<quint64> _sourceFrameCount =
        0;  ///< Tee-probe frame tally (streaming thread); drives the source-side flow heartbeat log.
    gulong _teeProbeId = 0;
    gulong _videoSinkProbeId = 0;
    gulong _eosProbeId = 0;
    // Ref-held source pad carrying _eosProbeId; released with the pipeline, not the decode branch.
    GstPad* _eosProbePad = nullptr;
    std::atomic<gulong> _keyframeWatchId = 0;  // cleared by _keyframeWatch (streaming thread) when it removes itself
    RecordingState _recordingState = RecordingState::Idle;
    /// Signalled from the bus handler; the pointer is fixed for the receiver's lifetime.
    const std::unique_ptr<RecordingFinalizer> _recordingFinalizer;

    int _statsTickCounter = 0;
    bool _streaming = false;
    std::atomic<bool> _decoding = false;  ///< Read by the tee probe's flow log.
    // Monotonic ms. Written on streaming threads (pad probes), read/written by the watchdog on the worker thread.
    std::atomic<qint64> _lastSourceFrameTime = 0;
    std::atomic<qint64> _lastVideoFrameTime = 0;

    mutable QMutex _decoderNameMutex;  // QString refcount isn't thread-safe across reader/writer threads
    QString _decoderName;
    std::atomic<quint64> _processedFrames{0};   // written on streaming thread (QOS), read on GUI
    std::atomic<quint64> _droppedFrames{0};
    std::atomic<qint64>  _currentJitterNs{0};
    std::atomic<double>  _qosProportion{1.0};
    std::atomic<int>     _qosQuality{1000000};
    std::atomic<bool> _qosStatsDirty{false};  // set per QOS message, drained by the 1 Hz watchdog tick

    static constexpr const char *_kFileMux[FILE_FORMAT_MAX + 1] = {
        "matroskamux",
        "qtmux",
        "mp4mux"
    };
};
