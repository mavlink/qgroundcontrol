#pragma once

#include <atomic>
#include <glib.h>
#include <gst/gstelement.h>
#include <gst/gstpad.h>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

#include "BusMessageRouter.h"
#include "DecodingBranch.h"
#include "GstScoped.h"
#include "PipelineDotDumper.h"
#include "RecordingBranch.h"
#include "StreamWatchdog.h"
#include "VideoReceiver.h"

class QThread;
class QTimer;
class GStreamerTest;
class GstVideoReceiverTest;

/// Shared by GstVideoReceiver and its components.
Q_DECLARE_LOGGING_CATEGORY(GstVideoReceiverLog)

class GstVideoReceiver : public VideoReceiver
{
    Q_OBJECT
    Q_PROPERTY(QString decoderName READ decoderName NOTIFY decoderNameChanged)

public:
    explicit GstVideoReceiver(QObject *parent = nullptr);
    ~GstVideoReceiver();

    /// Factory name of the decoder decodebin3 plugged last, empty before the first one.
    QString decoderName() const { return _decodingBranch.decoderName(); }

public slots:
    void start(uint32_t timeout) override;
    void stop() override;
    void startDecoding(void *sink) override;
    void stopDecoding() override;
    void startRecording(const QString &videoFile, FILE_FORMAT format) override;
    void stopRecording() override;
    /// Saves the frame on display in the widget's video sink to @p imageFile. Runs on the caller's thread, which must
    /// be the widget's (the GUI thread).
    void takeScreenshot(const QString &imageFile) override;

    /// Dumps the running pipeline's graph, without element properties, to GST_DEBUG_DUMP_DOT_DIR when that is set,
    /// else to CacheLocation/qgc-pipeline-dot for bug reports. @p tag becomes part of the file name; anything but
    /// letters, digits, '-' and '_' is replaced. Callable from QML for a debug menu.
    Q_INVOKABLE void dumpPipelineGraph(const QString &tag = QStringLiteral("manual"));

signals:
    /// Emitted on the receiver's own thread.
    void decoderNameChanged();

private:
    friend class GStreamerTest;
    friend class GstVideoReceiverTest;
    friend class BusMessageRouter;
    friend class DecodingBranch;
    friend class RecordingBranch;

    /// Settings a pipeline is started with, captured on the caller's thread so the worker and
    /// streaming threads never read members the GUI thread writes.
    struct StartConfig
    {
        QString name;
        QString uri;
        QString redactedUri;
        uint32_t timeoutSec = 0;
        bool lowLatency = false;
        int rtpJitterLatencyMs = 0;
    };

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

    void _startPipeline(const StartConfig& config);
    void _onNewSourcePad(GstPad *pad);
    void _onPadAdded(GstElement* element, GstPad* pad);
    void _noteTeeFrame();
    void _removeSourceEosProbe();
    /// Tears down the running pipeline (finalizing an active recording first) and resets all
    /// pipeline state. Returns false when nothing was running.
    bool _stopPipeline();
    /// Releases every pipeline-scoped element, probe and flag, emitting the matching change
    /// signals. Safe on a partially built pipeline (failed start()).
    void _resetPipelineState();
    void _onWatchdogTick();
    void _onPipelineError();
    void _onPipelinePlaying();

    /// Tears down a pipeline that failed (error, watchdog, end of stream), reports it with
    /// onStopComplete(STATUS_FAIL) and schedules a reconnect.
    void _onStreamFailure(const char* reason);
    /// Queues a restart with exponential backoff; no-op when autoReconnect() is off. The receiver is the
    /// only one that restarts after a failure. `reason` is logged so reconnect storms are diagnosable.
    void _scheduleReconnect(const char *reason);
    void _reconnect();

    static void _onNewPad(GstElement *element, GstPad *pad, gpointer data);
    static GstPadProbeReturn _teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);
    static GstPadProbeReturn _eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data);

    QThread* _workerThread = nullptr;
    /// Lives on _workerThread; tasks and the timers below run there. Deleted when the thread finishes.
    QObject* _workerContext = nullptr;
    QTimer* _watchdogTimer = nullptr;
    QTimer* _reconnectTimer = nullptr;

    StartConfig _config;
    GStreamer::GstElementPtr _pipeline;
    // Both owned by _pipeline.
    GstElement *_source = nullptr;
    GstElement *_tee = nullptr;
    /// Identifies the current pipeline; bumped on every teardown (stop() or failed start()) once its callbacks have
    /// stopped. Written on the worker, read by callbacks on streaming threads.
    std::atomic<quint64> _pipelineGeneration = 0;
    gulong _teeProbeId = 0;
    gulong _eosProbeId = 0;
    /// Source pad carrying _eosProbeId; released with the pipeline, not the decode branch.
    GStreamer::GstPadPtr _eosProbePad;
    RecordingBranch _recordingBranch;
    DecodingBranch _decodingBranch;
    BusMessageRouter _busRouter;

    int _statsTickCounter = 0;
    bool _streaming = false;
    StreamWatchdog _watchdog;
    PipelineDotDumper _dotDumper;
};
