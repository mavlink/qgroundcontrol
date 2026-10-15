//-----------------------------------------------------------------------------
// Our pipeline look like this:
//
//              +-->DecodingBranch: queue-->valve[-->decodebin3-->video sink]
//              |
// _source-->_tee
//              |
//              +-->RecordingBranch: queue-->valve[-->sinkbin]
//-----------------------------------------------------------------------------

#include "GstVideoReceiver.h"

#include <chrono>
#include <gst/gst.h>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>

#include "GStreamerHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(GstVideoReceiverLog, "Video.GStreamer.GstVideoReceiver")

namespace {
// Destructor wait for the worker. Must exceed the recording finalize timeout so a finalizing recording isn't
// abandoned.
constexpr std::chrono::seconds kWorkerShutdownTimeout = RecordingBranch::kFinalizeTimeout + std::chrono::seconds{2};
constexpr std::chrono::milliseconds kWatchdogInterval{1000};
// The live HW path counters are logged every this many watchdog ticks.
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

}  // namespace

GstVideoReceiver::GstVideoReceiver(QObject* parent)
    : VideoReceiver(parent)
    , _workerThread(new QThread(this))
    , _workerContext(new QObject())
    , _watchdogTimer(new QTimer(_workerContext))
    , _reconnectTimer(new QTimer(_workerContext))
    , _recordingBranch(*this, _workerContext)
    , _decodingBranch(*this)
    , _busRouter(*this)
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
        _busRouter.attach(_pipeline.get());

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

        _teeProbeId = gst_pad_add_probe(teeSink.get(), GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this, nullptr);
        if (_teeProbeId == 0) {
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

    _resetPipelineState();

    qCDebug(GstVideoReceiverLog) << "Stopped" << _redactedUri();

    if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(true); hwStats.totalDelivered > 0) {
        qCDebug(GstVideoReceiverLog).noquote()
            << "HW path stats" << _redactedUri() << hwStats.line + HwBuffers::takeExtraPathStats();
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
    _busRouter.detach();
    _pipelineGeneration.fetch_add(1, std::memory_order_relaxed);

    if (_teeProbeId != 0 && _tee) {
        if (const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_tee, "sink"))) {
            gst_pad_remove_probe(teeSink.get(), _teeProbeId);
        }
    }
    _teeProbeId = 0;

    _removeSourceEosProbe();

    _recordingBranch.reset();
    _decodingBranch.reset();

    _dotDumper.dump(_pipeline.get(), "pipeline-stopped");

    _pipeline.reset();

    // Owned by the (now released) pipeline.
    _tee = nullptr;
    _source = nullptr;

    _watchdog.stop();

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
    if (!_onWorkerThread()) {
        _runOnWorker([this]() { stopDecoding(); });
        return;
    }

    _decodingBranch.stop();
}

void GstVideoReceiver::startRecording(const QString &videoFile, FILE_FORMAT format)
{
    if (!_onWorkerThread()) {
        _runOnWorker([this, videoFile, format]() { startRecording(videoFile, format); });
        return;
    }

    _recordingBranch.start(videoFile, format);
}

void GstVideoReceiver::stopRecording()
{
    if (!_onWorkerThread()) {
        _runOnWorker([this]() { stopRecording(); });
        return;
    }

    _recordingBranch.stop();
}

void GstVideoReceiver::takeScreenshot(const QString& imageFile)
{
    // The widget's sink is the one attachAppSink() bound to the video sink, so its frame is the one on display.
    auto* const videoOutput = qobject_cast<QQuickVideoOutput*>(_widget);
    QVideoSink* const videoSink = videoOutput ? videoOutput->videoSink() : nullptr;
    if (!videoSink) {
        qCWarning(GstVideoReceiverLog) << "No video sink to take a screenshot from" << _name;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    // toImage() maps a GPU-backed frame back to the CPU.
    const QVideoFrame frame = videoSink->videoFrame();
    const QImage image = frame.isValid() ? frame.toImage() : QImage();
    if (image.isNull()) {
        qCWarning(GstVideoReceiverLog) << "No video frame to take a screenshot from" << _name;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    if (!image.save(imageFile)) {
        qCWarning(GstVideoReceiverLog) << "Failed to save screenshot" << imageFile;
        emit onTakeScreenshotComplete(STATUS_FAIL);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "Screenshot saved:" << imageFile;
    emit onTakeScreenshotComplete(STATUS_OK);
}

void GstVideoReceiver::_onWatchdogTick()
{
    if (!_pipeline) {
        return;
    }

    if (++_statsTickCounter >= kHwStatsLogIntervalTicks) {
        _statsTickCounter = 0;
        if (const HwBuffers::PathStats hwStats = HwBuffers::formatPathStats(false); hwStats.totalDelivered > 0) {
            qCDebug(GstVideoReceiverLog).noquote() << "HW path live" << _redactedUri() << hwStats.line;
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
    _removeSourceEosProbe();
    _eosProbeId = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _eosProbe, this, nullptr);
    if (_eosProbeId != 0) {
        _eosProbePad = GStreamer::adoptPad(GST_PAD(gst_object_ref(pad)));
    }

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

void GstVideoReceiver::_removeSourceEosProbe()
{
    if (_eosProbeId != 0 && _eosProbePad) {
        gst_pad_remove_probe(_eosProbePad.get(), _eosProbeId);
    }
    _eosProbeId = 0;
    _eosProbePad.reset();
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
