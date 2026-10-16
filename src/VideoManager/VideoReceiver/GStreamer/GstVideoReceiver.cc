#include "GstVideoReceiver.h"

#include <chrono>
#include <gst/gst.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include "GStreamerHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "gstqgc/gstqgcqvideosink.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {
// Destructor wait for the worker. Must exceed the recording finalize timeout so a finalizing recording isn't
// abandoned.
constexpr std::chrono::seconds kWorkerShutdownTimeout = RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2};
constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// The live GPU path counters are logged every this many watchdog ticks.
constexpr int kHwStatsLogIntervalTicks = 10;
// Source flow is logged every this many frames.
constexpr quint64 kSourceFlowLogIntervalFrames = 300;
// Retransmission needs playout latency left over after the source factory's 25 ms rtx-delay; with less, recovered
// packets arrive too late and only add load.
constexpr int kMinRetransmissionLatencyMs = 40;

// Blocking probe: returning OK keeps the pad blocked until the probe is removed.
GstPadProbeReturn holdPad(GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer /*userData*/)
{
    return GST_PAD_PROBE_OK;
}

void logBusWarning(GstMessage* message)
{
    // GStreamer posts WARNING for caps mismatches, decoder fallbacks, clock drift — surfacing keeps these
    // visible without escalating to STATUS_FAIL.
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_warning(message, &rawError, &rawDebug);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const GStreamer::GCharPtr debug = GStreamer::adoptGChar(rawDebug);
    qCWarning(GstVideoReceiverLog) << "GStreamer warning:" << (error ? error->message : "(no message)")
                                   << "debug:" << (debug ? debug.get() : "(none)");
}

/// Selecting only video keeps decodebin3 from instantiating audio decoder branches. Done synchronously so the
/// selection is in place before decodebin3 falls back to its default one.
void selectVideoStreams(GstMessage* message)
{
    GstStreamCollection* collection = nullptr;
    gst_message_parse_stream_collection(message, &collection);
    const GStreamer::GstObjectPtr collectionRef = GStreamer::adoptObject(GST_OBJECT_CAST(collection));
    if (!collectionRef) {
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
}

bool isRecoverableH265PaciError(GstMessage* msg, const GError* error, const gchar* debug)
{
    if (!msg || !error || (error->domain != GST_STREAM_ERROR) || (error->code != GST_STREAM_ERROR_FORMAT) || !debug ||
        !g_strrstr(debug, "NAL unit type 50 not supported yet")) {
        return false;
    }

    GstObject* src = GST_MESSAGE_SRC(msg);
    if (!src || !GST_IS_ELEMENT(src)) {
        return false;
    }

    GstElementFactory* factory = gst_element_get_factory(GST_ELEMENT(src));
    return factory && (g_strcmp0(GST_OBJECT_NAME(factory), "rtph265depay") == 0);
}

bool pipelineEnteredPlaying(GstMessage* message)
{
    // Only our pipeline is a GstPipeline on its own bus.
    if (!GST_IS_PIPELINE(GST_MESSAGE_SRC(message))) {
        return false;
    }
    GstState oldState = GST_STATE_NULL, newState = GST_STATE_NULL;
    gst_message_parse_state_changed(message, &oldState, &newState, nullptr);
    return (newState == GST_STATE_PLAYING) && (oldState != GST_STATE_PLAYING);
}

}  // namespace

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _workerContext(new QObject())
    , _watchdogTimer(new QTimer(_workerContext))
    , _reconnectTimer(new QTimer(_workerContext))
    , _recordingBranch(*this, _workerContext)
    , _decodingBranch(*this)
{
    qCDebug(GstVideoReceiverLog) << this;

    _workerThread->setObjectName(QStringLiteral("GstVideoReceiver"));
    _watchdogTimer->setInterval(kWatchdogInterval);
    _reconnectTimer->setSingleShot(true);
    // Backoff delays are asserted by tests; a coarse timer may fire up to 5% early.
    _reconnectTimer->setTimerType(Qt::PreciseTimer);
    (void) connect(_watchdogTimer, &QTimer::timeout, _workerContext, [this]() { _onWatchdogTick(); });
    (void) connect(_reconnectTimer, &QTimer::timeout, _workerContext, [this]() { _reconnect(); });

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

QString GstVideoReceiver::_redactedUri() const
{
    return _config.redactedUri;
}

void GstVideoReceiver::start(uint32_t timeout)
{
    const StartConfig config{
        .name = _name,
        .uri = _uri,
        .redactedUri = QGCNetworkHelper::redactedUrlForLogging(_uri),
        .timeoutSec = timeout,
        .lowLatency = _lowLatency,
        .rtpJitterLatencyMs = _rtpJitterLatencyMs,
    };
    _runOnWorker([this, config]() {
        // A user start supersedes any pending reconnect and restarts the backoff.
        _reconnectTimer->stop();
        _watchdog.resetBackoff();
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
    _watchdog.start(_config.timeoutSec);

    qCDebug(GstVideoReceiverLog) << "Starting" << _redactedUri() << ", lowLatency" << _config.lowLatency << ", timeout"
                                 << _config.timeoutSec;

    bool running = false;

    do {
        _dotDumper.setReceiverName(_config.name);
        _pipeline = GStreamer::adoptElement(gst_pipeline_new(_dotDumper.pipelineName().constData()));
        if (!_pipeline) {
            qCWarning(GstVideoReceiverLog) << "gst_pipeline_new() failed";
            break;
        }
        // Nothing else ever takes a ref on the pipeline, so ours is a plain one.
        (void) gst_object_ref_sink(_pipeline.get());
        _attachBus();

        g_object_set(_pipeline.get(), "message-forward", TRUE, nullptr);

        _tee = gst_element_factory_make("tee", nullptr);
        if (!_tee) {
            qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('tee') failed";
            break;
        }
        (void) gst_bin_add(GST_BIN(_pipeline.get()), _tee);

        const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_tee, "sink"));
        if (!teeSink) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
            break;
        }

        if (!_teeFrameProbe.add(teeSink.get(), GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this)) {
            // _teeProbe feeds the watchdog; without it the watchdog fires spuriously instead of reporting a real
            // failure.
            qCCritical(GstVideoReceiverLog) << "Failed to add the tee frame probe";
            break;
        }

        if (!_decodingBranch.build(_pipeline.get(), _tee)) {
            break;
        }

        if (!_recordingBranch.build(_pipeline.get(), _tee)) {
            break;
        }

        GStreamer::SourceFactory::Config sourceConfig;
        sourceConfig.jitterBuffer = _config.lowLatency ? GStreamer::SourceFactory::JitterBuffer::None
                                                       : GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
        sourceConfig.latencyMs = _config.rtpJitterLatencyMs;
        sourceConfig.doRetransmission = (_config.rtpJitterLatencyMs >= kMinRetransmissionLatencyMs) &&
                                        (sourceConfig.jitterBuffer != GStreamer::SourceFactory::JitterBuffer::None);
        _source = GStreamer::SourceFactory::create(_config.uri, sourceConfig);
        if (!_source) {
            qCWarning(GstVideoReceiverLog) << "Failed to create the source for" << _redactedUri();
            break;
        }
        (void) gst_bin_add(GST_BIN(_pipeline.get()), _source);

        if (const GStreamer::GstPadPtr srcPad = GStreamer::firstSrcPad(_source)) {
            _onNewSourcePad(srcPad.get());
        } else {
            (void) g_signal_connect(_source, "pad-added", G_CALLBACK(_onNewPad), this);
        }

        _dotDumper.dump(_pipeline.get(), "pipeline-initial");
        running = (gst_element_set_state(_pipeline.get(), GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    } while(0);

    if (!running) {
        qCWarning(GstVideoReceiverLog) << "Failed to start" << _redactedUri();
        _resetPipelineState();

        emit onStartComplete(STATUS_FAIL);
        _scheduleReconnect("start failure");
    } else {
        _dotDumper.dump(_pipeline.get(), "pipeline-started");
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
        _watchdog.resetBackoff();
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

    _recordingBranch.finishForTeardown();

    // Before the teardown releases the video sink.
    const GstQgc::PathStats hwStats = _decodingBranch.sinkPathStats();

    _resetPipelineState();

    qCDebug(GstVideoReceiverLog) << "Stopped" << _redactedUri();

    if (hwStats.totalDelivered > 0) {
        qCDebug(GstVideoReceiverLog).noquote()
            << "GPU path stats" << _redactedUri() << hwStats.line + HwBuffers::takeExtraPathStats();
    }

    return true;
}

void GstVideoReceiver::_resetPipelineState()
{
    _watchdogTimer->stop();

    if (_pipeline) {
        // Synchronous: the streaming threads have stopped when this returns.
        (void) gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
    }

    // Callbacks have stopped: streaming threads are joined and no thread is left in the bus handler. Work they
    // queued for this pipeline is now stale.
    _detachBus();
    _pipelineGeneration.fetch_add(1, std::memory_order_relaxed);

    _teeFrameProbe.remove();
    _sourceEosProbe.remove();

    _recordingBranch.reset();
    _decodingBranch.reset();

    _dotDumper.dump(_pipeline.get(), "pipeline-stopped");

    _pipeline.reset();

    // Owned by the (now released) pipeline.
    _tee = nullptr;
    _source = nullptr;

    if (std::exchange(_streaming, false)) {
        qCDebug(GstVideoReceiverLog) << "Streaming stopped" << _redactedUri();
        emit streamingChanged(false);
    }
}

void GstVideoReceiver::startDecoding(void* sink)
{
    if (!sink) {
        qCCritical(GstVideoReceiverLog) << "VideoSink is NULL";
        return;
    }

    const bool hasWidget = (_widget != nullptr);
    _runOnWorker([this, sink, hasWidget]() { _decodingBranch.start(GST_ELEMENT(sink), hasWidget, _streaming); });
}

void GstVideoReceiver::stopDecoding()
{
    _runOnWorker([this]() { _decodingBranch.stop(); });
}

void GstVideoReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    _runOnWorker([this, videoFile, format]() { _recordingBranch.start(videoFile, format); });
}

void GstVideoReceiver::stopRecording()
{
    _runOnWorker([this]() { _recordingBranch.stop(); });
}

void GstVideoReceiver::_onWatchdogTick()
{
    if (!_pipeline) {
        return;
    }

    if (++_statsTickCounter >= kHwStatsLogIntervalTicks) {
        _statsTickCounter = 0;
        if (const GstQgc::PathStats hwStats = _decodingBranch.sinkPathStats(); hwStats.totalDelivered > 0) {
            qCDebug(GstVideoReceiverLog).noquote() << "GPU path live" << _redactedUri() << hwStats.line;
        }
    }

    const StreamWatchdog::Result result = _watchdog.check(StreamWatchdog::now(), _decodingBranch.isDecoding());
    if (result.stall == StreamWatchdog::Stall::None) {
        return;
    }

    const bool sourceStalled = (result.stall == StreamWatchdog::Stall::Source);
    qCDebug(GstVideoReceiverLog) << (sourceStalled ? "Stream timeout, no frames for"
                                                   : "Video decoder timeout, no frames for")
                                 << result.silentMs << "ms" << _redactedUri();
    _dotDumper.snapshot(_pipeline.get(), QStringLiteral("pipeline-watchdog-timeout"));
    emit timeout();
    _onStreamFailure(sourceStalled ? "source watchdog" : "decoder watchdog");
}

void GstVideoReceiver::_onStreamFailure(const char* reason)
{
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

    const std::chrono::seconds delay = _watchdog.nextReconnectDelay();
    qCDebug(GstVideoReceiverLog) << "Scheduling reconnect #" << _watchdog.reconnectAttempts() << "in" << delay.count()
                                 << "s after" << reason << _redactedUri();
    _reconnectTimer->start(delay);
}

void GstVideoReceiver::_reconnect()
{
    if (!_autoReconnect) {
        qCDebug(GstVideoReceiverLog) << "Auto-reconnect disabled while waiting; not reconnecting" << _redactedUri();
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Reconnecting (attempt" << _watchdog.reconnectAttempts() << ")" << _redactedUri();
    _startPipeline(_config);
}

void GstVideoReceiver::dumpPipelineGraph(const QString& tag)
{
    _runOnWorker([this, tag]() {
        if (!_pipeline) {
            qCDebug(GstVideoReceiverLog) << "No running pipeline to dump";
            return;
        }
        _dotDumper.snapshot(_pipeline.get(), tag);
    });
}

void GstVideoReceiver::_onNewSourcePad(GstPad *pad)
{
    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_tee, "sink"));
    const GstPadLinkReturn linkResult = teeSink ? gst_pad_link(pad, teeSink.get()) : GST_PAD_LINK_WRONG_HIERARCHY;
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
    (void) _sourceEosProbe.add(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _eosProbe, this);

    _decodingBranch.onSourceStreaming();
}

void GstVideoReceiver::_noteTeeFrame()
{
    const quint64 sourceFrames = _watchdog.noteSourceFrame(StreamWatchdog::now());
    if (sourceFrames == 1) {
        qCDebug(GstVideoReceiverLog).noquote() << "Source receiving frames (tee):" << _redactedUri();
    } else if ((sourceFrames % kSourceFlowLogIntervalFrames) == 0) {
        qCDebug(GstVideoReceiverLog).noquote() << "Source flow: teeFrames=" << sourceFrames
                                               << "decoding=" << _decodingBranch.isDecoding() << _redactedUri();
    }
}

void GstVideoReceiver::_onPipelinePlaying()
{
    GstClockTime min = 0, max = 0;
    const GStreamer::GstQueryPtr query = GStreamer::adoptQuery(gst_query_new_latency());
    if (gst_element_query(_pipeline.get(), query.get())) {
        gboolean live = FALSE;
        gst_query_parse_latency(query.get(), &live, &min, &max);
    }
    const QString decName = decoderName();
    qCDebug(GstVideoReceiverLog).noquote() << "Pipeline PLAYING:" << _redactedUri()
                                           << "decoder:" << (decName.isEmpty() ? QStringLiteral("(pending)") : decName)
                                           << "min-latency:" << GST_TIME_AS_MSECONDS(min) << "ms"
                                           << "max-latency:" << GST_TIME_AS_MSECONDS(max) << "ms";
}

void GstVideoReceiver::_onPipelineError()
{
    _dotDumper.snapshot(_pipeline.get(), QStringLiteral("pipeline-error"));

    _onStreamFailure("pipeline error");
}

void GstVideoReceiver::_attachBus()
{
    _bus = GStreamer::adoptBus(gst_pipeline_get_bus(GST_PIPELINE(_pipeline.get())));
    if (!_bus) {
        qCCritical(GstVideoReceiverLog) << "gst_pipeline_get_bus() failed";
        return;
    }
    gst_bus_set_sync_handler(_bus.get(), _onBusSyncMessage, this, [](gpointer receiver) {
        static_cast<GstVideoReceiver*>(receiver)->_busHandlerReleased.release();
    });
}

void GstVideoReceiver::_detachBus()
{
    if (!_bus) {
        return;
    }

    gst_bus_set_sync_handler(_bus.get(), nullptr, nullptr, nullptr);
    _bus.reset();
    // GStreamer refcounts the handler per post and releases it once the last in-flight call has returned.
    _busHandlerReleased.acquire();
}

GstBusSyncReply GstVideoReceiver::_onBusSyncMessage(GstBus* /*bus*/, GstMessage* message, gpointer user_data)
{
    // The context bridges answer NEED_CONTEXT synchronously and consume (unref) the requests they handle.
    if (HwBuffers::onBusSyncMessage(message) == GST_BUS_DROP) {
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
            self->_recordingBranch.onBusEos();
            break;
        case GST_MESSAGE_ELEMENT:
            if (gst_message_has_name(message, GstQgc::kCapsInfoMessage)) {
                self->_decodingBranch.onBusCapsInfo(message);
            } else {
                self->_recordingBranch.onBusElement(message);
            }
            break;
        case GST_MESSAGE_STREAM_COLLECTION:
            selectVideoStreams(message);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if (pipelineEnteredPlaying(message)) {
                self->_runOnWorkerForPipeline([self]() { self->_onPipelinePlaying(); });
            }
            break;
        case GST_MESSAGE_LATENCY:
            // Recalculation hands the sinks their new latency in a LATENCY event.
            self->_runOnWorkerForPipeline(
                [self]() { (void) gst_bin_recalculate_latency(GST_BIN(self->_pipeline.get())); });
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
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_error(message, &rawError, &rawDebug);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const GStreamer::GCharPtr debug = GStreamer::adoptGChar(rawDebug);
    const bool recoverableH265PaciError = isRecoverableH265PaciError(message, error.get(), debug.get());
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");
    if (debug) {
        qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug.get();
    }

    if (recoverableH265PaciError) {
        qCWarning(GstVideoReceiverLog) << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << text;
        return;
    }

    if (_recordingBranch.onBusError(message, text)) {
        return;
    }

    const GStreamer::GCharPtr sourcePath = GStreamer::adoptGChar(
        GST_MESSAGE_SRC(message) ? gst_object_get_path_string(GST_MESSAGE_SRC(message)) : nullptr);
    qCWarning(GstVideoReceiverLog) << "GStreamer error from" << (sourcePath ? sourcePath.get() : "(unknown)") << ":"
                                   << text;
    HwBuffers::onPipelineError(error.get(), debug.get());
    // A failing pipeline usually posts several ERRORs; the first one's teardown makes the rest stale.
    _runOnWorkerForPipeline([this]() { _onPipelineError(); });
}

void GstVideoReceiver::_onNewPad(GstElement* element, GstPad* pad, gpointer data)
{
    auto* self = static_cast<GstVideoReceiver*>(data);

    // Runs on a streaming thread; the pipeline is only changed on the worker. Data waits at the pad until the
    // worker has linked it, since an unlinked push would fail the stream.
    const gulong blockId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, holdPad, nullptr, nullptr);
    self->_runOnWorkerForPipeline([self, blockId,
                                   padElement = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(element))),
                                   newPad = GStreamer::adoptPad(GST_PAD(gst_object_ref(pad)))]() {
        self->_onPadAdded(padElement.get(), newPad.get());
        gst_pad_remove_probe(newPad.get(), blockId);
    });
}

void GstVideoReceiver::_onPadAdded(GstElement* element, GstPad* pad)
{
    if (element == _source) {
        _onNewSourcePad(pad);
    } else if (element == _decodingBranch.decoder()) {
        _decodingBranch.onDecoderPad(pad);
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
