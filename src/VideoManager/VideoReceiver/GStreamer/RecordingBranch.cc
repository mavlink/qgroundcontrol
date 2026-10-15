#include "RecordingBranch.h"

#include <gst/gst.h>
#include <utility>

#include <QtCore/QDeadlineTimer>
#include <QtCore/QTimer>

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
    std::atomic<gulong>* watchId = nullptr;
    QString file;
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

GstElement* RecordingBranch::makeFileSink(const QString& videoFile, VideoReceiver::FILE_FORMAT format,
                                          const GstCaps* inputCaps)
{
    GstElement* fileSink = nullptr;
    GstElement* splitmux = nullptr;
    GstElement* parser = nullptr;
    GstElement* bin = nullptr;
    GstPad* videopad = nullptr;
    GstPad* parserSrcPad = nullptr;
    GstPad* parserSinkPad = nullptr;
    GstPad* ghostpad = nullptr;

    do {
        if (!VideoReceiver::isValidFileFormat(format)) {
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

        g_object_set(splitmux, "location", qPrintable(videoFile), "muxer-factory", kFileMux[format], "max-size-time",
                     G_GUINT64_CONSTANT(0), "max-size-bytes", G_GUINT64_CONSTANT(0), "async-finalize", TRUE,
                     // Surface "splitmuxsink-fragment-closed" element messages on the pipeline bus so
                     // stop() can wait on the precise per-fragment finalize signal instead of EOS
                     // (gstsplitmuxsink.c:send_fragment_opened_closed_msg posts this per fragment,
                     // including the final fragment torn down on EOS).
                     "message-forward", TRUE, nullptr);

        // Crash-safe MP4/MOV via robust muxing. faststart is deliberately off: it spools the whole
        // recording to a temp file and writes the real file only at EOS. matroskamux is naturally
        // streamable; skip the GstStructure dance.
        if (format == VideoReceiver::FILE_FORMAT_MP4 || format == VideoReceiver::FILE_FORMAT_MOV) {
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
        // splitmuxsink completes its change to PAUSED only once the first fragment opens, at the next keyframe.
        // Waited out inside the bin, that doesn't make the live pipeline lose PLAYING meanwhile, nor leave it
        // stuck in PAUSED when the sink fails first.
        g_object_set(bin, "async-handling", TRUE, nullptr);

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
    } while (0);

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

namespace {

GstPadProbeReturn keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    if (!info || !user_data) {
        qCCritical(GstVideoReceiverLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    // Upstream of the tee is always a parser (GstBaseParse), which pushes single buffers, never lists.
    GstBuffer* buf = gst_pad_probe_info_get_buffer(info);
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

    // set media file '0' offset to current timeline position - we don't want to touch other elements in the graph,
    // except these which are downstream!
    gst_pad_set_offset(pad, -static_cast<gint64>(runningTime));

    qCDebug(GstVideoReceiverLog) << "Got keyframe, stop dropping buffers";

    const auto* watch = static_cast<const KeyframeWatch*>(user_data);
    emit watch->receiver->recordingStarted(watch->file);

    // Returning REMOVE drops the probe; forget its id so RecordingBranch::_shutdown() doesn't remove it again.
    gulong ownId = GST_PAD_PROBE_INFO_ID(info);
    (void) watch->watchId->compare_exchange_strong(ownId, 0);
    return GST_PAD_PROBE_REMOVE;
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

    GstElement* queue = gst_element_factory_make("queue", nullptr);
    if (!queue) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('queue') failed";
        return false;
    }
    (void) gst_bin_add(GST_BIN(_pipeline), queue);

    _valve = gst_element_factory_make("valve", nullptr);
    if (!_valve) {
        qCCritical(GstVideoReceiverLog) << "gst_element_factory_make('valve') failed";
        return false;
    }
    g_object_set(_valve, "drop", TRUE, nullptr);
    // Preserve stream-start/caps/segment events while buffers are dropped. When recording
    // starts later, the newly linked parser receives the negotiated codec configuration.
    gst_util_set_object_arg(G_OBJECT(_valve), "drop-mode", "forward-sticky-events");
    (void) gst_bin_add(GST_BIN(_pipeline), _valve);

    if (!gst_element_link_many(_tee, queue, _valve, nullptr)) {
        qCCritical(GstVideoReceiverLog) << "Unable to link recorder queue";
        return false;
    }
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

    GstPad* probepad = gst_element_get_static_pad(_valve, "src");
    if (!probepad) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed" << _receiver._redactedUri();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_FAIL);
        return;
    }

    const auto failRecordingStart = [this, &probepad]() {
        gst_clear_object(&probepad);
        if (_fileSink) {
            (void) gst_element_set_state(_fileSink, GST_STATE_NULL);
            // Safe before and after a successful link. Removing a still-linked bin leaves the
            // valve's src pad attached to a finalized peer and breaks every later retry.
            gst_element_unlink(_valve, _fileSink);
            GstObject* parent = gst_element_get_parent(_fileSink);
            if (parent) {
                (void) gst_bin_remove(GST_BIN(parent), _fileSink);
                gst_clear_object(&parent);
            }
            gst_clear_object(&_fileSink);
        }
        // Only once the sink is out of the pipeline: until then its errors must still count as the recorder's.
        (void) _finalizer.disarm();
        emit _receiver.onStartRecordingComplete(VideoReceiver::STATUS_FAIL);
    };

    // forward-sticky-events preserves the exact negotiated caps while the valve is closed. Fall
    // back to a proxied caps query during the narrow startup window before negotiation completes.
    GstCaps* inputCaps = gst_pad_get_current_caps(probepad);
    if (!inputCaps) {
        inputCaps = gst_pad_query_caps(probepad, nullptr);
    }
    _fileSink = makeFileSink(videoFile, format, inputCaps);
    gst_clear_caps(&inputCaps);
    if (!_fileSink) {
        qCCritical(GstVideoReceiverLog) << "makeFileSink() failed" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    // Armed before the sink can post, so the bus handler attributes its messages (e.g. a failure to open the
    // file) to this recording instead of to the pipeline.
    _finalizer.arm(_fileSink);

    if (!gst_bin_add(GST_BIN(_pipeline), _fileSink)) {
        qCCritical(GstVideoReceiverLog) << "gst_bin_add(file sink) failed" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }
    (void) gst_object_ref(_fileSink);  // Keep a reference in addition to the pipeline's ownership.

    if (!gst_element_link(_valve, _fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Failed to link valve and file sink" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    if (!gst_element_sync_state_with_parent(_fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Recording sink failed to start" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }

    _receiver._dotDumper.dump(_pipeline, "pipeline-with-filesink");

    // Install a probe on the recording branch to drop buffers until we hit our first keyframe
    // When we hit our first keyframe, we can offset the timestamps appropriately according to the first keyframe time
    // This will ensure the first frame is a keyframe at t=0, and decoding can begin immediately on playback
    _receiver._recordingOutput = videoFile;
    _keyframeWatchId = gst_pad_add_probe(probepad, GST_PAD_PROBE_TYPE_BUFFER, keyframeWatch,
                                         new KeyframeWatch{&_receiver, &_keyframeWatchId, videoFile},
                                         [](gpointer watch) { delete static_cast<KeyframeWatch*>(watch); });
    if (_keyframeWatchId == 0) {
        qCCritical(GstVideoReceiverLog) << "gst_pad_add_probe(keyframeWatch) failed" << _receiver._redactedUri();
        failRecordingStart();
        return;
    }
    gst_clear_object(&probepad);

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
        if (GstPad* teeSink = gst_element_get_static_pad(_tee, "sink")) {
            (void) gst_pad_send_event(teeSink, gst_event_new_eos());
            gst_object_unref(teeSink);
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
    GstMessage* forwarded = nullptr;
    if (gst_structure_has_name(structure, "GstBinForwarded")) {
        gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forwarded, nullptr);
        if (forwarded && (GST_MESSAGE_TYPE(forwarded) == GST_MESSAGE_EOS)) {
            closedSource = GST_MESSAGE_SRC(forwarded);
        }
    }

    if (closedSource && _finalizer.settleFrom(closedSource, RecordingFinalizer::Outcome::Closed)) {
        _receiver._runOnWorkerForPipeline([this]() { _onFinalized(); });
    }
    gst_clear_message(&forwarded);
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
    GstPad* src = gst_element_get_static_pad(_valve, "src");
    if (!src) {
        qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad() failed";
        return false;
    }

    GstPad* sink = gst_pad_get_peer(src);
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

void RecordingBranch::_shutdown()
{
    _stopTimer->stop();

    const gulong keyframeWatchId = _keyframeWatchId.exchange(0);
    if (keyframeWatchId != 0 && _valve) {
        GstPad* probepad = gst_element_get_static_pad(_valve, "src");
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
