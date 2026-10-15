//-----------------------------------------------------------------------------
// Our pipeline look like this:
//
//              +-->queue-->_decoderValve[-->_decoder-->_videoSink]
//              |
// _source-->_tee
//              |
//              +-->queue-->_recorderValve[-->_fileSink]
//-----------------------------------------------------------------------------

#include "GstVideoReceiver.h"

#include <algorithm>
#include <chrono>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QWaitCondition>
#include <QtQuick/QQuickItem>

#include "GStreamerHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "QGCQVideoSinkController.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {
// Bus wait budget for an active recording to finalize during stop(); covers slow storage.
constexpr std::chrono::seconds kEosTimeout{3};
// Destructor wait for the worker. Must exceed kEosTimeout so a finalizing recording isn't abandoned.
constexpr std::chrono::seconds kWorkerShutdownTimeout = kEosTimeout + std::chrono::seconds{2};
constexpr qint64 kMinWatchdogTimeoutMs = 1000;
constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// Reconnect backoff: 1 s, 2 s, 4 s, ... capped at 30 s. The cap bounds worst-case recovery after a long
// RF outage and stays below a typical RTSP server keepalive (60 s).
constexpr int kMaxReconnectBackoffSec = 30;

// MP4/MOV robust muxing: qtmux reserves header space at the start of the file and rewrites the moov
// there every update period, so a crash or power loss leaves a file playable up to the last update.
// The reservation (550 B/s per track by default, ~8 MB here) outlasts the nominal duration since the
// index grows ~140 B/s at 30 fps and ~260 B/s at 60 fps; overrunning it makes qtmux post an ERROR.
// The period bounds crash data loss without rewriting the growing moov too often.
constexpr guint64 kRecordingReservedMaxDurationNs = 2 * 3600 * GST_SECOND;
constexpr guint64 kRecordingMoovUpdatePeriodNs = 5 * GST_SECOND;

qint64 monotonicMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

/// Per-recording state for the keyframe gate, owned by its probe.
struct KeyframeWatch
{
    GstVideoReceiver* receiver = nullptr;
    QString file;
};

void logBusWarning(GstMessage* message)
{
    // GStreamer posts WARNING for caps mismatches, decoder fallbacks, clock drift — surfacing keeps these
    // visible without escalating to STATUS_FAIL.
    gchar* debug = nullptr;
    GError* error = nullptr;
    gst_message_parse_warning(message, &error, &debug);
    qCWarning(GstVideoReceiverLog) << "GStreamer warning:" << (error ? error->message : "(no message)")
                                   << "debug:" << (debug ? debug : "(none)");
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);
}

/// Selecting only video keeps decodebin3 from instantiating audio decoder branches. Done synchronously so the
/// selection is in place before decodebin3 falls back to its default one.
void selectVideoStreams(GstMessage* message)
{
    GstStreamCollection* collection = nullptr;
    gst_message_parse_stream_collection(message, &collection);
    if (!collection) {
        return;
    }
    GList* selectedIds = nullptr;
    const guint nStreams = gst_stream_collection_get_size(collection);
    for (guint i = 0; i < nStreams; ++i) {
        GstStream* stream = gst_stream_collection_get_stream(collection, i);
        if (gst_stream_get_stream_type(stream) & GST_STREAM_TYPE_VIDEO) {
            selectedIds = g_list_append(selectedIds, g_strdup(gst_stream_get_stream_id(stream)));
        }
    }
    if (selectedIds) {
        (void) gst_element_send_event(GST_ELEMENT(GST_MESSAGE_SRC(message)), gst_event_new_select_streams(selectedIds));
        g_list_free_full(selectedIds, g_free);
    }
    gst_object_unref(collection);
}

using GstElementRef = std::unique_ptr<GstElement, GStreamer::GstObjectDeleter>;
using GstPadRef = std::unique_ptr<GstPad, GStreamer::GstObjectDeleter>;

// Blocking probe: returning OK keeps the pad blocked until the probe is removed.
GstPadProbeReturn holdPad(GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer /*userData*/)
{
    return GST_PAD_PROBE_OK;
}

// Refs the element's first src pad into *userData and stops iterating. Resync is handled
// internally by gst_element_foreach_src_pad (unlike a bare gst_iterator_next loop).
gboolean grabFirstSrcPad(GstElement * /*element*/, GstPad *pad, gpointer userData)
{
    *static_cast<GstPad **>(userData) = GST_PAD(gst_object_ref(pad));
    return FALSE;
}

bool isRecoverableH265PaciError(GstMessage *msg, const GError *error, const gchar *debug)
{
    if (!msg || !error || (error->domain != GST_STREAM_ERROR) || (error->code != GST_STREAM_ERROR_FORMAT) ||
        !debug || !g_strrstr(debug, "NAL unit type 50 not supported yet")) {
        return false;
    }

    GstObject *src = GST_MESSAGE_SRC(msg);
    if (!src || !GST_IS_ELEMENT(src)) {
        return false;
    }

    GstElementFactory *factory = gst_element_get_factory(GST_ELEMENT(src));
    return factory && (g_strcmp0(GST_OBJECT_NAME(factory), "rtph265depay") == 0);
}

const char* recordingParserFactory(const GstCaps* caps)
{
    if (!caps || gst_caps_is_any(caps) || gst_caps_is_empty(caps)) {
        return nullptr;
    }

    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstStructure* structure = gst_caps_get_structure(caps, i);
        if (gst_structure_has_name(structure, "video/x-h264")) {
            return "h264parse";
        }
        if (gst_structure_has_name(structure, "video/x-h265")) {
            return "h265parse";
        }
    }

    return nullptr;
}

}  // namespace

/// Finalization of the active recording. Settled from the bus handler on whichever thread posts, by the first of:
/// splitmuxsink-fragment-closed or a branch/pipeline EOS (closed), or a recorder ERROR (failed). Armed with the
/// recording's file sink so messages about an earlier file can never settle the current one.
class GstVideoReceiver::RecordingFinalizer
{
public:
    enum class Outcome
    {
        Pending,
        Closed,
        Failed,
    };

    ~RecordingFinalizer() { (void) disarm(); }

    void arm(GstElement* fileSink)
    {
        QMutexLocker lock(&_mutex);
        gst_clear_object(&_fileSink);
        _fileSink = GST_ELEMENT(gst_object_ref(fileSink));
        _outcome = Outcome::Pending;
    }

    /// Returns how the recording ended.
    Outcome disarm()
    {
        QMutexLocker lock(&_mutex);
        gst_clear_object(&_fileSink);
        return std::exchange(_outcome, Outcome::Pending);
    }

    Outcome outcome() const
    {
        QMutexLocker lock(&_mutex);
        return _outcome;
    }

    /// Settles the armed recording with @p outcome unless it already ended. Returns whether this call settled it.
    bool settle(Outcome outcome)
    {
        QMutexLocker lock(&_mutex);
        return _settleLocked(outcome);
    }

    /// As settle(), but only when @p source is (inside) the armed file sink. @p belongs reports that membership.
    bool settleFrom(GstObject* source, Outcome outcome, bool* belongs = nullptr)
    {
        QMutexLocker lock(&_mutex);
        const bool fromRecording =
            _fileSink && source && gst_object_has_as_ancestor(source, GST_OBJECT_CAST(_fileSink));
        if (belongs) {
            *belongs = fromRecording;
        }
        return fromRecording && _settleLocked(outcome);
    }

    /// Blocks until the recording ends or @p deadline expires; returns Pending on timeout.
    Outcome wait(QDeadlineTimer deadline)
    {
        QMutexLocker lock(&_mutex);
        while (_outcome == Outcome::Pending) {
            if (!_settled.wait(&_mutex, deadline)) {
                break;
            }
        }
        return _outcome;
    }

private:
    bool _settleLocked(Outcome outcome)
    {
        if (!_fileSink || (_outcome != Outcome::Pending)) {
            return false;
        }
        _outcome = outcome;
        _settled.wakeAll();
        return true;
    }

    mutable QMutex _mutex;
    QWaitCondition _settled;
    GstElement* _fileSink = nullptr;
    Outcome _outcome = Outcome::Pending;
};

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _workerContext(new QObject())
    , _watchdogTimer(new QTimer(_workerContext))
    , _reconnectTimer(new QTimer(_workerContext))
    , _recordingStopTimer(new QTimer(_workerContext))
    , _recordingFinalizer(std::make_unique<RecordingFinalizer>())
{
    qCDebug(GstVideoReceiverLog) << this;

    _workerThread->setObjectName(QStringLiteral("GstVideoReceiver"));
    _watchdogTimer->setInterval(kWatchdogInterval);
    _reconnectTimer->setSingleShot(true);
    // Backoff delays are asserted by tests; a coarse timer may fire up to 5% early.
    _reconnectTimer->setTimerType(Qt::PreciseTimer);
    (void) connect(_watchdogTimer, &QTimer::timeout, _workerContext, [this]() { _onWatchdogTick(); });
    (void) connect(_reconnectTimer, &QTimer::timeout, _workerContext, [this]() { _reconnect(); });
    _recordingStopTimer->setSingleShot(true);
    _recordingStopTimer->setInterval(kEosTimeout);
    (void) connect(_recordingStopTimer, &QTimer::timeout, _workerContext, [this]() { _onRecordingStopTimeout(); });

    _workerContext->moveToThread(_workerThread);
    (void) connect(_workerThread, &QThread::finished, _workerContext, &QObject::deleteLater);
    _workerThread->start();
}

GstVideoReceiver::~GstVideoReceiver()
{
    // Tear down synchronously: queued tasks and pipeline callbacks capture `this`, so the worker
    // must be idle and the pipeline gone before members are destroyed. No onStopComplete here;
    // listeners must not be handed a receiver that is being destroyed.
    _runOnWorker([this]() {
        _reconnectTimer->stop();
        (void) _stopPipeline();
        _workerThread->quit();
    });

    if (!_workerThread->wait(QDeadlineTimer(kWorkerShutdownTimeout))) {
        // Destroying a running QThread aborts the process, and queued tasks still reference this receiver.
        qCWarning(GstVideoReceiverLog) << "Worker still busy after"
                                       << std::chrono::milliseconds(kWorkerShutdownTimeout).count()
                                       << "ms; waiting for it to finish";
        (void) _workerThread->wait();
    }

    qCDebug(GstVideoReceiverLog) << this;
}

bool GstVideoReceiver::_onWorkerThread() const
{
    return QThread::currentThread() == _workerThread;
}

QString GstVideoReceiver::_redactedUri() const
{
    return _config.redactedUri;
}

void GstVideoReceiver::start(uint32_t timeout)
{
    const StartConfig config{
        .uri = _uri,
        .redactedUri = QGCNetworkHelper::redactedUrlForLogging(_uri),
        .timeoutSec = timeout,
        .lowLatency = _lowLatency,
        .rtpJitterLatencyMs = _rtpJitterLatencyMs,
    };
    _runOnWorker([this, config]() {
        // A user start supersedes any pending reconnect and restarts the backoff.
        _reconnectTimer->stop();
        _reconnectAttempts = 0;
        _startPipeline(config);
    });
}

void GstVideoReceiver::_startPipeline(const StartConfig& config)
{
    if (_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Already running!" << _redactedUri();
        emit onStartComplete(STATUS_INVALID_STATE);
        return;
    }

    if (config.uri.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Failed because URI is not specified";
        emit onStartComplete(STATUS_INVALID_URL);
        return;
    }

    _config = config;
    _sourceFrameCount.store(0, std::memory_order_relaxed);

    qCDebug(GstVideoReceiverLog) << "Starting" << _redactedUri() << ", lowLatency" << _config.lowLatency << ", timeout"
                                 << _config.timeoutSec;

    // GST_DEBUG_BIN_TO_DOT_FILE is a no-op unless GST_DEBUG_DUMP_DOT_DIR is set; surface that
    // once per process so field debugging doesn't require re-reading the source.
    [[maybe_unused]] static const bool dotDirHinted = []() {
        if (qgetenv("GST_DEBUG_DUMP_DOT_DIR").isEmpty()) {
            qCInfo(GstVideoReceiverLog).noquote()
                << "Pipeline dot-graph dumps are disabled. Set GST_DEBUG_DUMP_DOT_DIR=/path/to/dir to enable.";
        }
        return true;
    }();

    bool running = false;
    bool pipelineUp = false;

    GstElement *decoderQueue = nullptr;
    GstElement *recorderQueue = nullptr;

    do {
        _tee = gst_element_factory_make("tee", nullptr);
        if (!_tee)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('tee') failed";
            break;
        }

        GstPad *pad = gst_element_get_static_pad(_tee, "sink");
        if (!pad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        _lastSourceFrameTime = 0;

        _teeProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this, nullptr);
        gst_clear_object(&pad);
        if (_teeProbeId == 0) {
            // _teeProbe updates _lastSourceFrameTime; without it the watchdog timer fires spuriously instead of reporting a real failure.
            qCCritical(GstVideoReceiverLog) << "gst_pad_add_probe(_teeProbe) failed";
            break;
        }

        decoderQueue = gst_element_factory_make("queue", nullptr);
        if (!decoderQueue)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        // leaky=downstream (2) + tiny depth: the live-display branch must drop the oldest
        // buffer on backpressure, not stall the streaming thread. Recording branch (below)
        // keeps default non-leaky semantics so every frame reaches the muxer.
        g_object_set(decoderQueue,
                     "leaky", 2,
                     "max-size-buffers", 2,
                     "max-size-bytes", 0,
                     "max-size-time", G_GUINT64_CONSTANT(0),
                     nullptr);

        _decoderValve = gst_element_factory_make("valve", nullptr);
        if (!_decoderValve)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_decoderValve,
                     "drop", TRUE,
                     nullptr);

        recorderQueue = gst_element_factory_make("queue", nullptr);
        if (!recorderQueue)  {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
            break;
        }

        _recorderValve = gst_element_factory_make("valve", nullptr);
        if (!_recorderValve) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
            break;
        }

        g_object_set(_recorderValve,
                     "drop", TRUE,
                     nullptr);
        // Preserve stream-start/caps/segment events while buffers are dropped. When recording
        // starts later, the newly linked parser receives the negotiated codec configuration.
        gst_util_set_object_arg(G_OBJECT(_recorderValve), "drop-mode", "forward-sticky-events");

        GstElement* pipeline = gst_pipeline_new("receiver");
        if (!pipeline) {
            qCCritical(GstVideoReceiverLog) << "gst_pipeline_new() failed";
            break;
        }
        _pipeline = pipeline;
        // Before anything can post: the context bridges in the handler must see NEED_CONTEXT, which upstream
        // queries during PAUSED->PLAYING.
        _attachBusHandler();

        g_object_set(_pipeline,
                     "message-forward", TRUE,
                     nullptr);

        GStreamer::SourceFactory::Config sourceConfig;
        sourceConfig.jitterBuffer = _config.lowLatency ? GStreamer::SourceFactory::JitterBuffer::None
                                                       : GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
        sourceConfig.latencyMs = _config.rtpJitterLatencyMs;
        // do-retransmission needs ≥40 ms latency headroom over the default 20 ms rtx-delay;
        // forcibly disable for sub-frame latency configurations to avoid retransmit storms.
        sourceConfig.doRetransmission = (_config.rtpJitterLatencyMs >= 40) &&
                                        (sourceConfig.jitterBuffer != GStreamer::SourceFactory::JitterBuffer::None);
        _source = GStreamer::SourceFactory::create(_config.uri, sourceConfig);
        if (!_source) {
            qCCritical(GstVideoReceiverLog) << "SourceFactory::create() failed";
            break;
        }

        gst_bin_add_many(GST_BIN(_pipeline), _source, _tee, decoderQueue, _decoderValve, recorderQueue, _recorderValve, nullptr);

        pipelineUp = true;

        GstPad *srcPad = nullptr;
        (void) gst_element_foreach_src_pad(_source, grabFirstSrcPad, &srcPad);

        if (srcPad) {
            _onNewSourcePad(srcPad);
            gst_clear_object(&srcPad);
        } else {
            (void) g_signal_connect(_source, "pad-added", G_CALLBACK(_onNewPad), this);
        }

        if (!gst_element_link_many(_tee, decoderQueue, _decoderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Unable to link decoder queue";
            break;
        }

        if (!gst_element_link_many(_tee, recorderQueue, _recorderValve, nullptr)) {
            qCCritical(GstVideoReceiverLog) << "Unable to link recorder queue";
            break;
        }

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-initial");
        running = (gst_element_set_state(_pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    } while(0);

    if (!running) {
        qCWarning(GstVideoReceiverLog) << "Failed to start" << _redactedUri();

        if (!pipelineUp) {
            // Not yet owned by the pipeline; the tee probe dies with the tee.
            gst_clear_object(&_recorderValve);
            gst_clear_object(&recorderQueue);
            gst_clear_object(&_decoderValve);
            gst_clear_object(&decoderQueue);
            gst_clear_object(&_tee);
            gst_clear_object(&_source);
            _teeProbeId = 0;
        }

        _resetPipelineState();

        emit onStartComplete(STATUS_FAIL);
        _scheduleReconnect("start failure");
    } else {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-started");
        qCDebug(GstVideoReceiverLog) << "Started" << _redactedUri();

        _watchdogTimer->start();
        emit onStartComplete(STATUS_OK);
    }
}

void GstVideoReceiver::stop()
{
    _runOnWorker([this]() {
        // A user stop also cancels a pending reconnect.
        _reconnectTimer->stop();
        _reconnectAttempts = 0;
        emit onStopComplete(_stopPipeline() ? STATUS_OK : STATUS_INVALID_STATE);
    });
}

bool GstVideoReceiver::_stopPipeline()
{
    if (!_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Not running" << _redactedUri();
        // Still drop any state left behind.
        _resetPipelineState();
        return false;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping" << _redactedUri();

    if (_recordingState == RecordingState::Recording) {
        // EOS at the tee drains the recorder queue into the file. Not sent to the pipeline: a source handles that
        // EOS under its streaming thread's stream lock, which a thread held downstream would never release.
        if (GstPad* teeSink = gst_element_get_static_pad(_tee, "sink")) {
            (void) gst_pad_send_event(teeSink, gst_event_new_eos());
            gst_object_unref(teeSink);
        }
    }

    // Also waits for a stopRecording() still in flight; its EOS is already in the branch.
    if (_recordingState != RecordingState::Idle) {
        switch (_recordingFinalizer->wait(QDeadlineTimer(kEosTimeout))) {
            case RecordingFinalizer::Outcome::Closed:
                qCDebug(GstVideoReceiverLog) << "Recording finalized";
                break;
            case RecordingFinalizer::Outcome::Failed:
                qCWarning(GstVideoReceiverLog) << "Recording failed while finalizing";
                break;
            case RecordingFinalizer::Outcome::Pending:
                qCWarning(GstVideoReceiverLog)
                    << "Recording not finalized within" << std::chrono::milliseconds(kEosTimeout).count()
                    << "ms — forcing pipeline NULL (recording may be truncated; "
                    << "MP4/MOV stay playable up to the last moov update)";
                break;
        }
    }

    _resetPipelineState();

    qCDebug(GstVideoReceiverLog) << "Stopped" << _redactedUri();

    if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(true); hwStats.totalDelivered > 0) {
        qCInfo(GstVideoReceiverLog).noquote()
            << "HW path stats" << _redactedUri() << hwStats.line + HwBuffers::takeExtraPathStats();
    }

    return true;
}

void GstVideoReceiver::_attachBusHandler()
{
    GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
    if (!bus) {
        qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";
        return;
    }
    gst_bus_set_sync_handler(bus, _onBusSyncMessage, this, [](gpointer receiver) {
        static_cast<GstVideoReceiver*>(receiver)->_busHandlerReleased.release();
    });
    gst_object_unref(bus);
    _busHandlerAttached = true;
}

void GstVideoReceiver::_detachBusHandler()
{
    if (!_busHandlerAttached) {
        return;
    }
    _busHandlerAttached = false;

    if (GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline))) {
        gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
        gst_object_unref(bus);
    }
    // GStreamer refcounts the handler per post and releases it once the last in-flight call has returned.
    _busHandlerReleased.acquire();
}

void GstVideoReceiver::_resetPipelineState()
{
    _watchdogTimer->stop();
    _recordingStopTimer->stop();

    if (_pipeline) {
        (void) gst_element_set_state(_pipeline, GST_STATE_NULL);
        (void) gst_element_get_state(_pipeline, nullptr, nullptr, GST_CLOCK_TIME_NONE);
    }

    // Callbacks have stopped: streaming threads are joined and no thread is left in the bus handler. Work they
    // queued for this pipeline is now stale.
    _detachBusHandler();
    _pipelineGeneration.fetch_add(1, std::memory_order_relaxed);

    if (_teeProbeId != 0 && _tee) {
        GstPad* sinkpad = gst_element_get_static_pad(_tee, "sink");
        if (sinkpad) {
            gst_pad_remove_probe(sinkpad, _teeProbeId);
            gst_clear_object(&sinkpad);
        }
    }
    _teeProbeId = 0;

    _removeSourceEosProbe();

    if (_fileSink) {
        _shutdownRecordingBranch();
    }

    // The video sink outlives the pipeline (owned by the caller of startDecoding), so its probe
    // and pipeline membership must be dropped even when start() failed before decoding began.
    if (_decoder || _videoSink) {
        _shutdownDecodingBranch();
    }

    if (_pipeline) {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-stopped");
    }

    gst_clear_object(&_pipeline);

    // Owned by the (now released) pipeline.
    _recorderValve = nullptr;
    _decoderValve = nullptr;
    _tee = nullptr;
    _source = nullptr;

    _lastSourceFrameTime = 0;
    _lastVideoFrameTime = 0;

    if (std::exchange(_streaming, false)) {
        qCDebug(GstVideoReceiverLog) << "Streaming stopped" << _redactedUri();
        emit streamingChanged(false);
    }
}

void GstVideoReceiver::startDecoding(void *sink)
{
    if (!sink) {
        qCCritical(GstVideoReceiverLog) << "VideoSink is NULL";
        return;
    }

    const bool hasWidget = (_widget != nullptr);
    _runOnWorker([this, sink, hasWidget]() { _startDecoding(GST_ELEMENT(sink), hasWidget); });
}

void GstVideoReceiver::_startDecoding(GstElement* videoSink, bool hasWidget)
{
    qCDebug(GstVideoReceiverLog) << "Starting decoding" << _redactedUri();

    if (!hasWidget) {
        qCDebug(GstVideoReceiverLog) << "Video Widget is NULL" << _redactedUri();
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    if (!_pipeline && _videoSink) {
        // Replace a sink stored before start(); also drops its frame probe.
        _shutdownDecodingBranch();
    }

    if (_videoSink || _decoding) {
        qCDebug(GstVideoReceiverLog) << "Already decoding!" << _redactedUri();
        emit onStartDecodingComplete(STATUS_INVALID_STATE);
        return;
    }

    GstPad *pad = gst_element_get_static_pad(videoSink, "sink");
    if (!pad) {
        qCCritical(GstVideoReceiverLog) << "Unable to find sink pad of video sink" << _redactedUri();
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    _lastVideoFrameTime = 0;

    _videoSinkProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, _videoSinkProbe, this, nullptr);
    gst_clear_object(&pad);

    // The caller keeps its own (non-floating) reference; ours is dropped in _shutdownDecodingBranch().
    _videoSink = GST_ELEMENT(gst_object_ref(videoSink));

    if (!_streaming) {
        emit onStartDecodingComplete(STATUS_OK);
        return;
    }

    _ensureVideoSinkInPipeline();

    if (!_addDecoder(_decoderValve)) {
        qCWarning(GstVideoReceiverLog) << "_addDecoder() failed" << _redactedUri();
        _shutdownDecodingBranch();
        emit onStartDecodingComplete(STATUS_FAIL);
        return;
    }

    g_object_set(_decoderValve,
                 "drop", FALSE,
                 nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _redactedUri();

    emit onStartDecodingComplete(STATUS_OK);
}

void GstVideoReceiver::stopDecoding()
{
    if (!_onWorkerThread()) {
        _runOnWorker([this]() { stopDecoding(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping decoding" << _redactedUri();

    // Gate on _videoSink (set by startDecoding) instead of _decoding (which only flips on the first frame),
    // so a stop before the first frame still removes the branch.
    if (!_videoSink) {
        qCDebug(GstVideoReceiverLog) << "Not decoding!" << _redactedUri();
        emit onStopDecodingComplete(STATUS_INVALID_STATE);
        return;
    }

    // The display branch has nothing to flush, so it is torn down at once instead of draining an EOS.
    if (_decoderValve) {
        g_object_set(_decoderValve, "drop", TRUE, nullptr);
    }
    _shutdownDecodingBranch();

    emit onStopDecodingComplete(STATUS_OK);
}

void GstVideoReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    if (!_onWorkerThread()) {
        _runOnWorker([this, videoFile, format]() { startRecording(videoFile, format); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Starting recording" << _redactedUri();

    if (!_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Streaming is not active!" << _redactedUri();
        emit onStartRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    if (_recordingState != RecordingState::Idle) {
        qCDebug(GstVideoReceiverLog) << "Already recording!" << _redactedUri();
        emit onStartRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "New video file:" << videoFile << _redactedUri();

    GstPad* probepad = gst_element_get_static_pad(_recorderValve, "src");
    if (!probepad) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed" << _redactedUri();
        emit onStartRecordingComplete(STATUS_FAIL);
        return;
    }

    const auto failRecordingStart = [this, &probepad]() {
        gst_clear_object(&probepad);
        if (_fileSink) {
            (void) gst_element_set_state(_fileSink, GST_STATE_NULL);
            // Safe before and after a successful link. Removing a still-linked bin leaves the
            // valve's src pad attached to a finalized peer and breaks every later retry.
            gst_element_unlink(_recorderValve, _fileSink);
            GstObject* parent = gst_element_get_parent(_fileSink);
            if (parent) {
                (void) gst_bin_remove(GST_BIN(parent), _fileSink);
                gst_clear_object(&parent);
            }
            gst_clear_object(&_fileSink);
        }
        // Only once the sink is out of the pipeline: until then its errors must still count as the recorder's.
        (void) _recordingFinalizer->disarm();
        emit onStartRecordingComplete(STATUS_FAIL);
    };

    // forward-sticky-events preserves the exact negotiated caps while the valve is closed. Fall
    // back to a proxied caps query during the narrow startup window before negotiation completes.
    GstCaps* inputCaps = gst_pad_get_current_caps(probepad);
    if (!inputCaps) {
        inputCaps = gst_pad_query_caps(probepad, nullptr);
    }
    _fileSink = _makeFileSink(videoFile, format, inputCaps);
    gst_clear_caps(&inputCaps);
    if (!_fileSink) {
        qCCritical(GstVideoReceiverLog) << "_makeFileSink() failed" << _redactedUri();
        failRecordingStart();
        return;
    }

    // Armed before the sink can post, so the bus handler attributes its messages (e.g. a failure to open the
    // file) to this recording instead of to the pipeline.
    _recordingFinalizer->arm(_fileSink);

    if (!gst_bin_add(GST_BIN(_pipeline), _fileSink)) {
        qCCritical(GstVideoReceiverLog) << "gst_bin_add(file sink) failed" << _redactedUri();
        failRecordingStart();
        return;
    }
    (void) gst_object_ref(_fileSink);  // Keep a reference in addition to the pipeline's ownership.

    if (!gst_element_link(_recorderValve, _fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Failed to link valve and file sink" << _redactedUri();
        failRecordingStart();
        return;
    }

    if (!gst_element_sync_state_with_parent(_fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Recording sink failed to start" << _redactedUri();
        failRecordingStart();
        return;
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-filesink");

    // Install a probe on the recording branch to drop buffers until we hit our first keyframe
    // When we hit our first keyframe, we can offset the timestamps appropriately according to the first keyframe time
    // This will ensure the first frame is a keyframe at t=0, and decoding can begin immediately on playback
    _recordingOutput = videoFile;
    _keyframeWatchId =
        gst_pad_add_probe(probepad, GST_PAD_PROBE_TYPE_BUFFER, _keyframeWatch, new KeyframeWatch{this, videoFile},
                          [](gpointer watch) { delete static_cast<KeyframeWatch*>(watch); });
    if (_keyframeWatchId == 0) {
        qCCritical(GstVideoReceiverLog) << "gst_pad_add_probe(_keyframeWatch) failed" << _redactedUri();
        failRecordingStart();
        return;
    }
    gst_clear_object(&probepad);

    g_object_set(_recorderValve,
                 "drop", FALSE,
                 nullptr);

    _recordingState = RecordingState::Recording;
    qCDebug(GstVideoReceiverLog) << "Recording started" << _redactedUri();
    emit onStartRecordingComplete(STATUS_OK);
    emit recordingChanged(true);
}

void GstVideoReceiver::stopRecording()
{
    if (!_onWorkerThread()) {
        _runOnWorker([this]() { stopRecording(); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping recording" << _redactedUri();

    if (!_pipeline || (_recordingState != RecordingState::Recording)) {
        qCDebug(GstVideoReceiverLog) << "Not recording!" << _redactedUri();
        emit onStopRecordingComplete(STATUS_INVALID_STATE);
        return;
    }

    g_object_set(_recorderValve,
                 "drop", TRUE,
                 nullptr);

    // The EOS drains parser -> muxer -> file; _onRecordingFinalized completes the stop once the file is closed, the
    // timer if that is never reported.
    _recordingState = RecordingState::Stopping;
    _recordingStopTimer->start();
    if (!_unlinkBranch(_recorderValve) || (_recordingFinalizer->outcome() != RecordingFinalizer::Outcome::Pending)) {
        _shutdownRecordingBranch();
    }
}

void GstVideoReceiver::takeScreenshot(const QString &imageFile)
{
    if (!_onWorkerThread()) {
        _runOnWorker([this, imageFile]() { takeScreenshot(imageFile); });
        return;
    }

    qCDebug(GstVideoReceiverLog) << "taking screenshot" << _redactedUri();

    // FIXME: record screenshot here
    emit onTakeScreenshotComplete(STATUS_NOT_IMPLEMENTED);
}

void GstVideoReceiver::_onWatchdogTick()
{
    if (!_pipeline) {
        return;
    }

    const qint64 now = monotonicMs();
    // start(0) would otherwise trip on every tick.
    const qint64 timeoutMs = std::max(static_cast<qint64>(_config.timeoutSec) * 1000, kMinWatchdogTimeoutMs);
    qint64 lastSourceFrameTime = _lastSourceFrameTime.load(std::memory_order_relaxed);
    if (lastSourceFrameTime == 0) {
        lastSourceFrameTime = now;
        _lastSourceFrameTime.store(now, std::memory_order_relaxed);
    }

    if (++_statsTickCounter >= 10) {
        _statsTickCounter = 0;
        if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(false); hwStats.totalDelivered > 0) {
            qCDebug(GstVideoReceiverLog).noquote() << "HW path live" << _redactedUri() << hwStats.line;
        }
    }

    // Drain QoS updates accumulated since the last tick (see GST_MESSAGE_QOS).
    if (_qosStatsDirty.exchange(false, std::memory_order_acq_rel)) {
        emit decoderStatsChanged();
    }

    qint64 elapsed = now - lastSourceFrameTime;
    if (elapsed > timeoutMs) {
        qCDebug(GstVideoReceiverLog) << "Stream timeout, no frames for" << elapsed << "ms" << _redactedUri();
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-watchdog-timeout");
        emit timeout();
        _onStreamFailure("source watchdog");
        return;
    }

    // _decoding is only set once a frame reached the sink, so the last frame time is valid. The probe alone writes it:
    // its first write is what reports decoding.
    if (_decoding) {
        elapsed = now - _lastVideoFrameTime.load(std::memory_order_relaxed);
        if (elapsed > (timeoutMs * 2)) {
            qCDebug(GstVideoReceiverLog) << "Video decoder timeout, no frames for" << elapsed << "ms" << _redactedUri();
            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-watchdog-timeout");
            emit timeout();
            _onStreamFailure("decoder watchdog");
        }
    }
}

void GstVideoReceiver::_onStreamFailure(const char* reason)
{
    // A pipeline that delivered frames recovered; the next failure starts the backoff over.
    if (_sourceFrameCount.load(std::memory_order_relaxed) > 0) {
        _reconnectAttempts = 0;
    }

    qCDebug(GstVideoReceiverLog) << "Stopping after" << reason << _redactedUri();
    (void) _stopPipeline();
    emit onStopComplete(STATUS_FAIL);
    _scheduleReconnect(reason);
}

void GstVideoReceiver::_scheduleReconnect(const char* reason)
{
    if (!_autoReconnect) {
        qCDebug(GstVideoReceiverLog) << "Auto-reconnect disabled — not retrying after" << reason;
        return;
    }

    ++_reconnectAttempts;
    const int delaySec = std::min(1 << std::min(_reconnectAttempts - 1, 5), kMaxReconnectBackoffSec);
    qCDebug(GstVideoReceiverLog) << "Scheduling reconnect #" << _reconnectAttempts << "in" << delaySec << "s after"
                                 << reason << _redactedUri();
    _reconnectTimer->start(std::chrono::seconds(delaySec));
}

void GstVideoReceiver::_reconnect()
{
    if (!_autoReconnect) {
        qCDebug(GstVideoReceiverLog) << "Auto-reconnect disabled while waiting; not reconnecting" << _redactedUri();
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Reconnecting (attempt" << _reconnectAttempts << ")" << _redactedUri();
    _startPipeline(_config);
}

void GstVideoReceiver::dumpPipelineGraph(const QString &tag)
{
    _runOnWorker([this, tag]() {
        if (!_pipeline) {
            qCDebug(GstVideoReceiverLog) << "dumpPipelineGraph: pipeline not running";
            return;
        }
        const QByteArray tagUtf8 = tag.toUtf8();
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, tagUtf8.constData());
        const QString dotPath = GStreamer::writePipelineDot(_pipeline, tagUtf8.constData());
        if (!dotPath.isEmpty()) {
            qCDebug(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
        }
    });
}

GstElement *GstVideoReceiver::_makeDecoder()
{
    GstElement *decoder = gst_element_factory_make("decodebin3", nullptr);
    if (!decoder) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('decodebin3') failed";
    }
    return decoder;
}

GstElement* GstVideoReceiver::_makeFileSink(const QString& videoFile, FILE_FORMAT format, const GstCaps* inputCaps)
{
    GstElement *fileSink = nullptr;
    GstElement *splitmux = nullptr;
    GstElement* parser = nullptr;
    GstElement *bin = nullptr;
    GstPad *videopad = nullptr;
    GstPad* parserSrcPad = nullptr;
    GstPad* parserSinkPad = nullptr;
    GstPad *ghostpad = nullptr;

    do {
        if (!isValidFileFormat(format)) {
            qCCritical(GstVideoReceiverLog) << "Unsupported file format";
            break;
        }

        const char* parserFactory = recordingParserFactory(inputCaps);
        if (parserFactory) {
            parser = gst_element_factory_make(parserFactory, nullptr);
            if (!parser) {
                qCCritical(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << parserFactory;
                break;
            }
            g_object_set(parser, "config-interval", -1, nullptr);
        }

        // splitmuxsink owns its own muxer + filesink internally, handles request-pad
        // lifetime, and finalizes asynchronously so EOS no longer wedges the worker
        // thread (replaces the manual qtmux/matroskamux+filesink combo + "stuck muxer"
        // bounded-wait in stop()). max-size-time=0 keeps single-file behaviour.
        splitmux = gst_element_factory_make("splitmuxsink", nullptr);
        if (!splitmux) {
            qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('splitmuxsink') failed";
            break;
        }

        g_object_set(splitmux,
                     "location", qPrintable(videoFile),
                     "muxer-factory", _kFileMux[format],
                     "max-size-time", G_GUINT64_CONSTANT(0),
                     "max-size-bytes", G_GUINT64_CONSTANT(0),
                     "async-finalize", TRUE,
                     // Surface "splitmuxsink-fragment-closed" element messages on the pipeline bus so
                     // stop() can wait on the precise per-fragment finalize signal instead of EOS
                     // (gstsplitmuxsink.c:send_fragment_opened_closed_msg posts this per fragment,
                     // including the final fragment torn down on EOS).
                     "message-forward", TRUE,
                     nullptr);

        // Crash-safe MP4/MOV via robust muxing. faststart is deliberately off: it spools the whole
        // recording to a temp file and writes the real file only at EOS. matroskamux is naturally
        // streamable; skip the GstStructure dance.
        if (format == FILE_FORMAT_MP4 || format == FILE_FORMAT_MOV) {
            GstStructure* muxerProps =
                gst_structure_new("properties", "reserved-max-duration", G_TYPE_UINT64, kRecordingReservedMaxDurationNs,
                                  "reserved-moov-update-period", G_TYPE_UINT64, kRecordingMoovUpdatePeriodNs, nullptr);
            g_object_set(splitmux, "muxer-properties", muxerProps, nullptr);
            gst_structure_free(muxerProps);
        }

        bin = gst_bin_new("sinkbin");
        if (!bin) {
            qCCritical(GstVideoReceiverLog) << "gst_bin_new('sinkbin') failed";
            break;
        }

        // splitmuxsink's video sink pad is a request pad — request once during construction
        // and ghost it as "sink" so the existing recorderValve→fileSink link works unchanged.
        // request_pad_simple (1.20+) does the pad-template lookup internally.
        videopad = gst_element_request_pad_simple(splitmux, "video");
        if (!videopad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_request_pad_simple(splitmuxsink, video) failed";
            break;
        }

        GstElement* binParser = nullptr;
        if (parser) {
            if (!gst_bin_add(GST_BIN(bin), parser)) {
                qCCritical(GstVideoReceiverLog) << "gst_bin_add(parser) failed";
                break;
            }
            binParser = parser;
            parser = nullptr;  // bin now owns it
        }

        if (!gst_bin_add(GST_BIN(bin), splitmux)) {
            qCCritical(GstVideoReceiverLog) << "gst_bin_add(splitmuxsink) failed";
            break;
        }
        splitmux = nullptr;  // bin now owns it

        GstPad* ghostTarget = videopad;
        if (binParser) {
            parserSrcPad = gst_element_get_static_pad(binParser, "src");
            parserSinkPad = gst_element_get_static_pad(binParser, "sink");
            if (!parserSrcPad || !parserSinkPad) {
                qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad(parser) failed";
                break;
            }

            const GstPadLinkReturn linkResult = gst_pad_link(parserSrcPad, videopad);
            if (linkResult != GST_PAD_LINK_OK) {
                qCCritical(GstVideoReceiverLog)
                    << "Failed to link parser and splitmuxsink:" << gst_pad_link_get_name(linkResult);
                break;
            }
            ghostTarget = parserSinkPad;
        }

        ghostpad = gst_ghost_pad_new("sink", ghostTarget);
        if (!ghostpad) {
            qCCritical(GstVideoReceiverLog) << "gst_ghost_pad_new() failed";
            break;
        }

        if (!gst_element_add_pad(bin, ghostpad)) {
            qCCritical(GstVideoReceiverLog) << "gst_element_add_pad(ghost) failed";
            break;
        }
        ghostpad = nullptr;  // bin now owns it

        fileSink = bin;
        bin = nullptr;
    } while(0);

    gst_clear_object(&ghostpad);
    gst_clear_object(&parserSinkPad);
    gst_clear_object(&parserSrcPad);
    // No release_request_pad: on success splitmux is already NULL (owned by bin), and on failure the
    // bin/splitmux unref below finalizes splitmuxsink, which reclaims its "video" request pad itself.
    gst_clear_object(&videopad);
    gst_clear_object(&splitmux);
    gst_clear_object(&parser);
    gst_clear_object(&bin);
    return fileSink;
}

void GstVideoReceiver::_onNewSourcePad(GstPad *pad)
{
    // FIXME: check for caps - if this is not video stream (and preferably - one of these which we have to support) then simply skip it
    GstPad* teeSink = gst_element_get_static_pad(_tee, "sink");
    const GstPadLinkReturn linkResult = teeSink ? gst_pad_link(pad, teeSink) : GST_PAD_LINK_WRONG_HIERARCHY;
    gst_clear_object(&teeSink);
    if (linkResult != GST_PAD_LINK_OK) {
        qCWarning(GstVideoReceiverLog) << "Unable to link source:" << gst_pad_link_get_name(linkResult);
        return;
    }

    if (!_streaming) {
        _streaming = true;
        qCDebug(GstVideoReceiverLog) << "Streaming started" << _redactedUri();
        emit streamingChanged(true);
    }

    // Lives as long as the source pad/pipeline (removed in _resetPipelineState), so stopDecoding()
    // doesn't lose source-EOS detection.
    _removeSourceEosProbe();
    _eosProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _eosProbe, this, nullptr);
    if (_eosProbeId != 0) {
        _eosProbePad = GST_PAD_CAST(gst_object_ref(pad));
    }
    if (!_videoSink) {
        return;
    }

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-source-pad");

    _ensureVideoSinkInPipeline();

    if (!_addDecoder(_decoderValve)) {
        qCWarning(GstVideoReceiverLog) << "_addDecoder() failed";
        _shutdownDecodingBranch();
        return;
    }

    g_object_set(_decoderValve,
                 "drop", FALSE,
                 nullptr);

    qCDebug(GstVideoReceiverLog) << "Decoding started" << _redactedUri();
}

void GstVideoReceiver::_logDecodebin3SelectedCodec(GstElement *decodebin3)
{
    GValue value = G_VALUE_INIT;
    GstIterator *iter = gst_bin_iterate_elements(GST_BIN(decodebin3));
    GstElement *child;

    while (gst_iterator_next(iter, &value) == GST_ITERATOR_OK) {
        child = GST_ELEMENT(g_value_get_object(&value));
        GstElementFactory *factory = gst_element_get_factory(child);

        if (factory) {
            gboolean is_decoder = gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER);
            if (is_decoder) {
                const gchar *decoderKlass = gst_element_factory_get_klass(factory);
                GstPluginFeature *feature = GST_PLUGIN_FEATURE(factory);
                const gchar *featureName = gst_plugin_feature_get_name(feature);
                const guint rank = gst_plugin_feature_get_rank(feature);
                bool isHardwareDecoder = GStreamer::isHardwareDecoderFactory(factory);

                QString pluginName = featureName;
                GstPlugin *plugin = gst_plugin_feature_get_plugin(feature);
                if (plugin) {
                    pluginName = gst_plugin_get_name(plugin);
                    gst_object_unref(plugin);
                }
                qCDebug(GstVideoReceiverLog) << "Decodebin3 selected codec:rank -" << pluginName << "/" << featureName << "-" << decoderKlass << (isHardwareDecoder ? "(HW)" : "(SW)") << ":" << rank;

                const QString newName = QString::fromUtf8(featureName);
                bool nameChanged = false;
                {
                    QMutexLocker locker(&_decoderNameMutex);
                    if (newName != _decoderName) {
                        _decoderName = newName;
                        nameChanged = true;
                    }
                }
                if (nameChanged) {
                    emit decoderStatsChanged();
                }

                // Disable QoS on the internal decoder to prevent cascading
                // frame drops on live streams.  The videodecoder base class
                // aggressively advances earliest_time after the first late
                // frame, causing all subsequent frames to be dropped.
                g_object_set(child, "qos", FALSE, nullptr);
                qCDebug(GstVideoReceiverLog) << "Disabled QoS on internal decoder" << featureName;
            }
        }
        g_value_reset(&value);
    }
    g_value_unset(&value);
    gst_iterator_free(iter);
}


void GstVideoReceiver::_onNewDecoderPad(GstPad *pad)
{
    qCDebug(GstVideoReceiverLog) << "_onNewDecoderPad" << _redactedUri();

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-new-decoder-pad");

    // We should now know what codec decodebin3 selected.
    _logDecodebin3SelectedCodec(_decoder);

    if (!_addVideoSink(pad)) {
        qCCritical(GstVideoReceiverLog) << "_addVideoSink() failed";
    }
}

bool GstVideoReceiver::_addDecoder(GstElement *src)
{
    _decoder = _makeDecoder();
    if (!_decoder) {
        qCCritical(GstVideoReceiverLog) << "_makeDecoder() failed";
        return false;
    }

    (void) gst_object_ref(_decoder);

    (void) gst_bin_add(GST_BIN(_pipeline), _decoder);
    (void) gst_element_sync_state_with_parent(_decoder);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-decoder");

    if (!gst_element_link(src, _decoder)) {
        qCCritical(GstVideoReceiverLog) << "Unable to link decoder";
        gst_element_set_state(_decoder, GST_STATE_NULL);
        (void) gst_element_get_state(_decoder, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        (void) gst_bin_remove(GST_BIN(_pipeline), _decoder);
        gst_clear_object(&_decoder);
        return false;
    }

    GstPad *srcPad = nullptr;
    (void) gst_element_foreach_src_pad(_decoder, grabFirstSrcPad, &srcPad);

    if (srcPad) {
        _onNewDecoderPad(srcPad);
    } else {
        (void) g_signal_connect(_decoder, "pad-added", G_CALLBACK(_onNewPad), this);
    }

    gst_clear_object(&srcPad);
    return true;
}

void GstVideoReceiver::_ensureVideoSinkInPipeline()
{
    if (!_videoSink || !_pipeline) {
        return;
    }

    GstObject *parent = gst_element_get_parent(_videoSink);
    if (parent) {
        gst_object_unref(parent);
        return;
    }

    g_object_set(_videoSink, "sync", !_config.lowLatency, NULL);

    // The sink is never floating here, so the bin takes its own reference; removal drops only that one.
    (void) gst_bin_add(GST_BIN(_pipeline), _videoSink);

    // PAUSED (not READY) triggers downstream caps negotiation before source data arrives.
    (void) gst_element_set_state(_videoSink, GST_STATE_PAUSED);
}

bool GstVideoReceiver::_addVideoSink(GstPad *pad)
{
    GstCaps *caps = gst_pad_query_caps(pad, nullptr);

    _ensureVideoSinkInPipeline();

    GstPad *sinkPad = gst_element_get_static_pad(_videoSink, "sink");
    GstPadLinkReturn linkRet = sinkPad ? gst_pad_link(pad, sinkPad) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkRet != GST_PAD_LINK_OK) {
        qCCritical(GstVideoReceiverLog) << "Unable to link decoder pad to video sink, result:" << linkRet;

        // _ensureVideoSinkInPipeline() added it before linking; detach for the next retry.
        GstObject *parent = gst_element_get_parent(_videoSink);
        if (parent) {
            (void) gst_element_set_state(_videoSink, GST_STATE_NULL);
            (void) gst_element_get_state(_videoSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            (void) gst_bin_remove(GST_BIN(_pipeline), _videoSink);
            gst_clear_object(&parent);
        }

        gst_clear_object(&sinkPad);
        gst_clear_caps(&caps);
        return false;
    }
    gst_clear_object(&sinkPad);

    (void) gst_element_sync_state_with_parent(_videoSink);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-videosink");

    // Determine video size. Errors here are non-fatal.
    QSize videoSize;
    do {
        if (!_decoderValve) {
            qCCritical(GstVideoReceiverLog) << "Unable to determine video size - _decoderValve is NULL"
                                            << _redactedUri();
            break;
        }

        GstPad *valveSrcPad = gst_element_get_static_pad(_decoderValve, "src");
        if (!valveSrcPad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        GstCaps *valveSrcPadCaps = gst_pad_query_caps(valveSrcPad, nullptr);
        if (!valveSrcPadCaps) {
            qCCritical(GstVideoReceiverLog) << "gst_pad_query_caps() failed";
            gst_clear_object(&valveSrcPad);
            break;
        }

        const GstStructure *structure = gst_caps_get_structure(valveSrcPadCaps, 0);
        if (!structure) {
            qCCritical(GstVideoReceiverLog) << "Unable to determine video size - structure is NULL"
                                            << _redactedUri();
            gst_clear_object(&valveSrcPad);
            break;
        }

        gint width = 0;
        gint height = 0;
        (void) gst_structure_get_int(structure, "width", &width);
        (void) gst_structure_get_int(structure, "height", &height);

        // Swap W×H for 90°/270° streams so QML AR is computed on display dimensions.
        gint orientation = 0;
        if (gst_structure_get_int(structure, "video-orientation", &orientation)
            && (orientation == GST_VIDEO_ORIENTATION_90R
                || orientation == GST_VIDEO_ORIENTATION_90L
                || orientation == GST_VIDEO_ORIENTATION_UL_LR
                || orientation == GST_VIDEO_ORIENTATION_UR_LL)) {
            videoSize.setWidth(height);
            videoSize.setHeight(width);
        } else {
            videoSize.setWidth(width);
            videoSize.setHeight(height);
        }

        gst_clear_caps(&valveSrcPadCaps);
        gst_clear_object(&valveSrcPad);
    } while (false);
    emit videoSizeChanged(videoSize);

    gst_clear_caps(&caps);
    return true;
}

void GstVideoReceiver::_noteTeeFrame()
{
    _lastSourceFrameTime.store(monotonicMs(), std::memory_order_relaxed);
    const quint64 sourceFrames = _sourceFrameCount.fetch_add(1, std::memory_order_relaxed) + 1;
    if (sourceFrames == 1) {
        qCDebug(GstVideoReceiverLog).noquote() << "Source receiving frames (tee):" << _redactedUri();
    } else if ((sourceFrames % 300) == 0) {
        qCDebug(GstVideoReceiverLog).noquote()
            << "Source flow: teeFrames=" << sourceFrames << "decoding=" << _decoding << _redactedUri();
    }
}

void GstVideoReceiver::_noteVideoSinkFrame()
{
    if (_lastVideoFrameTime.exchange(monotonicMs(), std::memory_order_relaxed) == 0) {
        _runOnWorkerForPipeline([this]() { _onFirstVideoFrame(); });
    }
}

void GstVideoReceiver::_onFirstVideoFrame()
{
    if (!_videoSink || _decoding) {
        return;
    }
    _decoding = true;
    qCDebug(GstVideoReceiverLog) << "Decoding started";
    emit decodingChanged(true);
}

bool GstVideoReceiver::_unlinkBranch(GstElement *from)
{
    GstPad *src = gst_element_get_static_pad(from, "src");
    if (!src) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstPad *sink = gst_pad_get_peer(src);
    if (!sink) {
        gst_clear_object(&src);
        qCWarning(GstVideoReceiverLog) << "Branch is not linked";
        return false;
    }

    if (!gst_pad_unlink(src, sink)) {
        gst_clear_object(&src);
        gst_clear_object(&sink);
        qCWarning(GstVideoReceiverLog) << "gst_pad_unlink() failed";
        return false;
    }

    gst_clear_object(&src);

    // Send EOS at the beginning of the branch
    const gboolean ret = gst_pad_send_event(sink, gst_event_new_eos());

    gst_clear_object(&sink);

    if (!ret) {
        qCWarning(GstVideoReceiverLog) << "Branch EOS was NOT sent";
        return false;
    }

    qCDebug(GstVideoReceiverLog) << "Branch EOS was sent";

    return true;
}

void GstVideoReceiver::_shutdownDecodingBranch()
{
    if (_decoder) {
        GstObject *parent = gst_element_get_parent(_decoder);
        if (parent) {
            (void) gst_bin_remove(GST_BIN(parent), _decoder);
            (void) gst_element_set_state(_decoder, GST_STATE_NULL);
            (void) gst_element_get_state(_decoder, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_clear_object(&parent);
        }

        gst_clear_object(&_decoder);
    }

    if (_videoSinkProbeId != 0 && _videoSink) {
        GstPad *sinkpad = gst_element_get_static_pad(_videoSink, "sink");
        if (sinkpad) {
            gst_pad_remove_probe(sinkpad, _videoSinkProbeId);
            gst_clear_object(&sinkpad);
        }
    }
    _videoSinkProbeId = 0;

    _lastVideoFrameTime = 0;

    if (_videoSink) {
        GstObject *parent = gst_element_get_parent(_videoSink);
        if (parent) {
            // Drops only the bin's own reference; _videoSink keeps ours until the clear below.
            (void) gst_bin_remove(GST_BIN(parent), _videoSink);
            (void) gst_element_set_state(_videoSink, GST_STATE_NULL);
            (void) gst_element_get_state(_videoSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
            gst_clear_object(&parent);
        }
        gst_clear_object(&_videoSink);
    }

    if (_decoding.exchange(false)) {
        qCDebug(GstVideoReceiverLog) << "Decoding stopped";
        emit decodingChanged(false);
    }

    if (_pipeline) {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-decoding-stopped");
    }
}

void GstVideoReceiver::_removeSourceEosProbe()
{
    if (_eosProbeId != 0 && _eosProbePad) {
        gst_pad_remove_probe(_eosProbePad, _eosProbeId);
    }
    _eosProbeId = 0;
    gst_clear_object(&_eosProbePad);
}

void GstVideoReceiver::_shutdownRecordingBranch()
{
    _recordingStopTimer->stop();

    const gulong keyframeWatchId = _keyframeWatchId.exchange(0);
    if (keyframeWatchId != 0 && _recorderValve) {
        GstPad *probepad = gst_element_get_static_pad(_recorderValve, "src");
        if (probepad) {
            gst_pad_remove_probe(probepad, keyframeWatchId);
            gst_clear_object(&probepad);
        }
    }

    if (_fileSink) {
        // Removed before going NULL, so nothing it posts while stopping reaches the bus as a pipeline error.
        gst_bin_remove(GST_BIN(_pipeline), _fileSink);
        gst_element_set_state(_fileSink, GST_STATE_NULL);
        (void) gst_element_get_state(_fileSink, nullptr, nullptr, GST_CLOCK_TIME_NONE);
        gst_clear_object(&_fileSink);
    }
    const RecordingFinalizer::Outcome outcome = _recordingFinalizer->disarm();

    const RecordingState state = std::exchange(_recordingState, RecordingState::Idle);
    if (state != RecordingState::Idle) {
        qCDebug(GstVideoReceiverLog) << "Recording stopped";
        emit recordingChanged(false);
    }
    if (state == RecordingState::Stopping) {
        emit onStopRecordingComplete((outcome == RecordingFinalizer::Outcome::Closed) ? STATUS_OK : STATUS_FAIL);
    }

    if (_pipeline) {
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-recording-stopped");
    }
}

GstBusSyncReply GstVideoReceiver::_onBusSyncMessage(GstBus* bus, GstMessage* message, gpointer user_data)
{
    // The context bridges answer NEED_CONTEXT synchronously and consume (unref) the requests they handle.
    if (HwBuffers::onBusSyncMessage(bus, message, nullptr) == GST_BUS_DROP) {
        return GST_BUS_DROP;
    }

    auto* self = static_cast<GstVideoReceiver*>(user_data);
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            self->_onBusError(message);
            break;
        case GST_MESSAGE_WARNING:
            logBusWarning(message);
            break;
        case GST_MESSAGE_EOS:
            // Every branch drained, the recorder included: the fallback for a missed fragment-closed.
            if (self->_recordingFinalizer->settle(RecordingFinalizer::Outcome::Closed)) {
                self->_runOnWorkerForPipeline([self]() { self->_onRecordingFinalized(); });
            }
            break;
        case GST_MESSAGE_ELEMENT:
            self->_onBusElement(message);
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            selectVideoStreams(message);
            break;
        case GST_MESSAGE_QOS:
            self->_onBusQos(message);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            self->_onBusStateChanged(message);
            break;
        case GST_MESSAGE_LATENCY:
            self->_runOnWorkerForPipeline([self]() { (void) gst_bin_recalculate_latency(GST_BIN(self->_pipeline)); });
            // Re-prime sink-side latency tracking after the pipeline recalculation (e.g. RTSP
            // jitter-buffer reconfigure). Controllers live on the GUI thread; hop there to query.
            QMetaObject::invokeMethod(
                self,
                [self]() {
                    for (auto* c : QGCQVideoSinkController::controllersOf(self)) {
                        c->refreshLatency();
                    }
                },
                Qt::QueuedConnection);
            break;
        default:
            break;
    }

    // Every message is handled above and nothing pops this bus; queueing it would grow the bus for the
    // pipeline's lifetime. A sync handler that drops a message owns it.
    gst_message_unref(message);
    return GST_BUS_DROP;
}

void GstVideoReceiver::_onBusError(GstMessage* message)
{
    gchar* debug = nullptr;
    GError* error = nullptr;
    gst_message_parse_error(message, &error, &debug);
    const bool recoverableH265PaciError = isRecoverableH265PaciError(message, error, debug);
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");
    if (debug) {
        qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug;
    }
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);

    if (recoverableH265PaciError) {
        qCWarning(GstVideoReceiverLog) << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << text;
        return;
    }

    bool fromRecording = false;
    const bool settled =
        _recordingFinalizer->settleFrom(GST_MESSAGE_SRC(message), RecordingFinalizer::Outcome::Failed, &fromRecording);
    if (fromRecording) {
        // A recorder failure (disk full, header reservation overrun) ends the recording, not the stream. Closing the
        // valve now, before the failing element returns its flow error, keeps that error out of the tee: the valve
        // reports OK for a push that fails after drop was set.
        qCWarning(GstVideoReceiverLog) << "Recording error:" << text;
        g_object_set(_recorderValve, "drop", TRUE, nullptr);
        if (settled) {
            _runOnWorkerForPipeline([this]() { _onRecorderError(); });
        }
        return;
    }

    qCWarning(GstVideoReceiverLog) << "GStreamer error:" << text;
    // Drops cached GPU devices on a device-loss error.
    HwBuffers::dispatchBusMessage(message);
    // A failing pipeline usually posts several ERRORs; the first one's teardown makes the rest stale.
    _runOnWorkerForPipeline([this]() { _onPipelineError(); });
}

void GstVideoReceiver::_onBusElement(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);
    if (!structure) {
        return;
    }

    if (gst_structure_has_name(structure, "qgc-caps-info")) {
        gint w = 0, h = 0;
        const gchar* fmt = gst_structure_get_string(structure, "format");
        gst_structure_get_int(structure, "width", &w);
        gst_structure_get_int(structure, "height", &h);
        const QString format = QString::fromUtf8(fmt ? fmt : "");
        const QSize resolution(w, h);
        // src compared by address only on the GUI thread; never dereferenced (may be gone by then).
        void* src = GST_MESSAGE_SRC(message);
        QMetaObject::invokeMethod(
            this,
            [this, format, resolution, src]() {
                for (auto* c : QGCQVideoSinkController::controllersOf(this)) {
                    if (static_cast<const void*>(c->element()) == src) {
                        c->updateNegotiation(format, resolution);
                    }
                }
            },
            Qt::QueuedConnection);
        return;
    }

    // splitmuxsink posts fragment-closed once the muxer has written the file and passed EOS to its sink.
    GstObject* closedSource = nullptr;
    if (gst_structure_has_name(structure, "splitmuxsink-fragment-closed")) {
        closedSource = GST_MESSAGE_SRC(message);
    }

    // Branch EOS (pipeline message-forward); only the recorder's needs handling.
    GstMessage* forwarded = nullptr;
    if (gst_structure_has_name(structure, "GstBinForwarded")) {
        gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forwarded, nullptr);
        if (forwarded && (GST_MESSAGE_TYPE(forwarded) == GST_MESSAGE_EOS)) {
            closedSource = GST_MESSAGE_SRC(forwarded);
        }
    }

    if (closedSource && _recordingFinalizer->settleFrom(closedSource, RecordingFinalizer::Outcome::Closed)) {
        _runOnWorkerForPipeline([this]() { _onRecordingFinalized(); });
    }
    gst_clear_message(&forwarded);
}

void GstVideoReceiver::_onBusQos(GstMessage* message)
{
    guint64 processed = 0, dropped = 0;
    gst_message_parse_qos_stats(message, nullptr, &processed, &dropped);

    gint64 jitter = 0;
    gdouble proportion = 0;
    gint quality = 0;
    gst_message_parse_qos_values(message, &jitter, &proportion, &quality);

    _processedFrames.store(processed, std::memory_order_relaxed);
    _droppedFrames.store(dropped, std::memory_order_relaxed);
    _currentJitterNs.store(jitter, std::memory_order_relaxed);
    _qosProportion.store(proportion, std::memory_order_relaxed);
    _qosQuality.store(quality, std::memory_order_relaxed);
    // GstBaseSink can post QOS per dropped buffer; defer the emit to the 1 Hz
    // watchdog tick so QML isn't flooded from the streaming thread.
    _qosStatsDirty.store(true, std::memory_order_release);
}

void GstVideoReceiver::_onBusStateChanged(GstMessage* message)
{
    // Only our pipeline is a GstPipeline on its own bus.
    if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message))) {
        return;
    }
    GstState oldState = GST_STATE_NULL, newState = GST_STATE_NULL;
    gst_message_parse_state_changed(message, &oldState, &newState, nullptr);
    if ((newState == GST_STATE_PLAYING) && (oldState != GST_STATE_PLAYING)) {
        _runOnWorkerForPipeline([this]() { _onPipelinePlaying(); });
    }
}

void GstVideoReceiver::_onPipelinePlaying()
{
    GstClockTime min = 0, max = 0;
    GstQuery* q = gst_query_new_latency();
    if (gst_element_query(_pipeline, q)) {
        gboolean live = FALSE;
        gst_query_parse_latency(q, &live, &min, &max);
    }
    gst_query_unref(q);
    const QString decName = decoderName();
    qCDebug(GstVideoReceiverLog).noquote() << "Pipeline PLAYING:" << _redactedUri()
                                           << "decoder:" << (decName.isEmpty() ? QStringLiteral("(pending)") : decName)
                                           << "min-latency:" << (min / 1000000) << "ms"
                                           << "max-latency:" << (max / 1000000) << "ms";
}

void GstVideoReceiver::_onPipelineError()
{
    // Native dump path (no-op without GST_DEBUG_DUMP_DOT_DIR) plus an unconditional
    // CacheLocation fallback so field-bug-report bundles include pipeline topology.
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-error");
    const QString dotPath = GStreamer::writePipelineDot(_pipeline, "pipeline-error");
    if (!dotPath.isEmpty()) {
        qCDebug(GstVideoReceiverLog) << "Pipeline graph saved to" << dotPath;
    }

    _onStreamFailure("pipeline error");
}

void GstVideoReceiver::_onRecordingFinalized()
{
    // Recording: the source ended and the file closed with it; stop() collects it. Idle: already handled.
    if (_recordingState != RecordingState::Stopping) {
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Recording finalized" << _redactedUri();
    _shutdownRecordingBranch();
}

void GstVideoReceiver::_onRecorderError()
{
    // Idle: startRecording() already reported the failure.
    if (_recordingState == RecordingState::Idle) {
        return;
    }
    _shutdownRecordingBranch();

    // Adding the sink made the live pipeline wait for it to preroll; a sink that failed first leaves that
    // PAUSED->PLAYING commit failed, and later sinks would sync to PAUSED. With the sink gone, resume PLAYING.
    if (gst_element_set_state(_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qCWarning(GstVideoReceiverLog) << "Pipeline did not resume after the recording failed" << _redactedUri();
    }
}

void GstVideoReceiver::_onRecordingStopTimeout()
{
    if (_recordingState != RecordingState::Stopping) {
        return;
    }
    qCWarning(GstVideoReceiverLog) << "Recording not finalized within" << std::chrono::milliseconds(kEosTimeout).count()
                                   << "ms; closing it (MP4/MOV stay playable up to the last moov update)";
    _shutdownRecordingBranch();
}

void GstVideoReceiver::_onNewPad(GstElement *element, GstPad *pad, gpointer data)
{
    auto* self = static_cast<GstVideoReceiver*>(data);

    // Runs on a streaming thread; the pipeline is only changed on the worker. Data waits at the pad until the
    // worker has linked it, since an unlinked push would fail the stream.
    const gulong blockId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, holdPad, nullptr, nullptr);
    self->_runOnWorkerForPipeline([self, blockId, padElement = GstElementRef(GST_ELEMENT(gst_object_ref(element))),
                                   newPad = GstPadRef(GST_PAD(gst_object_ref(pad)))]() {
        self->_onPadAdded(padElement.get(), newPad.get());
        gst_pad_remove_probe(newPad.get(), blockId);
    });
}

void GstVideoReceiver::_onPadAdded(GstElement* element, GstPad* pad)
{
    if (element == _source) {
        _onNewSourcePad(pad);
    } else if (element == _decoder) {
        _onNewDecoderPad(pad);
    } else {
        qCDebug(GstVideoReceiverLog) << "Ignoring a pad of a removed element";
    }
}

GstPadProbeReturn GstVideoReceiver::_teeProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad); Q_UNUSED(info)

    if (user_data) {
        GstVideoReceiver *pThis = static_cast<GstVideoReceiver*>(user_data);
        pThis->_noteTeeFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_videoSinkProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad); Q_UNUSED(info)

    if (user_data) {
        static_cast<GstVideoReceiver*>(user_data)->_noteVideoSinkFrame();
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_eosProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad);

    if (!user_data || !info) {
        return GST_PAD_PROBE_OK;
    }

    const GstEvent* event = gst_pad_probe_info_get_event(info);
    if (event && (GST_EVENT_TYPE(event) == GST_EVENT_EOS)) {
        auto* self = static_cast<GstVideoReceiver*>(user_data);
        self->_runOnWorkerForPipeline([self]() { self->_onStreamFailure("end of stream"); });
    }

    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn GstVideoReceiver::_keyframeWatch(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    if (!info || !user_data) {
        qCCritical(GstVideoReceiverLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    // Upstream of the tee is always a parser (GstBaseParse), which pushes single buffers, never lists.
    GstBuffer *buf = gst_pad_probe_info_get_buffer(info);
    if (!buf || GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT) || !GST_BUFFER_PTS_IS_VALID(buf)) {
        // wait for a timestamped keyframe
        return GST_PAD_PROBE_DROP;
    }

    // The pad offset shifts running time (gst_segment_offset_running_time), so subtract the
    // keyframe's running time, not its PTS; they differ when the segment doesn't start at 0.
    GstClockTime runningTime = GST_CLOCK_TIME_NONE;
    if (GstEvent* segmentEvent = gst_pad_get_sticky_event(pad, GST_EVENT_SEGMENT, 0)) {
        const GstSegment* segment = nullptr;
        gst_event_parse_segment(segmentEvent, &segment);
        if (segment && (segment->format == GST_FORMAT_TIME)) {
            runningTime = gst_segment_to_running_time(segment, GST_FORMAT_TIME, GST_BUFFER_PTS(buf));
        }
        gst_event_unref(segmentEvent);
    }
    if (!GST_CLOCK_TIME_IS_VALID(runningTime)) {
        return GST_PAD_PROBE_DROP;
    }

    // set media file '0' offset to current timeline position - we don't want to touch other elements in the graph, except these which are downstream!
    gst_pad_set_offset(pad, -static_cast<gint64>(runningTime));

    qCDebug(GstVideoReceiverLog) << "Got keyframe, stop dropping buffers";

    const auto* watch = static_cast<const KeyframeWatch*>(user_data);
    emit watch->receiver->recordingStarted(watch->file);

    // Returning REMOVE drops the probe; forget its id so _shutdownRecordingBranch doesn't remove it again.
    gulong ownId = GST_PAD_PROBE_INFO_ID(info);
    (void) watch->receiver->_keyframeWatchId.compare_exchange_strong(ownId, 0);
    return GST_PAD_PROBE_REMOVE;
}
