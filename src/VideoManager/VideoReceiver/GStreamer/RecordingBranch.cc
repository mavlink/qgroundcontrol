#include "RecordingBranch.h"

#include <gst/gst.h>
#include <utility>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QTimer>

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"

namespace {

// MP4/MOV robust muxing: qtmux reserves header space at the start of the file and rewrites the moov
// there every update period, so a crash or power loss leaves a file playable up to the last update.
// The reservation (550 B/s per track by default, ~8 MB here) outlasts the nominal duration since the
// index grows ~140 B/s at 30 fps and ~260 B/s at 60 fps; overrunning it makes qtmux post an ERROR.
// The period bounds crash data loss without rewriting the growing moov too often.
constexpr guint64 kRecordingReservedMaxDurationNs = 2 * 3600 * GST_SECOND;
constexpr guint64 kRecordingMoovUpdatePeriodNs = 5 * GST_SECOND;

constexpr const char* kFileMux[VideoReceiver::FILE_FORMAT_MAX + 1] = {
    "matroskamux",
    "qtmux",
    "mp4mux",
};

/// Per-recording state for the keyframe gate, owned by its probe.
struct KeyframeWatch
{
    VideoReceiver* receiver = nullptr;
    QString file;
    bool started = false;  ///< Only the probe touches it, on the recorder's streaming thread.
};

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

GStreamer::GstElementPtr RecordingBranch::makeFileSink(const QString& videoFile, VideoReceiver::FILE_FORMAT format,
                                                       const GstCaps* inputCaps)
{
    if (!VideoReceiver::isValidFileFormat(format)) {
        qCCritical(GstVideoReceiverLog) << "Unsupported file format";
        return nullptr;
    }

    GStreamer::GstElementPtr bin = GStreamer::adoptElement(gst_bin_new("sinkbin"));
    if (!bin) {
        qCWarning(GstVideoReceiverLog) << "gst_bin_new('sinkbin') failed";
        return nullptr;
    }
    (void) gst_object_ref_sink(bin.get());
    // splitmuxsink completes its change to PAUSED only once the first fragment opens, at the next keyframe.
    // Waited out inside the bin, that doesn't make the live pipeline lose PLAYING meanwhile, nor leave it
    // stuck in PAUSED when the sink fails first.
    g_object_set(bin.get(), "async-handling", TRUE, nullptr);

    GstElement* parser = nullptr;
    if (const char* parserFactory = recordingParserFactory(inputCaps)) {
        GStreamer::GstElementPtr parserOwner =
            GStreamer::adoptElement(gst_element_factory_make(parserFactory, nullptr));
        if (!parserOwner) {
            qCWarning(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << parserFactory;
            return nullptr;
        }
        g_object_set(parserOwner.get(), "config-interval", -1, nullptr);
        parser = GStreamer::addToBin(bin.get(), std::move(parserOwner), "parser");
        if (!parser) {
            return nullptr;
        }
    }

    GStreamer::GstElementPtr splitmuxOwner = GStreamer::adoptElement(gst_element_factory_make("splitmuxsink", nullptr));
    if (!splitmuxOwner) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make('splitmuxsink') failed";
        return nullptr;
    }
    GstElement* const splitmux = splitmuxOwner.get();

    // One file per recording: no size or time limit.
    g_object_set(splitmux, "location", qPrintable(videoFile), "muxer-factory", kFileMux[format], "max-size-time",
                 G_GUINT64_CONSTANT(0), "max-size-bytes", G_GUINT64_CONSTANT(0), "async-finalize", TRUE,
                 // Forwards the inner file sink's EOS, which settles the recording like fragment-closed does.
                 "message-forward", TRUE, nullptr);

    // faststart stays off: it would spool the recording to a temp file and write the real file only at EOS.
    if (format == VideoReceiver::FILE_FORMAT_MP4 || format == VideoReceiver::FILE_FORMAT_MOV) {
        const GStreamer::GstStructurePtr muxerProps = GStreamer::adoptStructure(
            gst_structure_new("properties", "reserved-max-duration", G_TYPE_UINT64, kRecordingReservedMaxDurationNs,
                              "reserved-moov-update-period", G_TYPE_UINT64, kRecordingMoovUpdatePeriodNs, nullptr));
        g_object_set(splitmux, "muxer-properties", muxerProps.get(), nullptr);
    }

    if (!GStreamer::addToBin(bin.get(), std::move(splitmuxOwner), "splitmuxsink")) {
        return nullptr;
    }

    // No release_request_pad on failure: finalizing splitmuxsink with the bin reclaims the pad.
    const GStreamer::GstPadPtr videopad = GStreamer::adoptPad(gst_element_request_pad_simple(splitmux, "video"));
    if (!videopad) {
        qCWarning(GstVideoReceiverLog) << "gst_element_request_pad_simple(splitmuxsink, video) failed";
        return nullptr;
    }

    GStreamer::GstPadPtr parserSinkPad;
    if (parser) {
        const GStreamer::GstPadPtr parserSrcPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "src"));
        parserSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "sink"));
        if (!parserSrcPad || !parserSinkPad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad(parser) failed";
            return nullptr;
        }

        const GstPadLinkReturn linkResult = gst_pad_link(parserSrcPad.get(), videopad.get());
        if (linkResult != GST_PAD_LINK_OK) {
            qCWarning(GstVideoReceiverLog)
                << "Failed to link parser and splitmuxsink:" << gst_pad_link_get_name(linkResult);
            return nullptr;
        }
    }

    GstPad* const ghostpad = gst_ghost_pad_new("sink", parserSinkPad ? parserSinkPad.get() : videopad.get());
    if (!ghostpad) {
        qCWarning(GstVideoReceiverLog) << "gst_ghost_pad_new() failed";
        return nullptr;
    }
    if (!gst_element_add_pad(bin.get(), ghostpad)) {
        qCWarning(GstVideoReceiverLog) << "gst_element_add_pad(ghost) failed";
        return nullptr;
    }

    return bin;
}

namespace {

GstPadProbeReturn keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    if (!info || !user_data) {
        qCCritical(GstVideoReceiverLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    auto* const watch = static_cast<KeyframeWatch*>(user_data);
    if (watch->started) {
        return GST_PAD_PROBE_OK;
    }

    // Upstream of the tee is always a parser (GstBaseParse), which pushes single buffers, never lists.
    GstBuffer* buf = gst_pad_probe_info_get_buffer(info);
    if (!buf || GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT) || !GST_BUFFER_PTS_IS_VALID(buf)) {
        // wait for a timestamped keyframe
        return GST_PAD_PROBE_DROP;
    }

    // The offset applies to running time, which differs from the PTS when the segment doesn't start at 0.
    GstClockTime runningTime = GST_CLOCK_TIME_NONE;
    if (const GStreamer::GstEventPtr segmentEvent =
            GStreamer::adoptEvent(gst_pad_get_sticky_event(pad, GST_EVENT_SEGMENT, 0))) {
        const GstSegment* segment = nullptr;
        gst_event_parse_segment(segmentEvent.get(), &segment);
        if (segment && (segment->format == GST_FORMAT_TIME)) {
            runningTime = gst_segment_to_running_time(segment, GST_FORMAT_TIME, GST_BUFFER_PTS(buf));
        }
    }
    if (!GST_CLOCK_TIME_IS_VALID(runningTime)) {
        return GST_PAD_PROBE_DROP;
    }

    // Starts the file at 0; only the elements downstream of the valve see the offset.
    gst_pad_set_offset(pad, -static_cast<gint64>(runningTime));

    qCDebug(GstVideoReceiverLog) << "Got keyframe, stop dropping buffers";

    watch->started = true;
    emit watch->receiver->recordingStarted(watch->file);
    return GST_PAD_PROBE_OK;
}

}  // namespace

RecordingBranch::RecordingBranch(GstVideoReceiver& receiver, QObject* workerContext)
    : _receiver(receiver)
    , _stopTimer(new QTimer(workerContext))
{
    _stopTimer->setSingleShot(true);
    _stopTimer->setInterval(kFinalizeTimeout);
    (void) QObject::connect(_stopTimer, &QTimer::timeout, workerContext, [this]() { _onStopTimeout(); });
}

bool RecordingBranch::build(GstElement* pipeline, GstElement* tee)
{
    _pipeline = pipeline;
    _tee = tee;

    const GStreamer::TeeBranch branch = GStreamer::addTeeBranch(_pipeline, _tee, "recording");
    if (!branch.valve) {
        return false;
    }
    _valve = branch.valve;
    // Preserve stream-start/caps/segment events while buffers are dropped. When recording
    // starts later, the newly linked parser receives the negotiated codec configuration.
    // drop-mode exists since GStreamer 1.20, the oldest version QGC supports.
    gst_util_set_object_arg(G_OBJECT(_valve), "drop-mode", "forward-sticky-events");
    return true;
}

void RecordingBranch::reset()
{
    _stopTimer->stop();
    if (_fileSink) {
        _shutdown();
    }
    // Owned by the pipeline.
    _valve = nullptr;
    _tee = nullptr;
    _pipeline = nullptr;
}

void RecordingBranch::start(const QString& videoFile, VideoReceiver::FILE_FORMAT format)
{
    qCDebug(GstVideoReceiverLog) << "Starting recording" << _receiver._redactedUri();

    if (!_pipeline) {
        qCDebug(GstVideoReceiverLog) << "Streaming is not active!" << _receiver._redactedUri();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_INVALID_STATE);
        return;
    }

    if (_state != State::Idle) {
        qCDebug(GstVideoReceiverLog) << "Already recording!" << _receiver._redactedUri();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_INVALID_STATE);
        return;
    }

    qCDebug(GstVideoReceiverLog) << "New video file:" << videoFile << _receiver._redactedUri();

    const GStreamer::GstPadPtr probepad = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "src"));
    if (!probepad) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed" << _receiver._redactedUri();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_FAIL);
        return;
    }

    const auto failRecordingStart = [this]() {
        if (_fileSink) {
            GStreamer::disposeChild(_fileSink.get());
            _fileSink.reset();
        }
        // Only once the sink is out of the pipeline: until then its errors must still count as the recorder's.
        (void) _finalizer.disarm();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_FAIL);
    };

    // forward-sticky-events preserves the exact negotiated caps while the valve is closed. Fall
    // back to a proxied caps query during the narrow startup window before negotiation completes.
    GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(probepad.get()));
    if (!inputCaps) {
        inputCaps = GStreamer::adoptCaps(gst_pad_query_caps(probepad.get(), nullptr));
    }
    _fileSink = makeFileSink(videoFile, format, inputCaps.get());
    if (!_fileSink) {
        qCWarning(GstVideoReceiverLog) << "Failed to create the recording sink" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    // Armed before the sink can post, so the bus handler attributes its messages (e.g. a failure to open the
    // file) to this recording instead of to the pipeline.
    _finalizer.arm(_fileSink.get());

    // The bin takes its own ref; ours survives a failed add.
    if (!gst_bin_add(GST_BIN(_pipeline), _fileSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the recording sink" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    if (!gst_element_link(_valve, _fileSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Failed to link valve and file sink" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    if (!gst_element_sync_state_with_parent(_fileSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Recording sink failed to start" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-filesink");

    _receiver._recordingOutput = videoFile;
    // The file starts with a keyframe at t=0, so playback can decode from its first frame.
    if (!_keyframeWatch.add(probepad.get(), GST_PAD_PROBE_TYPE_BUFFER, keyframeWatch,
                            new KeyframeWatch{.receiver = &_receiver, .file = videoFile},
                            [](gpointer watch) { delete static_cast<KeyframeWatch*>(watch); })) {
        qCCritical(GstVideoReceiverLog) << "Failed to add the keyframe probe" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    g_object_set(_valve, "drop", FALSE, nullptr);

    _state = State::Recording;
    qCDebug(GstVideoReceiverLog) << "Recording started" << _receiver._redactedUri();
    emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_OK);
    emit _receiver.recordingChanged(true);
}

void RecordingBranch::stop()
{
    qCDebug(GstVideoReceiverLog) << "Stopping recording" << _receiver._redactedUri();

    if (!_pipeline || (_state != State::Recording)) {
        qCDebug(GstVideoReceiverLog) << "Not recording!" << _receiver._redactedUri();
        emit _receiver.onStopRecordingComplete(VideoReceiver::STATUS_INVALID_STATE);
        return;
    }

    g_object_set(_valve, "drop", TRUE, nullptr);

    // The EOS drains parser -> muxer -> file; _onFinalized() completes the stop once the file is closed, the
    // timer if that is never reported.
    _state = State::Stopping;
    _stopTimer->start();
    if (!_unlinkBranch() || (_finalizer.outcome() != RecordingFinalizer::Outcome::Pending)) {
        _shutdown();
    }
}

void RecordingBranch::finishForTeardown()
{
    if (_state == State::Recording) {
        // EOS at the tee drains the recorder queue into the file. Not sent to the pipeline: a source handles that
        // EOS under its streaming thread's stream lock, which a thread held downstream would never release.
        if (const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_tee, "sink"))) {
            (void) gst_pad_send_event(teeSink.get(), gst_event_new_eos());
        }
    }

    // Also waits for a stop() still in flight; its EOS is already in the branch.
    if (_state != State::Idle) {
        switch (_finalizer.wait(QDeadlineTimer(kFinalizeTimeout))) {
            case RecordingFinalizer::Outcome::Closed:
                qCDebug(GstVideoReceiverLog) << "Recording finalized";
                break;
            case RecordingFinalizer::Outcome::Failed:
                qCWarning(GstVideoReceiverLog) << "Recording failed while finalizing";
                break;
            case RecordingFinalizer::Outcome::Pending:
                qCWarning(GstVideoReceiverLog)
                    << "Recording not finalized within" << std::chrono::milliseconds(kFinalizeTimeout).count()
                    << "ms — forcing pipeline NULL (recording may be truncated; "
                    << "MP4/MOV stay playable up to the last moov update)";
                break;
        }
    }
}

void RecordingBranch::onBusEos()
{
    // Every branch drained, the recorder included: the fallback for a missed fragment-closed.
    if (_finalizer.settle(RecordingFinalizer::Outcome::Closed)) {
        _receiver._runOnWorkerForPipeline([this]() { _onFinalized(); });
    }
}

void RecordingBranch::onBusElement(GstMessage* message)
{
    const GstStructure* structure = gst_message_get_structure(message);
    if (!structure) {
        return;
    }

    // splitmuxsink posts fragment-closed once the muxer has written the file and passed EOS to its sink.
    GstObject* closedSource = nullptr;
    if (gst_structure_has_name(structure, "splitmuxsink-fragment-closed")) {
        closedSource = GST_MESSAGE_SRC(message);
    }

    // Branch EOS (pipeline message-forward); only the recorder's needs handling.
    GStreamer::GstMessagePtr forwarded;
    if (gst_structure_has_name(structure, "GstBinForwarded")) {
        GstMessage* forwardedMessage = nullptr;
        (void) gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forwardedMessage, nullptr);
        forwarded = GStreamer::adoptMessage(forwardedMessage);
        if (forwarded && (GST_MESSAGE_TYPE(forwarded.get()) == GST_MESSAGE_EOS)) {
            closedSource = GST_MESSAGE_SRC(forwarded.get());
        }
    }

    if (closedSource && _finalizer.settleFrom(closedSource, RecordingFinalizer::Outcome::Closed)) {
        _receiver._runOnWorkerForPipeline([this]() { _onFinalized(); });
    }
}

bool RecordingBranch::onBusError(GstMessage* message, const QString& text)
{
    bool fromRecording = false;
    const bool settled =
        _finalizer.settleFrom(GST_MESSAGE_SRC(message), RecordingFinalizer::Outcome::Failed, &fromRecording);
    if (!fromRecording) {
        return false;
    }

    // A recorder failure (disk full, header reservation overrun) ends the recording, not the stream. Closing the
    // valve now, before the failing element returns its flow error, keeps that error out of the tee: the valve
    // reports OK for a push that fails after drop was set.
    qCWarning(GstVideoReceiverLog) << "Recording error:" << text;
    g_object_set(_valve, "drop", TRUE, nullptr);
    if (settled) {
        _receiver._runOnWorkerForPipeline([this]() { _onRecorderError(); });
    }
    return true;
}

bool RecordingBranch::_unlinkBranch()
{
    const GStreamer::GstPadPtr src = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "src"));
    if (!src) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    const GStreamer::GstPadPtr sink = GStreamer::adoptPad(gst_pad_get_peer(src.get()));
    if (!sink) {
        qCWarning(GstVideoReceiverLog) << "Branch is not linked";
        return false;
    }

    if (!gst_pad_unlink(src.get(), sink.get())) {
        qCWarning(GstVideoReceiverLog) << "gst_pad_unlink() failed";
        return false;
    }

    // Send EOS at the beginning of the branch
    const gboolean ret = gst_pad_send_event(sink.get(), gst_event_new_eos());

    if (!ret) {
        qCWarning(GstVideoReceiverLog) << "Branch EOS was NOT sent";
        return false;
    }

    qCDebug(GstVideoReceiverLog) << "Branch EOS was sent";

    return true;
}

void RecordingBranch::_shutdown()
{
    _stopTimer->stop();

    _keyframeWatch.remove();

    if (_fileSink) {
        // Removed before going NULL, so nothing it posts while stopping reaches the bus as a pipeline error.
        GStreamer::disposeChild(_fileSink.get());
        _fileSink.reset();
    }
    const RecordingFinalizer::Outcome outcome = _finalizer.disarm();

    const State state = std::exchange(_state, State::Idle);
    if (state != State::Idle) {
        qCDebug(GstVideoReceiverLog) << "Recording stopped";
        emit _receiver.recordingChanged(false);
    }
    if (state == State::Stopping) {
        emit _receiver.onStopRecordingComplete(
            (outcome == RecordingFinalizer::Outcome::Closed) ? VideoReceiver::STATUS_OK : VideoReceiver::STATUS_FAIL);
    }

    _receiver._dotDumper.dump(_pipeline, "pipeline-recording-stopped");
}

void RecordingBranch::_onFinalized()
{
    // Recording: the source ended and the file closed with it; finishForTeardown() collects it. Idle: already handled.
    if (_state != State::Stopping) {
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Recording finalized" << _receiver._redactedUri();
    _shutdown();
}

void RecordingBranch::_onRecorderError()
{
    // Idle: start() already reported the failure.
    if (_state == State::Idle) {
        return;
    }
    _shutdown();
}

void RecordingBranch::_onStopTimeout()
{
    if (_state != State::Stopping) {
        return;
    }
    qCWarning(GstVideoReceiverLog) << "Recording not finalized within"
                                   << std::chrono::milliseconds(kFinalizeTimeout).count()
                                   << "ms; closing it (MP4/MOV stay playable up to the last moov update)";
    _shutdown();
}
