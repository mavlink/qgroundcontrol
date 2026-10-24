#include "RecordingBranch.h"

#include <gst/gst.h>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QTimer>

#include "GStreamerHelpers.h"
#include "PipelineDotDumper.h"

namespace {

// MP4/MOV robust muxing: qtmux reserves header space at the start of the file and rewrites the moov
// there every update period, so a crash or power loss leaves a file playable up to the last update.
// The reservation (550 B/s per track by default, ~8 MB here) outlasts the nominal duration since the
// index grows ~140 B/s at 30 fps and ~260 B/s at 60 fps; overrunning it makes qtmux post an ERROR.
// The period bounds crash data loss without rewriting the growing moov too often.
constexpr guint64 kRecordingReservedMaxDurationNs = 2 * 3600 * GST_SECOND;
constexpr guint64 kRecordingMoovUpdatePeriodNs = 5 * GST_SECOND;

const char* muxerFactory(VideoReceiver::FileFormat format)
{
    switch (format) {
        case VideoReceiver::FileFormat::Mkv:
            return "matroskamux";
        case VideoReceiver::FileFormat::Mov:
            return "qtmux";
        case VideoReceiver::FileFormat::Mp4:
            return "mp4mux";
    }
    return nullptr;
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

/// After packet loss the parsers can't place some frames and pass them on with only a DTS; qtmux rejects a buffer
/// without a PTS ("Could not multiplex stream.") and fails the whole recording. Live video carries no B-frames, so
/// such a frame's PTS is its DTS; one with neither timestamp is dropped.
GstPadProbeReturn timestampForMuxer([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, [[maybe_unused]] gpointer data)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer || GST_BUFFER_PTS_IS_VALID(buffer)) {
        return GST_PAD_PROBE_OK;
    }
    if (!GST_BUFFER_DTS_IS_VALID(buffer)) {
        return GST_PAD_PROBE_DROP;
    }
    buffer = gst_buffer_make_writable(buffer);
    GST_BUFFER_PTS(buffer) = GST_BUFFER_DTS(buffer);
    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    return GST_PAD_PROBE_OK;
}

/// The keyframe probe's data: which recording it gates.
struct KeyframeWatch
{
    RecordingBranch* branch = nullptr;
    quint64 recording = 0;
};

QString notFinalizedError()
{
    return QCoreApplication::translate("RecordingBranch",
                                       "The recording was not finalized in time and may be incomplete.");
}

}  // namespace

GStreamer::GstElementPtr RecordingBranch::makeFileSink(const QString& file, VideoReceiver::FileFormat format,
                                                       const GstCaps* inputCaps)
{
    const char* const muxerName = muxerFactory(format);
    if (!muxerName) {
        qCCritical(GstVideoReceiverLog) << "Unsupported file format" << static_cast<int>(format);
        return nullptr;
    }

    GstElement* const rawBin = gst_bin_new("sinkbin");
    GStreamer::GstElementPtr bin = GStreamer::adoptElement(rawBin ? GST_ELEMENT(gst_object_ref_sink(rawBin)) : nullptr);
    if (!bin) {
        qCWarning(GstVideoReceiverLog) << "gst_bin_new('sinkbin') failed";
        return nullptr;
    }

    GstElement* parser = nullptr;
    if (const char* parserFactory = recordingParserFactory(inputCaps)) {
        GStreamer::GstElementPtr parserOwner = GStreamer::makeElement(parserFactory, "parser");
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

    GStreamer::GstElementPtr muxerOwner = GStreamer::makeElement(muxerName, "muxer");
    GStreamer::GstElementPtr fileSinkOwner = GStreamer::makeElement("filesink", "filesink");
    if (!muxerOwner || !fileSinkOwner) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << muxerName << "or filesink";
        return nullptr;
    }

    // faststart stays off: it would spool the recording to a temp file and write the real file only at EOS.
    if ((format == VideoReceiver::FileFormat::Mp4) || (format == VideoReceiver::FileFormat::Mov)) {
        g_object_set(muxerOwner.get(), "reserved-max-duration", kRecordingReservedMaxDurationNs,
                     "reserved-moov-update-period", kRecordingMoovUpdatePeriodNs, nullptr);
    }
    // Not async: a sink that waits to preroll would take the live pipeline out of PLAYING until the first keyframe.
    g_object_set(fileSinkOwner.get(), "location", QFile::encodeName(file).constData(), "async", FALSE, nullptr);

    GstElement* const muxer = GStreamer::addToBin(bin.get(), std::move(muxerOwner), "muxer");
    GstElement* const fileSink = GStreamer::addToBin(bin.get(), std::move(fileSinkOwner), "filesink");
    if (!muxer || !fileSink || !gst_element_link(muxer, fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Failed to link the muxer and filesink";
        return nullptr;
    }

    // No release_request_pad on failure: disposing the bin reclaims the pad.
    const GStreamer::GstPadPtr muxerPad = GStreamer::adoptPad(gst_element_request_pad_simple(muxer, "video_%u"));
    if (!muxerPad) {
        qCWarning(GstVideoReceiverLog) << "gst_element_request_pad_simple(" << muxerName << ", video_%u) failed";
        return nullptr;
    }
    (void) gst_pad_add_probe(muxerPad.get(), GST_PAD_PROBE_TYPE_BUFFER, timestampForMuxer, nullptr, nullptr);

    GStreamer::GstPadPtr parserSinkPad;
    if (parser) {
        const GStreamer::GstPadPtr parserSrcPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "src"));
        parserSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "sink"));
        if (!parserSrcPad || !parserSinkPad) {
            qCCritical(GstVideoReceiverLog) << "gst_element_get_static_pad(parser) failed";
            return nullptr;
        }

        const GstPadLinkReturn linkResult = gst_pad_link(parserSrcPad.get(), muxerPad.get());
        if (linkResult != GST_PAD_LINK_OK) {
            qCWarning(GstVideoReceiverLog) << "Failed to link parser and muxer:" << gst_pad_link_get_name(linkResult);
            return nullptr;
        }
    }

    // Each element, not just the bin: a message can outlive the bin that held its source.
    for (GstElement* const element : {bin.get(), parser, muxer, fileSink}) {
        if (element) {
            GStreamer::markBranch(element, PipelineBranch::Recording);
        }
    }

    GstPad* const ghostpad = gst_ghost_pad_new("sink", parserSinkPad ? parserSinkPad.get() : muxerPad.get());
    if (!ghostpad || !gst_element_add_pad(bin.get(), ghostpad)) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the recording sink's ghost pad";
        return nullptr;
    }

    return bin;
}

RecordingBranch::RecordingBranch(const BranchContext& context, Listener& listener, QObject* timerParent)
    : _context(context)
    , _listener(listener)
    , _finalizeTimer(new QTimer(timerParent))
{
    _finalizeTimer->setSingleShot(true);
    _finalizeTimer->setInterval(kFinalizeTimeout);
    (void) QObject::connect(_finalizeTimer, &QTimer::timeout, timerParent, [this]() {
        if (_state != State::Finalizing) {
            return;
        }
        qCWarning(GstVideoReceiverLog) << "Recording not finalized within"
                                       << std::chrono::milliseconds(kFinalizeTimeout).count()
                                       << "ms; closing it (MP4/MOV stay playable up to the last moov update)";
        _finish(notFinalizedError());
    });
}

RecordingBranch::~RecordingBranch()
{
    reset();
    delete _finalizeTimer;
}

bool RecordingBranch::build()
{
    const GStreamer::TeeBranch branch = GStreamer::addTeeBranch(_context.pipeline, _context.tee, "recording");
    if (!branch.valve) {
        return false;
    }
    _queue = branch.queue;
    _valve = branch.valve;
    GStreamer::markBranch(_queue, PipelineBranch::Recording);
    GStreamer::markBranch(_valve, PipelineBranch::Recording);

    // Every frame must reach the muxer, so no time or buffer limit; storage that falls this far behind ends the
    // recording (_onQueueOverrun) instead of holding up the tee.
    g_object_set(_queue, "max-size-time", G_GUINT64_CONSTANT(0), "max-size-buffers", 0, "max-size-bytes",
                 static_cast<guint>(kQueueMaxBytes), nullptr);
    (void) _queueOverrun.connect(_queue, "overrun", G_CALLBACK(_onQueueOverrun), this);

    // Preserve stream-start/caps/segment events while buffers are dropped. When recording
    // starts later, the newly linked parser receives the negotiated codec configuration.
    // drop-mode exists since GStreamer 1.20, the oldest version QGC supports.
    gst_util_set_object_arg(G_OBJECT(_valve), "drop-mode", "forward-sticky-events");
    return true;
}

void RecordingBranch::reset()
{
    _finalizeTimer->stop();
    _queueOverrun.disconnect();
    _keyframeWatch.remove();
    if (_fileSink) {
        GStreamer::flushAndDisposeChild(_fileSink.get());
        _fileSink.reset();
    }
    (void) _finalizer.disarm();
    _state = State::Idle;
    _queue = nullptr;
    _valve = nullptr;
}

void RecordingBranch::setRequest(const std::optional<Request>& request)
{
    _requested = request;
    _reconcile();
}

void RecordingBranch::onSourceStreaming()
{
    _streaming = true;
    _reconcile();
}

void RecordingBranch::_reconcile()
{
    switch (_state) {
        case State::Idle:
            if (_requested && _streaming) {
                _start(*_requested);
            }
            break;
        case State::Recording:
            if (!_requested || (*_requested != _current)) {
                _end();
            }
            break;
        case State::Finalizing:
            break;
    }
}

void RecordingBranch::_start(const Request& request)
{
    _current = request;
    (void) _recording.fetch_add(1, std::memory_order_relaxed);
    _failure.clear();
    _fileStarted.store(false, std::memory_order_relaxed);
    _openedReported = false;
    _state = State::Recording;

    qCDebug(GstVideoReceiverLog) << "Starting recording" << request.file << _context.redactedUri;

    // forward-sticky-events keeps the negotiated caps on the closed valve; the query covers the moment before.
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "src"));
    GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(valveSrc.get()));
    if (!inputCaps) {
        inputCaps = GStreamer::adoptCaps(gst_pad_query_caps(valveSrc.get(), nullptr));
    }

    _fileSink = makeFileSink(request.file, request.format, inputCaps.get());
    if (!_fileSink) {
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to create the recording sink.");
        _finish(QString());
        return;
    }
    // Before the sink can post, so its ERROR (such as a file that can't be opened) settles this recording.
    _finalizer.arm(_fileSink.get());

    gst_util_set_object_arg(G_OBJECT(_queue), "leaky", "no");
    if (!gst_bin_add(GST_BIN(_context.pipeline), _fileSink.get()) || !gst_element_link(_valve, _fileSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the recording sink" << _context.redactedUri;
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to add the recording sink.");
        _finish(QString());
        return;
    }

    if (!gst_element_sync_state_with_parent(_fileSink.get())) {
        // The sink posted why (usually a file that can't be opened); that ERROR ends the recording.
        GStreamer::flushAndDisposeChild(_fileSink.get());
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to start recording to %1.").arg(request.file);
        _state = State::Finalizing;
        _finalizeTimer->start();
        return;
    }

    // The file starts with a keyframe at t=0, so playback can decode from its first frame.
    if (!_keyframeWatch.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, _keyframeProbe,
                            new KeyframeWatch{.branch = this, .recording = _recording.load(std::memory_order_relaxed)},
                            [](gpointer watch) { delete static_cast<KeyframeWatch*>(watch); })) {
        qCCritical(GstVideoReceiverLog) << "Failed to add the keyframe probe" << _context.redactedUri;
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to start recording.");
        _finish(QString());
        return;
    }

    g_object_set(_valve, "drop", FALSE, nullptr);
    const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "sink"));
    GStreamer::requestKeyframe(valveSink.get());
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-filesink");
}

void RecordingBranch::_end(const QString& failure)
{
    if (_state != State::Recording) {
        return;
    }
    if (!failure.isEmpty()) {
        _failure = failure;
    }

    g_object_set(_valve, "drop", TRUE, nullptr);
    _keyframeWatch.remove();

    if (!_fileStarted.load(std::memory_order_acquire)) {
        _finish(QString());
        return;
    }

    // Unlinked, so the next recording can take the valve while this one drains parser -> muxer -> file. Not sent from
    // here: the EOS waits for the recorder's streaming thread, which slow storage can hold up.
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_valve, "src"));
    const GStreamer::GstPadPtr recorderSink = GStreamer::adoptPad(gst_pad_get_peer(valveSrc.get()));
    if (!recorderSink || !gst_pad_unlink(valveSrc.get(), recorderSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Unable to unlink the recording sink";
        _finish(notFinalizedError());
        return;
    }
    GStreamer::sendEventAsync(recorderSink.get(), gst_event_new_eos());
    _state = State::Finalizing;
    _finalizeTimer->start();
}

void RecordingBranch::_finish(const QString& error)
{
    _finalizeTimer->stop();
    _keyframeWatch.remove();
    if (_valve) {
        g_object_set(_valve, "drop", TRUE, nullptr);
    }

    const RecordingFinalizer::Outcome outcome = _finalizer.disarm();
    if (_fileSink) {
        // Removed before going NULL, so nothing it posts while stopping reaches the bus. Flushed unless it closed its
        // file: going NULL would otherwise wait for a streaming thread stuck inside it.
        if (outcome == RecordingFinalizer::Outcome::Closed) {
            GStreamer::disposeChild(_fileSink.get());
        } else {
            GStreamer::flushAndDisposeChild(_fileSink.get());
        }
        _fileSink.reset();
    }
    _state = State::Idle;
    _context.dotDumper->dump(_context.pipeline, "pipeline-recording-stopped");

    const QString file = _current.file;
    const bool fileOpened = _fileStarted.load(std::memory_order_acquire);
    if (fileOpened) {
        _reportOpened();
        const QString fileError = error.isEmpty() ? _failure : error;
        qCDebug(GstVideoReceiverLog) << "Recording closed" << file << fileError;
        _listener.onRecordingFileClosed(file, fileError);
    } else if (QFile::exists(file) && !QFile::remove(file)) {
        // The filesink created the file, but it never got a frame.
        qCDebug(GstVideoReceiverLog) << "Unable to remove the empty recording" << file;
    }

    if (!_failure.isEmpty()) {
        if (_requested && (_requested->file == file)) {
            _requested.reset();
        }
        const QString failure = std::exchange(_failure, QString());
        _listener.onRecordingFailed(file, failure, fileOpened);
    }

    _reconcile();
}

bool RecordingBranch::beginTeardown()
{
    _requested.reset();
    if (_state != State::Recording) {
        return finalizing();
    }

    // Its recorder is done already, so there is nothing left to drain.
    if (_finalizer.outcome() != RecordingFinalizer::Outcome::Pending) {
        _finish(QString());
        return false;
    }

    if (!_fileStarted.load(std::memory_order_acquire)) {
        _end();
        return finalizing();
    }

    // EOS at the tee drains the recorder queue into the file. Not sent to the pipeline: a source handles that EOS under
    // its streaming thread's stream lock, which a thread held downstream would never release.
    const GStreamer::GstPadPtr teeSink = GStreamer::adoptPad(gst_element_get_static_pad(_context.tee, "sink"));
    GStreamer::sendEventAsync(teeSink.get(), gst_event_new_eos());
    _state = State::Finalizing;
    return true;
}

void RecordingBranch::completeTeardown()
{
    if (_state != State::Finalizing) {
        return;
    }

    if (_finalizer.outcome() == RecordingFinalizer::Outcome::Pending) {
        qCWarning(GstVideoReceiverLog) << "Recording not finalized within"
                                       << std::chrono::milliseconds(kFinalizeTimeout).count()
                                       << "ms; closing it (MP4/MOV stay playable up to the last moov update)";
        // Not serialized, so it doesn't wait for the stream lock: wakes the tee's thread if the full queue holds it,
        // and has the queue stop pushing into the recorder.
        const GStreamer::GstPadPtr queueSink = GStreamer::adoptPad(gst_element_get_static_pad(_queue, "sink"));
        (void) gst_pad_send_event(queueSink.get(), gst_event_new_flush_start());
        _finish(notFinalizedError());
        return;
    }
    _finish(QString());
}

void RecordingBranch::onBusError(GstMessage* message, const QString& text, const char* debug)
{
    bool fromRecorder = false;
    const bool settled =
        _finalizer.settleFrom(GST_MESSAGE_SRC(message), RecordingFinalizer::Outcome::Failed, &fromRecorder);
    if (!fromRecorder) {
        qCDebug(GstVideoReceiverLog) << "Ignoring an error from an earlier recording:" << text;
        return;
    }

    // The detail names the cause behind a generic text such as "Could not multiplex stream."
    if (debug) {
        qCWarning(GstVideoReceiverLog) << "Recording error:" << text << "-" << debug;
    } else {
        qCWarning(GstVideoReceiverLog) << "Recording error:" << text;
    }
    // An error after the file closed doesn't fail it; a recording still running ends either way.
    if (!settled && (_state != State::Recording)) {
        return;
    }

    _failure = text;
    // The bus handler closed the valve already: a failing recorder ends the recording, not the stream.
    _finish(QString());
}

void RecordingBranch::onBusElement(GstMessage* message)
{
    // The recorder's EOS as the pipeline forwards it (message-forward), ahead of the pipeline's own EOS. That one is
    // ignored: without a display sink it follows every recorder's EOS, and may arrive once the next recording started.
    const GstStructure* structure = gst_message_get_structure(message);
    if (!structure || !gst_structure_has_name(structure, "GstBinForwarded")) {
        return;
    }
    GstMessage* forwardedMessage = nullptr;
    (void) gst_structure_get(structure, "message", GST_TYPE_MESSAGE, &forwardedMessage, nullptr);
    const GStreamer::GstMessagePtr forwarded = GStreamer::adoptMessage(forwardedMessage);
    if (!forwarded || (GST_MESSAGE_TYPE(forwarded.get()) != GST_MESSAGE_EOS)) {
        return;
    }
    if (!_finalizer.settleFrom(GST_MESSAGE_SRC(forwarded.get()), RecordingFinalizer::Outcome::Closed)) {
        return;
    }
    if (_state == State::Recording) {
        // The stream's own EOS came through the open valve. Nothing more will stream through this pipeline, so the
        // request waits for the next one rather than start over in the file just closed.
        _streaming = false;
    }
    qCDebug(GstVideoReceiverLog) << "Recording finalized" << _current.file;
    _finish(QString());
}

void RecordingBranch::_onFileOpened(quint64 recording)
{
    if ((recording != _recording.load(std::memory_order_relaxed)) || (_state != State::Recording)) {
        return;
    }
    _reportOpened();
}

void RecordingBranch::_onOverrun(quint64 recording)
{
    if ((recording != _recording.load(std::memory_order_relaxed)) || (_state != State::Recording)) {
        return;
    }
    qCWarning(GstVideoReceiverLog) << "Recording storage too slow, its backlog is full; ending the recording"
                                   << _current.file;
    _end(QCoreApplication::translate("RecordingBranch", "The storage is too slow to keep up with the video."));
}

void RecordingBranch::_reportOpened()
{
    if (_openedReported) {
        return;
    }
    _openedReported = true;
    qCDebug(GstVideoReceiverLog) << "Recording started" << _current.file;
    _listener.onRecordingFileOpened(_current.file);
}

GstPadProbeReturn RecordingBranch::_keyframeProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    const auto* const watch = static_cast<const KeyframeWatch*>(data);
    RecordingBranch* const self = watch->branch;
    if (self->_fileStarted.load(std::memory_order_relaxed)) {
        return GST_PAD_PROBE_OK;
    }

    // Upstream of the tee is always a parser (GstBaseParse), which pushes single buffers, never lists.
    GstBuffer* buf = gst_pad_probe_info_get_buffer(info);
    if (!buf || GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT) || !GST_BUFFER_PTS_IS_VALID(buf)) {
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

    self->_fileStarted.store(true, std::memory_order_release);
    // Reported from the worker, which knows whether this recording is still the current one.
    const quint64 recording = watch->recording;
    self->_context.post([self, recording]() { self->_onFileOpened(recording); });
    return GST_PAD_PROBE_OK;
}

void RecordingBranch::_onQueueOverrun(GstElement* queue, gpointer data)
{
    auto* const self = static_cast<RecordingBranch*>(data);
    // Read back by the queue right after this handler: it now drops the frame instead of waiting for room, so the tee
    // (and the display behind it) never waits for slow storage.
    gst_util_set_object_arg(G_OBJECT(queue), "leaky", "upstream");
    const quint64 recording = self->_recording.load(std::memory_order_relaxed);
    self->_context.post([self, recording]() { self->_onOverrun(recording); });
}
