#include "PipelineSession.h"

#include <gst/gst.h>
#include <utility>

#include <QtCore/QDeadlineTimer>

#include "GStreamerHelpers.h"
#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/common/HwBuffers.h"
#include "PipelineDotDumper.h"
#include "StreamWatchdog.h"
#include "VideoStreamStats.h"
#include "gstqgc/GstQgcSinkStats.h"

namespace {

// Source flow is logged every this many frames.
constexpr quint64 kSourceFlowLogIntervalFrames = 300;
// Retransmission needs playout latency left over after the source factory's 25 ms rtx-delay; with less, recovered
// packets arrive too late and only add load.
constexpr int kMinRetransmissionLatencyMs = 40;

void logBusWarning(GstMessage* message)
{
    // GStreamer posts WARNING for caps mismatches, decoder fallbacks, clock drift: surfacing keeps these visible
    // without failing the stream.
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

/// Errors that a reconnect can't fix.
bool isPermanentError(const GError* error)
{
    return g_error_matches(error, GST_CORE_ERROR, GST_CORE_ERROR_MISSING_PLUGIN) ||
           g_error_matches(error, GST_STREAM_ERROR, GST_STREAM_ERROR_CODEC_NOT_FOUND);
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

QString messageSourcePath(GstMessage* message)
{
    const GStreamer::GCharPtr path = GStreamer::adoptGChar(
        GST_MESSAGE_SRC(message) ? gst_object_get_path_string(GST_MESSAGE_SRC(message)) : nullptr);
    return QString::fromUtf8(path ? path.get() : "(unknown)");
}

}  // namespace

PipelineSession::PipelineSession(const Settings& settings, Listener& listener, StreamWatchdog& watchdog,
                                 const PipelineDotDumper& dotDumper, QObject* parent)
    : QObject(parent)
    , _settings(settings)
    , _listener(listener)
    , _watchdog(watchdog)
    , _dotDumper(dotDumper)
{}

PipelineSession::~PipelineSession()
{
    shutdown();
}

bool PipelineSession::start()
{
    qCDebug(GstVideoReceiverLog) << "Starting" << _settings.redactedUri << "lowLatency" << _settings.lowLatency;

    GstElement* const pipeline = gst_pipeline_new(_dotDumper.pipelineName().constData());
    _pipeline = GStreamer::adoptElement(pipeline ? GST_ELEMENT(gst_object_ref_sink(pipeline)) : nullptr);
    // The sync handler is in place before anything can post: the context bridges must see NEED_CONTEXT, which upstream
    // queries during PAUSED->PLAYING.
    if (!_pipeline || !_bus.attach(
                          _pipeline.get(), [this](GstMessage* message) { return _onBusSync(message); },
                          [this](GstMessage* message) {
                              if (_phase != Phase::Down) {
                                  _handleMessage(message);
                              }
                          })) {
        _failure = {.reason = QStringLiteral("pipeline creation failed"),
                    .message = tr("Unable to create the video pipeline."),
                    .permanent = true};
        return false;
    }
    // Branch EOS reaches the bus wrapped in GstBinForwarded, so the recorder's own EOS can be told apart.
    g_object_set(_pipeline.get(), "message-forward", TRUE, nullptr);

    _tee = GStreamer::addToBin(_pipeline.get(), GStreamer::makeElement("tee"), "tee");
    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(_tee ? gst_element_get_static_pad(_tee, "sink") : nullptr);
    // The probe feeds the watchdog; without it the watchdog fires spuriously instead of reporting a real failure.
    if (!teeSink || !_teeFrameProbe.add(teeSink.get(), GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this)) {
        _failure = {.reason = QStringLiteral("tee creation failed"),
                    .message = tr("Unable to create the video pipeline."),
                    .permanent = true};
        return false;
    }

    const BranchContext context{
        .pipeline = _pipeline.get(),
        .tee = _tee,
        .redactedUri = _settings.redactedUri,
        .dotDumper = &_dotDumper,
        .watchdog = &_watchdog,
        .post = [this](std::function<void()> task) { _post(std::move(task)); },
    };
    _decoding = std::make_unique<DecodingBranch>(context, _listener, _settings.lowLatency);
    _recording = std::make_unique<RecordingBranch>(context, _listener);
    if (!_decoding->build() || !_recording->build()) {
        _failure = {.reason = QStringLiteral("branch creation failed"),
                    .message = tr("Unable to create the video pipeline."),
                    .permanent = true};
        return false;
    }
    _displayValve = _decoding->valve();
    _recordingValve = _recording->valve();

    GStreamer::SourceFactory::Config sourceConfig;
    sourceConfig.jitterBuffer = _settings.lowLatency ? GStreamer::SourceFactory::JitterBuffer::None
                                                     : GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    sourceConfig.latencyMs = _settings.rtpJitterLatencyMs;
    sourceConfig.doRetransmission = (_settings.rtpJitterLatencyMs >= kMinRetransmissionLatencyMs) &&
                                    (sourceConfig.jitterBuffer != GStreamer::SourceFactory::JitterBuffer::None);
    // Floating: the pipeline takes it over.
    _source = GStreamer::SourceFactory::create(_settings.uri, sourceConfig);
    if (!_source || !gst_bin_add(GST_BIN(_pipeline.get()), _source)) {
        _source = nullptr;
        _failure = {.reason = QStringLiteral("no source for the URI"),
                    .message = tr("The video stream %1 is not supported, or a GStreamer plugin it needs is missing.")
                                   .arg(_settings.redactedUri),
                    .permanent = true};
        return false;
    }

    if (const GStreamer::GstPadPtr srcPad = GStreamer::firstSrcPad(_source)) {
        _linkSourcePad(srcPad.get());
    } else {
        (void) _sourcePadAdded.connect(_source, "pad-added", G_CALLBACK(_onSourcePadAdded), this);
    }

    _dotDumper.dump(_pipeline.get(), "pipeline-initial");
    if (gst_element_set_state(_pipeline.get(), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        const QString error = _takeStartError();
        qCWarning(GstVideoReceiverLog) << "Failed to start" << _settings.redactedUri;
        _failure = {.reason = QStringLiteral("start failure"),
                    .message = tr("Unable to start the video stream: %1").arg(error),
                    .permanent = false};
        return false;
    }

    _dotDumper.dump(_pipeline.get(), "pipeline-started");
    qCDebug(GstVideoReceiverLog) << "Started" << _settings.redactedUri;
    return true;
}

void PipelineSession::setVideoSink(GstElement* sink)
{
    if (_decoding && !_tearingDown()) {
        _decoding->setVideoSink(sink);
    }
}

void PipelineSession::setRecording(const std::optional<RecordingBranch::Request>& request)
{
    if (_recording && !_tearingDown()) {
        _recording->setRequest(request);
    }
}

void PipelineSession::restartDecoder(const char* reason)
{
    if (_decoding && !_tearingDown()) {
        _decoding->restartDecoder(reason);
    }
}

bool PipelineSession::displaying() const
{
    return _decoding && _decoding->displaying();
}

GstQgc::PathStats PipelineSession::sinkPathStats() const
{
    return _decoding ? _decoding->sinkPathStats() : GstQgc::PathStats{};
}

void PipelineSession::sampleStats(VideoStreamStatsMeter::Counters& counters, VideoStreamStats& stats)
{
    if (_decoding) {
        const DecodingBranch::StatsSample display = _decoding->sampleStats();
        counters.framesDisplayed = display.framesDisplayed;
        counters.framesDropped = display.framesDropped;
        stats.gpuPath = display.displayPath;
        stats.decoder = _decoding->decoderName();
        stats.hardwareDecoder = _decoding->hardwareDecoder();
        stats.gpuFallbacks = static_cast<qint64>(display.gpuFallbacks);
        stats.decoderRestarts = static_cast<qint64>(display.decoderRestarts);
    }

    _statsProbe.sample(_pipeline.get(), _source, _tee, counters, stats);
}

void PipelineSession::snapshot(const QString& tag) const
{
    _dotDumper.snapshot(_pipeline.get(), tag);
}

void PipelineSession::shutdown()
{
    if (_phase == Phase::Down) {
        return;
    }
    _phase = Phase::TearingDown;

    if (_recording && _pipeline && _recording->beginTeardown()) {
        // Handles the recorder's messages until it is finalized.
        _bus.pumpUntil(
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_ELEMENT),
            [this]() { return !_recording->finalizing(); }, QDeadlineTimer(RecordingBranch::kFinalizeTimeout));
        _recording->completeTeardown();
    }
    // From here on, work queued by streaming threads is stale.
    _phase = Phase::Down;

    const GstQgc::PathStats hwStats = sinkPathStats();

    if (_pipeline) {
        // Synchronous: the streaming threads have stopped when this returns.
        (void) gst_element_set_state(_pipeline.get(), GST_STATE_NULL);
    }
    _bus.detach();

    _teeFrameProbe.remove();
    _sourceEos.remove();
    _sourcePadAdded.disconnect();
    if (_decoding) {
        _decoding->reset();
    }
    if (_recording) {
        _recording->reset();
    }

    if (hwStats.totalDelivered > 0) {
        qCDebug(GstVideoReceiverLog).noquote() << "GPU path stats" << _settings.redactedUri << hwStats.line;
    }

    if (_pipeline) {
        _dotDumper.dump(_pipeline.get(), "pipeline-stopped");
        qCDebug(GstVideoReceiverLog) << "Stopped" << _settings.redactedUri;
    }
    _pipeline.reset();
    _tee = nullptr;
    _source = nullptr;
}

void PipelineSession::_post(std::function<void()> task)
{
    (void) QMetaObject::invokeMethod(
        this,
        [this, task = std::move(task)]() {
            if (_phase != Phase::Down) {
                task();
            }
        },
        Qt::QueuedConnection);
}

void PipelineSession::_fail(const Failure& failure)
{
    if (_phase != Phase::Running) {
        return;
    }
    _phase = Phase::Failed;
    _failure = failure;
    _post([this, failure]() { _listener.onSessionFailed(failure); });
}

void PipelineSession::_handleMessage(GstMessage* message)
{
    if (!_decoding || !_recording) {
        return;
    }
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR:
            _onBusError(message);
            break;
        case GST_MESSAGE_WARNING:
            if (!_tearingDown()) {
                logBusWarning(message);
            }
            break;
        case GST_MESSAGE_ELEMENT:
            _recording->onBusElement(message);
            break;
        case GST_MESSAGE_STATE_CHANGED:
            if (!_tearingDown() && pipelineEnteredPlaying(message)) {
                _logLatency();
            }
            break;
        case GST_MESSAGE_LATENCY:
            // Recalculation hands the sinks their new latency in a LATENCY event.
            if (!_tearingDown()) {
                (void) gst_bin_recalculate_latency(GST_BIN(_pipeline.get()));
            }
            break;
        default:
            break;
    }
}

void PipelineSession::_onBusError(GstMessage* message)
{
    GError* rawError = nullptr;
    gchar* rawDebug = nullptr;
    gst_message_parse_error(message, &rawError, &rawDebug);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const GStreamer::GCharPtr debug = GStreamer::adoptGChar(rawDebug);
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");

    if (isRecoverableH265PaciError(message, error.get(), debug.get())) {
        qCWarning(GstVideoReceiverLog) << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << text;
        return;
    }

    const GStreamer::PipelineBranch branch = GStreamer::branchOf(GST_MESSAGE_SRC(message));
    if (branch == GStreamer::PipelineBranch::Recording) {
        _recording->onBusError(message, text, debug.get());
        return;
    }

    if (debug) {
        qCDebug(GstVideoReceiverLog) << "GStreamer debug:" << debug.get();
    }
    if (_phase != Phase::Running) {
        qCDebug(GstVideoReceiverLog) << "Ignoring an error after the stream ended:" << text;
        return;
    }

    if (branch == GStreamer::PipelineBranch::Decoding) {
        if (!_decoding->isCurrent(GST_MESSAGE_SRC(message))) {
            qCDebug(GstVideoReceiverLog) << "Ignoring an error from a replaced decoder:" << text;
            return;
        }
        qCWarning(GstVideoReceiverLog) << "Decoder error from" << messageSourcePath(message) << ":" << text;
        HwBuffers::onPipelineError(error.get(), debug.get());
        if (isPermanentError(error.get())) {
            _fail({.reason = QStringLiteral("decoder error"),
                   .message = tr("Unable to decode the video: %1").arg(text),
                   .permanent = true});
        } else {
            _decoding->restartDecoder("decoder error");
        }
        return;
    }

    qCWarning(GstVideoReceiverLog) << "GStreamer error from" << messageSourcePath(message) << ":" << text;
    HwBuffers::onPipelineError(error.get(), debug.get());
    _dotDumper.snapshot(_pipeline.get(), QStringLiteral("pipeline-error"));
    _fail({.reason = QStringLiteral("pipeline error"),
           .message = tr("Video stream error: %1").arg(text),
           .permanent = isPermanentError(error.get())});
}

QString PipelineSession::_takeStartError()
{
    // The state change posted why it failed; nothing else in the queue matters for a pipeline that never ran.
    const GStreamer::GstMessagePtr message = _bus.pop(GST_MESSAGE_ERROR);
    if (!message) {
        return tr("unknown error");
    }
    GError* rawError = nullptr;
    gst_message_parse_error(message.get(), &rawError, nullptr);
    const GStreamer::GErrorPtr error = GStreamer::adoptError(rawError);
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");
    qCWarning(GstVideoReceiverLog) << "GStreamer error from" << messageSourcePath(message.get()) << ":" << text;
    return text;
}

void PipelineSession::_linkSourcePad(GstPad* pad)
{
    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_tee, "sink"));
    const GstPadLinkReturn linkResult = teeSink ? gst_pad_link(pad, teeSink.get()) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkResult != GST_PAD_LINK_OK) {
        qCWarning(GstVideoReceiverLog) << "Unable to link source:" << gst_pad_link_get_name(linkResult);
        return;
    }
    // Streaming is reported from the first frame, not from here: a static source pad exists before any data does.
    (void) _sourceEos.add(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _sourceEosProbe, this);
    _dotDumper.dump(_pipeline.get(), "pipeline-with-new-source-pad");
}

void PipelineSession::_onFirstFrame()
{
    if (_streaming) {
        return;
    }
    _streaming = true;
    qCDebug(GstVideoReceiverLog) << "Streaming started" << _settings.redactedUri;
    _decoding->onSourceStreaming();
    _recording->onSourceStreaming();
    _listener.onSessionStreaming();
}

void PipelineSession::_logLatency()
{
    GstClockTime min = 0, max = 0;
    (void) PipelineStatsProbe::queryLatency(_pipeline.get(), min, max);
    qCDebug(GstVideoReceiverLog).noquote()
        << "Pipeline PLAYING:" << _settings.redactedUri << "min-latency:" << GST_TIME_AS_MSECONDS(min) << "ms"
        << "max-latency:" << GST_TIME_AS_MSECONDS(max) << "ms";
}

GstBusSyncReply PipelineSession::_onBusSync(GstMessage* message)
{
    // The context bridges answer NEED_CONTEXT synchronously and consume (unref) the requests they handle.
    if (HwBuffers::onBusSyncMessage(message) == GST_BUS_DROP) {
        return GST_BUS_DROP;
    }

    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_STREAM_COLLECTION:
            selectVideoStreams(message);
            gst_message_unref(message);
            return GST_BUS_DROP;
        case GST_MESSAGE_ERROR:
            // Closed now, before the failing element returns its flow error: the valve reports OK for a push that
            // fails after drop was set, which keeps the error out of the tee and so out of the other branch.
            switch (GStreamer::branchOf(GST_MESSAGE_SRC(message))) {
                case GStreamer::PipelineBranch::Decoding:
                    if (_displayValve) {
                        g_object_set(_displayValve, "drop", TRUE, nullptr);
                    }
                    break;
                case GStreamer::PipelineBranch::Recording:
                    if (_recordingValve) {
                        g_object_set(_recordingValve, "drop", TRUE, nullptr);
                    }
                    break;
                case GStreamer::PipelineBranch::None:
                    break;
            }
            break;
        default:
            break;
    }
    return GST_BUS_PASS;
}

void PipelineSession::_onSourcePadAdded(GstElement* source, GstPad* pad, gpointer data)
{
    auto* const self = static_cast<PipelineSession*>(data);

    // Runs on a streaming thread; the pipeline is only changed on the worker.
    self->_post(GStreamer::holdPadUntilLinked(pad, [self, padSource = GStreamer::shareObject(source)](GstPad* newPad) {
        if (padSource.get() == self->_source) {
            self->_linkSourcePad(newPad);
        }
    }));
}

GstPadProbeReturn PipelineSession::_teeProbe(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer data)
{
    auto* const self = static_cast<PipelineSession*>(data);
    if (GstBuffer* const buffer = GST_PAD_PROBE_INFO_BUFFER(info)) {
        self->_statsProbe.noteBuffer(buffer);
    }
    const quint64 frames = self->_watchdog.noteSourceFrame(StreamWatchdog::now());
    if (frames == 1) {
        self->_post([self]() { self->_onFirstFrame(); });
    } else if ((frames % kSourceFlowLogIntervalFrames) == 0) {
        qCDebug(GstVideoReceiverLog).noquote() << "Source flow: teeFrames=" << frames << self->_settings.redactedUri;
    }
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn PipelineSession::_sourceEosProbe(GstPad* /*pad*/, GstPadProbeInfo* info, gpointer data)
{
    const GstEvent* const event = gst_pad_probe_info_get_event(info);
    if (event && (GST_EVENT_TYPE(event) == GST_EVENT_EOS)) {
        auto* const self = static_cast<PipelineSession*>(data);
        self->_post([self]() {
            self->_fail({.reason = QStringLiteral("end of stream"),
                         .message = tr("The video stream ended."),
                         .permanent = false});
        });
    }
    return GST_PAD_PROBE_OK;
}
