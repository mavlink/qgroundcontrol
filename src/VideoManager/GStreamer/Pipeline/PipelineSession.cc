#include "PipelineSession.h"

#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <utility>

#include <QtCore/QDeadlineTimer>

#include "GStreamerHelpers.h"
#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "HwBuffers/HwBuffers.h"
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
    const auto [error, debug] = GStreamer::parseMessageError(message);
    qCWarning(GstVideoReceiverLog) << "GStreamer warning:" << (error ? error->message : "(no message)")
                                   << "debug:" << (debug ? debug.get() : "(none)");
}

/// rtspsrc reports the steps of its connection (open, connect, request, close) as progress messages, and a stream
/// whose sender went quiet as a "GstRTSPSrcTimeout" element message: neither is an error, so only the log shows them.
void logSourceProgress(GstMessage* message)
{
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT) {
        if (gst_message_has_name(message, "GstRTSPSrcTimeout")) {
            gint stream = -1;
            (void) gst_structure_get_int(gst_message_get_structure(message), "stream-number", &stream);
            qCDebug(GstVideoReceiverLog) << "The sender of RTSP stream" << stream << "timed out";
        }
        return;
    }
    GstProgressType type = GST_PROGRESS_TYPE_START;
    gchar* rawCode = nullptr;
    gchar* rawText = nullptr;
    gst_message_parse_progress(message, &type, &rawCode, &rawText);
    const GStreamer::GCharPtr code = GStreamer::adoptGChar(rawCode);
    const GStreamer::GCharPtr text = GStreamer::adoptGChar(rawText);
    // The connect step's text is "Connecting to <address>", and the log shows that address redacted.
    const bool connect = (g_strcmp0(code.get(), "connect") == 0);
    qCDebug(GstVideoReceiverLog) << "Source progress:" << (code ? code.get() : "(none)")
                                 << (connect ? "Connecting" : (text ? text.get() : ""));
}

bool isRecoverableH265PaciError(GstMessage* msg, const GError* error, const gchar* debug)
{
    if (!msg || !error || (error->domain != GST_STREAM_ERROR) || (error->code != GST_STREAM_ERROR_FORMAT) || !debug ||
        !g_strrstr(debug, "NAL unit type 50 not supported yet")) {
        return false;
    }
    return g_strcmp0(GStreamer::factoryNameOf(GST_MESSAGE_SRC(msg)), "rtph265depay") == 0;
}

bool postedByRtspSrc(GstMessage* message)
{
    return g_strcmp0(GStreamer::factoryNameOf(GST_MESSAGE_SRC(message)), "rtspsrc") == 0;
}

/// The server refused the credentials (RTSP 401), or the stream needs some and has none.
bool isUnauthorizedError(const GError* error)
{
    return g_error_matches(error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NOT_AUTHORIZED);
}

/// Errors that a reconnect can't fix.
bool isPermanentError(GstMessage* message, const GError* error)
{
    if (g_error_matches(error, GST_STREAM_ERROR, GST_STREAM_ERROR_CODEC_NOT_FOUND)) {
        // rtspsrc's means it set up no stream at all, as when the server refused every transport: not a codec that
        // is missing, and the next attempt may get one.
        return !postedByRtspSrc(message);
    }
    return g_error_matches(error, GST_CORE_ERROR, GST_CORE_ERROR_MISSING_PLUGIN) || isUnauthorizedError(error);
}

/// What the element added, in the debug string, to the text of its error's code: all there is to tell a refused
/// connection from a missing file in "Could not open resource for reading.", the only text network elements post.
/// GStreamer puts "file(line): function (): /element/path:\n" in front of it. Empty when the element added nothing.
QString debugCause(const gchar* debug)
{
    const gchar* const separator = debug ? g_strstr_len(debug, -1, ":\n") : nullptr;
    return separator ? QString::fromUtf8(separator + 2).simplified() : QString();
}

/// The status of the response an rtspsrc error is about, which its details carry: "RTSP 404 Not Found". Empty for
/// other errors.
QString rtspStatus(GstMessage* message)
{
    const GstStructure* details = nullptr;
    gst_message_parse_error_details(message, &details);
    guint code = 0;
    if (!details || !gst_structure_get_uint(details, "rtsp-status-code", &code)) {
        return {};
    }
    const gchar* const reason = gst_structure_get_string(details, "rtsp-status-reason");
    return QStringLiteral("RTSP %1 %2").arg(QString::number(code), QString::fromUtf8(reason ? reason : "")).trimmed();
}

/// @p text, the text of an error's code, followed by what is known of the cause: @p missingPlugin when given, else the
/// RTSP status the error is about, else what the element added in @p debug.
QString describeError(GstMessage* message, const QString& text, const gchar* debug,
                      const QString& missingPlugin = QString())
{
    QString cause = missingPlugin;
    if (cause.isEmpty()) {
        cause = rtspStatus(message);
    }
    if (cause.isEmpty()) {
        cause = debugCause(debug);
    }
    // The cause can quote the far end (a server's reason phrase), and the message is shown where markup is rendered.
    cause.remove(QLatin1Char('<'));
    return cause.isEmpty() ? text : QStringLiteral("%1 (%2)").arg(text, cause);
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

std::optional<PipelineSession::Failure> PipelineSession::start()
{
    qCDebug(GstVideoReceiverLog) << "Starting" << _settings.redactedUri << "lowLatency" << _settings.lowLatency;

    _pipeline = GStreamer::adoptFloating(gst_pipeline_new(_dotDumper.pipelineName().constData()));
    // The sync handler is in place before anything can post: the context bridges must see NEED_CONTEXT, which upstream
    // queries during PAUSED->PLAYING.
    _bus.attach(
        _pipeline.get(), [this](GstMessage* message) { return _onBusSync(message); },
        [this](GstMessage* message) {
            if (_phase != Phase::Down) {
                _handleMessage(message);
            }
        });
    // Branch EOS reaches the bus wrapped in GstBinForwarded, so the recorder's own EOS can be told apart.
    g_object_set(_pipeline.get(), "message-forward", TRUE, nullptr);

    _tee = GStreamer::addToBin(_pipeline.get(), GStreamer::makeElement("tee"), "tee");
    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(_tee ? gst_element_get_static_pad(_tee, "sink") : nullptr);
    if (!teeSink) {
        return Failure{.reason = QStringLiteral("tee creation failed"),
                       .message = tr("Unable to create the video pipeline."),
                       .permanent = true};
    }
    // The probe feeds the watchdog.
    (void) _teeFrameProbe.add(teeSink.get(), GST_PAD_PROBE_TYPE_BUFFER, _teeProbe, this);

    const BranchContext context{
        .pipeline = _pipeline.get(),
        .tee = _tee,
        .redactedUri = _settings.redactedUri,
        .dotDumper = &_dotDumper,
        .watchdog = &_watchdog,
        .timings = _settings.timings,
        .post = [this](std::function<void()> task) { _post(std::move(task)); },
    };
    _decoding =
        std::make_unique<DecodingBranch>(context, _listener, _settings.lowLatency, _settings.softwareDecoderThreads);
    _recording = std::make_unique<RecordingBranch>(context, _listener);
    if (!_decoding->build() || !_recording->build()) {
        return Failure{.reason = QStringLiteral("branch creation failed"),
                       .message = tr("Unable to create the video pipeline."),
                       .permanent = true};
    }
    _displayValve = _decoding->valve();
    _recordingValve = _recording->valve();

    GStreamer::SourceFactory::Config sourceConfig;
    sourceConfig.jitterBuffer = _settings.lowLatency ? GStreamer::SourceFactory::JitterBuffer::None
                                                     : GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    sourceConfig.latencyMs = _settings.rtpJitterLatencyMs;
    sourceConfig.doRetransmission = (_settings.rtpJitterLatencyMs >= kMinRetransmissionLatencyMs) &&
                                    (sourceConfig.jitterBuffer != GStreamer::SourceFactory::JitterBuffer::None);
    sourceConfig.rtspTcpTimeout = _settings.timings.rtspTcpTimeout;
    // Floating: the pipeline takes it over.
    _source = GStreamer::SourceFactory::create(_settings.uri, sourceConfig);
    if (!_source || !gst_bin_add(GST_BIN(_pipeline.get()), _source)) {
        _source = nullptr;
        return Failure{.reason = QStringLiteral("no source for the URI"),
                       .message = tr("The video stream %1 is not supported, or a GStreamer plugin it needs is missing.")
                                      .arg(_settings.redactedUri),
                       .permanent = true};
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
        return Failure{.reason = QStringLiteral("start failure"),
                       .message = tr("Unable to start the video stream: %1").arg(error),
                       .permanent = false};
    }

    _dotDumper.dump(_pipeline.get(), "pipeline-started");
    qCDebug(GstVideoReceiverLog) << "Started" << _settings.redactedUri;
    return std::nullopt;
}

void PipelineSession::setVideoSink(const GstQgc::SinkBin* sink)
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

void PipelineSession::restartDecoder(const char* reason, const QString& cause)
{
    if (_decoding && !_tearingDown()) {
        _decoding->restartDecoder(reason, cause);
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
        // Handles the recorder's messages until it is finalized. The pump discards every other type, which
        // _handleMessage() ignores anyway while tearing down.
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
            if (gst_message_has_name(message, HwBuffers::kGraphicsDeviceServedMessage)) {
                if (!_tearingDown()) {
                    _decoding->onGraphicsDeviceServed();
                }
                break;
            }
            if (gst_message_has_name(message, HwBuffers::kGraphicsDeviceLostMessage)) {
                if (_phase == Phase::Running) {
                    qCDebug(GstVideoReceiverLog) << "The graphics device is gone" << _settings.redactedUri;
                    _post([this]() { _listener.onSessionGraphicsDeviceLost(); });
                }
                break;
            }
            _noteMissingPlugin(message);
            PipelineStatsProbe::noteElementMessage(message);
            _recording->onBusElement(message);
            logSourceProgress(message);
            break;
        case GST_MESSAGE_PROGRESS:
            logSourceProgress(message);
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
    const auto [error, debug] = GStreamer::parseMessageError(message);
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");

    if (isRecoverableH265PaciError(message, error.get(), debug.get())) {
        qCWarning(GstVideoReceiverLog) << "Ignoring unsupported H.265 RTP PACI packet from rtph265depay:" << text;
        return;
    }

    const GStreamer::PipelineBranch branch = GStreamer::branchOf(GST_MESSAGE_SRC(message));
    if (branch == GStreamer::PipelineBranch::Recording) {
        _recording->onBusError(message, error.get(), text, debug.get());
        return;
    }

    const char* const debugText = debug ? debug.get() : "(none)";
    if (_phase != Phase::Running) {
        qCDebug(GstVideoReceiverLog) << "Ignoring an error after the stream ended:" << text << "debug:" << debugText;
        return;
    }
    const bool namesMissingPlugin =
        g_error_matches(error.get(), GST_CORE_ERROR, GST_CORE_ERROR_MISSING_PLUGIN) && (branch == _missingPluginBranch);
    const QString description =
        describeError(message, text, debug.get(), namesMissingPlugin ? _missingPlugin : QString());

    if (branch == GStreamer::PipelineBranch::Decoding) {
        if (!_decoding->isCurrent(GST_MESSAGE_SRC(message))) {
            qCDebug(GstVideoReceiverLog) << "Ignoring an error from a replaced decoder:" << text
                                         << "debug:" << debugText;
            return;
        }
        qCWarning(GstVideoReceiverLog) << "Decoder error from" << messageSourcePath(message) << ":" << text
                                       << "debug:" << debugText;
        // Either way the stream goes on, for a recording if not for the display. A plugin one decoder misses, the
        // next one misses too.
        if (isPermanentError(message, error.get())) {
            _decoding->failDecoder(description);
        } else {
            _decoding->restartDecoder("decoder error", description);
        }
        return;
    }

    qCWarning(GstVideoReceiverLog) << "GStreamer error from" << messageSourcePath(message) << ":" << text
                                   << "debug:" << debugText;
    _dotDumper.snapshot(_pipeline.get(), QStringLiteral("pipeline-error"));
    const QString userMessage =
        isUnauthorizedError(error.get())
            ? tr("The video stream refused the connection: its user name or password is missing or wrong.")
            : tr("Video stream error: %1").arg(description);
    _fail({.reason = QStringLiteral("pipeline error"),
           .message = userMessage,
           .permanent = isPermanentError(message, error.get())});
}

QString PipelineSession::_takeStartError()
{
    // The state change posted why it failed; nothing else in the queue matters for a pipeline that never ran: pop()
    // discards what was queued before the error, and shutdown() flushes the rest.
    const GStreamer::GstMessagePtr message = _bus.pop(GST_MESSAGE_ERROR);
    if (!message) {
        return tr("unknown error");
    }
    const auto [error, debug] = GStreamer::parseMessageError(message.get());
    const QString text = QString::fromUtf8(error ? error->message : "(no message)");
    qCWarning(GstVideoReceiverLog) << "GStreamer error from" << messageSourcePath(message.get()) << ":" << text
                                   << "debug:" << (debug ? debug.get() : "(none)");
    return describeError(message.get(), text, debug.get());
}

void PipelineSession::_noteMissingPlugin(GstMessage* message)
{
    if (!gst_is_missing_plugin_message(message)) {
        return;
    }
    const GStreamer::GCharPtr description = GStreamer::adoptGChar(gst_missing_plugin_message_get_description(message));
    _missingPlugin = QString::fromUtf8(description ? description.get() : "");
    _missingPluginBranch = GStreamer::branchOf(GST_MESSAGE_SRC(message));
    qCWarning(GstVideoReceiverLog) << "Missing GStreamer plugin:" << _missingPlugin << "for"
                                   << messageSourcePath(message);
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

    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        // Closed now, before the failing element returns its flow error: the valve reports OK for a push that fails
        // after drop was set, which keeps the error out of the tee and so out of the other branch.
        const GStreamer::PipelineBranch branch = GStreamer::branchOf(GST_MESSAGE_SRC(message));
        GstElement* const valve = (branch == GStreamer::PipelineBranch::Decoding)
                                      ? _displayValve
                                      : ((branch == GStreamer::PipelineBranch::Recording) ? _recordingValve : nullptr);
        if (valve) {
            g_object_set(valve, "drop", TRUE, nullptr);
        }
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
