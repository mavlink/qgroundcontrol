#include "RecordingBranch.h"

#include <cerrno>
#include <cstring>
#include <gst/gst.h>
#include <utility>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

#include "GStreamerHelpers.h"
#include "GstVideoReceiver.h"
#include "PipelineDotDumper.h"
#include "PipelineStatsProbe.h"
#include "RecordingPlan.h"

namespace {

// MP4/MOV robust muxing: qtmux reserves header space at the start of the file and rewrites the moov
// there every update period, so a crash or power loss leaves a file playable up to the last update.
// The reservation (550 B/s per track by default, ~8 MB here) outlasts the nominal duration since the
// index grows ~140 B/s at 30 fps and ~260 B/s at 60 fps; overrunning it makes qtmux post an ERROR.
// No file is meant to get that far: its recording continues in a new part after half the nominal duration. The
// index grows faster at higher frame rates, and when no two frames last the same, as without a jitter buffer, so
// the muxer's own estimate of the room left is watched too (RecordingBranch::kHeaderSpaceMargin).
// The period bounds crash data loss without rewriting the growing moov too often.
constexpr guint64 kRecordingReservedMaxDurationNs = 2 * 3600 * GST_SECOND;
constexpr guint64 kRecordingMoovUpdatePeriodNs = 5 * GST_SECOND;
constexpr std::chrono::seconds kRecordingWatchInterval{1};
// matroskamux never has its file synced, as qtmux has with each header update. Storage that takes writes into a cache
// (an Android SD card, a network share) reports a full disk only to a sync: a recording there would lose everything
// without an error. A sync as often as the MP4's also bounds what a power loss takes.
constexpr gint64 kRecordingSyncPeriodUs = static_cast<gint64>(kRecordingMoovUpdatePeriodNs / GST_USECOND);
static_assert(std::chrono::nanoseconds(GstReceiverTimings{}.recordingHeaderEstimateDelay).count() >=
                  static_cast<qint64>(2 * kRecordingMoovUpdatePeriodNs),
              "The muxer's estimate of its header space counts from its second header update");
static_assert((2 * std::chrono::nanoseconds(RecordingPlan::kMaxPartDuration).count()) <=
                  static_cast<qint64>(kRecordingReservedMaxDurationNs),
              "A recording's part must end well within the header space its muxer reserves");

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

/// Whether @p caps tell what a stream is.
bool describesStream(const GstCaps* caps)
{
    return caps && !gst_caps_is_any(caps) && !gst_caps_is_empty(caps);
}

/// The parser a recording puts ahead of its muxer for a codec: it brings the stream into the layout the muxer asks
/// for (its stream-format and alignment), whichever the stream arrives in.
struct RecordingParser
{
    const char* mediaType;
    const char* factory;
};

const RecordingParser* recordingParser(const GstCaps* caps)
{
    static constexpr RecordingParser kParsers[] = {{"video/x-h264", "h264parse"}, {"video/x-h265", "h265parse"}};
    if (!describesStream(caps)) {
        return nullptr;
    }

    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstStructure* structure = gst_caps_get_structure(caps, i);
        for (const RecordingParser& parser : kParsers) {
            if (gst_structure_has_name(structure, parser.mediaType)) {
                return &parser;
            }
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

/// Has filesink sync the file after the first buffer and after one every kRecordingSyncPeriodUs. @p data: when the
/// last one was marked, on the monotonic clock.
GstPadProbeReturn syncPeriodically([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* const markedAt = static_cast<gint64*>(data);
    const gint64 now = g_get_monotonic_time();
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer || ((*markedAt != 0) && ((now - *markedAt) < kRecordingSyncPeriodUs))) {
        return GST_PAD_PROBE_OK;
    }
    *markedAt = now;
    buffer = gst_buffer_make_writable(buffer);
    GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_SYNC_AFTER);
    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    return GST_PAD_PROBE_OK;
}

/// Has filesink sync an MKV once more when matroskamux has finalized it, as qtmux asks with the last thing it writes:
/// without that, a power cut takes the file's end with its index and duration, and a disk that was full for it goes
/// unreported. Asked with a buffer ahead of the EOS, which follows the muxer's last write. filesink writes out what
/// it holds back, and then syncs, only for a buffer that has a memory; an empty one adds nothing to the file. A sync
/// that fails is filesink's error, as while recording.
GstPadProbeReturn syncAtEnd(GstPad* pad, GstPadProbeInfo* info, [[maybe_unused]] gpointer data)
{
    const GstEvent* const event = gst_pad_probe_info_get_event(info);
    if (!event || (GST_EVENT_TYPE(event) != GST_EVENT_EOS)) {
        return GST_PAD_PROBE_OK;
    }
    GstBuffer* const buffer = gst_buffer_new();
    gst_buffer_append_memory(buffer, gst_allocator_alloc(nullptr, 0, nullptr));
    GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_SYNC_AFTER);
    // From the thread that brings the EOS, which holds the pad's stream lock for it: the lock is recursive.
    (void) gst_pad_chain(pad, buffer);
    return GST_PAD_PROBE_OK;
}

/// The custom downstream event that marks, in the recording's queue, where a stopped recording ends.
constexpr char kEndMark[] = "qgc-recording-end";

bool isEndMark(GstEvent* event)
{
    return event && (GST_EVENT_TYPE(event) == GST_EVENT_CUSTOM_DOWNSTREAM) && gst_event_has_name(event, kEndMark);
}

/// The data of a probe that reports to its recording: which one that is.
struct RecordingWatch
{
    RecordingBranch* branch = nullptr;
    quint64 recording = 0;
};

/// Blocks the CAPS event that sets the caps passed as @p data, until the probe is removed.
GstPadProbeReturn holdCapsEvent([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstEvent* const event = gst_pad_probe_info_get_event(info);
    GstCaps* caps = nullptr;
    if (event && (GST_EVENT_TYPE(event) == GST_EVENT_CAPS)) {
        gst_event_parse_caps(event, &caps);
    }
    return (caps && gst_caps_is_equal(caps, static_cast<const GstCaps*>(data))) ? GST_PAD_PROBE_OK : GST_PAD_PROBE_PASS;
}

/// The switch probe's data: which hold it is.
struct SwitchWatch
{
    RecordingBranch* branch = nullptr;
    quint64 hold = 0;
};

/// Whether a recorder's bus error is a full disk. filesink names it by the error's code when a write fails, and only
/// by errno's text in the detail when the fsync after one of the muxer's header updates does.
bool reportsFullStorage(const GError* error, const char* debug)
{
    if (!error) {
        return false;
    }
    if (g_error_matches(error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_NO_SPACE_LEFT)) {
        return true;
    }
    return g_error_matches(error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE) && debug &&
           (std::strstr(debug, g_strerror(ENOSPC)) != nullptr);
}

QString notFinalizedError()
{
    return QCoreApplication::translate("RecordingBranch",
                                       "The recording was not finalized in time and may be incomplete.");
}

void warnNotFinalized()
{
    qCWarning(GstVideoReceiverLog) << "Recording not finalized within"
                                   << std::chrono::milliseconds(RecordingBranch::kFinalizeTimeout).count()
                                   << "ms; closing it (MP4/MOV stay playable up to the last moov update)";
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

    GStreamer::GstElementPtr bin = GStreamer::adoptFloating(gst_bin_new("sinkbin"));
    if (!bin) {
        qCWarning(GstVideoReceiverLog) << "gst_bin_new('sinkbin') failed";
        return nullptr;
    }

    GstElement* parser = nullptr;
    if (const RecordingParser* const recorderParser = recordingParser(inputCaps)) {
        GStreamer::GstElementPtr parserOwner = GStreamer::makeElement(recorderParser->factory, "parser");
        if (!parserOwner) {
            qCWarning(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << recorderParser->factory;
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

    // Tagged with the time it starts and the application, as a USB camera's recordings are (UVCCameraController).
    if (GST_IS_TAG_SETTER(muxerOwner.get())) {
        GstDateTime* const started = gst_date_time_new_from_g_date_time(g_date_time_new_now_utc());
        gst_tag_setter_add_tags(GST_TAG_SETTER(muxerOwner.get()), GST_TAG_MERGE_REPLACE, GST_TAG_DATE_TIME, started,
                                GST_TAG_COMMENT, VideoReceiver::recordingComment().toUtf8().constData(), nullptr);
        gst_date_time_unref(started);
    }
    // faststart stays off: it would spool the recording to a temp file and write the real file only at EOS.
    if ((format == VideoReceiver::FileFormat::Mp4) || (format == VideoReceiver::FileFormat::Mov)) {
        g_object_set(muxerOwner.get(), "reserved-max-duration", kRecordingReservedMaxDurationNs,
                     "reserved-moov-update-period", kRecordingMoovUpdatePeriodNs, nullptr);
    }
    // Not async: a sink that waits to preroll would take the live pipeline out of PLAYING until the first keyframe.
    g_object_set(fileSinkOwner.get(), "location", GStreamer::glibFileName(file).constData(), "async", FALSE, nullptr);

    GstElement* const muxer = GStreamer::addToBin(bin.get(), std::move(muxerOwner), "muxer");
    GstElement* const fileSink = GStreamer::addToBin(bin.get(), std::move(fileSinkOwner), "filesink");
    if (!muxer || !fileSink || !gst_element_link(muxer, fileSink)) {
        qCWarning(GstVideoReceiverLog) << "Failed to link the muxer and filesink";
        return nullptr;
    }
    if (format == VideoReceiver::FileFormat::Mkv) {
        const GStreamer::GstPadPtr fileSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(fileSink, "sink"));
        (void) gst_pad_add_probe(fileSinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, syncPeriodically, g_new0(gint64, 1),
                                 g_free);
        (void) gst_pad_add_probe(fileSinkPad.get(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, syncAtEnd, nullptr, nullptr);
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
            GStreamer::markBranch(element, GStreamer::PipelineBranch::Recording);
        }
    }

    GstPad* const ghostpad = gst_ghost_pad_new("sink", parserSinkPad ? parserSinkPad.get() : muxerPad.get());
    if (!ghostpad || !gst_element_add_pad(bin.get(), ghostpad)) {
        qCWarning(GstVideoReceiverLog) << "Failed to add the recording sink's ghost pad";
        return nullptr;
    }

    return bin;
}

std::optional<VideoReceiver::FileFormat> RecordingBranch::containerFor(VideoReceiver::FileFormat format,
                                                                       const GstCaps* inputCaps)
{
    if (!describesStream(inputCaps)) {
        return format;
    }
    // Of a stream the recorder parses, only the codec counts: the parser gives the muxer the layout it asks for.
    const RecordingParser* const parser = recordingParser(inputCaps);
    const GStreamer::GstCapsPtr codec =
        GStreamer::adoptCaps(parser ? gst_caps_new_empty_simple(parser->mediaType) : nullptr);
    const GstCaps* const muxed = codec ? codec.get() : inputCaps;

    for (const VideoReceiver::FileFormat container : {format, VideoReceiver::FileFormat::Mkv}) {
        const char* const muxerName = muxerFactory(container);
        const GStreamer::GstFactoryPtr muxer =
            GStreamer::adoptFactory(muxerName ? gst_element_factory_find(muxerName) : nullptr);
        if (muxer && gst_element_factory_can_sink_any_caps(muxer.get(), muxed)) {
            return container;
        }
    }
    return std::nullopt;
}

RecordingBranch::RecordingBranch(const BranchContext& context, Listener& listener)
    : _context(context)
    , _listener(listener)
{
    _finalizeTimer.setSingleShot(true);
    _finalizeTimer.setInterval(kFinalizeTimeout);
    (void) QObject::connect(&_finalizeTimer, &QTimer::timeout, &_finalizeTimer, [this]() {
        if ((_state == State::Finalizing) || (_state == State::Draining)) {
            warnNotFinalized();
            _finish(notFinalizedError());
        }
    });
    _watchTimer.setInterval(kRecordingWatchInterval);
    (void) QObject::connect(&_watchTimer, &QTimer::timeout, &_watchTimer, [this]() {
        _checkStarted();
        _checkHeaderSpace();
    });
}

RecordingBranch::~RecordingBranch()
{
    reset();
}

bool RecordingBranch::build()
{
    if (!_head.build(_context.pipeline, _context.tee, GStreamer::PipelineBranch::Recording, "recording")) {
        return false;
    }

    // Every frame must reach the muxer, so no time or buffer limit; storage that falls this far behind ends the
    // recording (_onQueueOverrun) instead of holding up the tee.
    g_object_set(_head.queue(), "max-size-time", G_GUINT64_CONSTANT(0), "max-size-buffers", 0, "max-size-bytes",
                 static_cast<guint>(kQueueMaxBytes), nullptr);
    _head.onOverrun(G_CALLBACK(_onQueueOverrun), this);

    // Preserve stream-start/caps/segment events while buffers are dropped. When recording
    // starts later, the newly linked parser receives the negotiated codec configuration.
    // drop-mode exists since GStreamer 1.20, the oldest version QGC supports.
    gst_util_set_object_arg(G_OBJECT(_head.valve()), "drop-mode", "forward-sticky-events");
    return true;
}

void RecordingBranch::reset()
{
    _finalizeTimer.stop();
    _watchTimer.stop();
    _keyframeWatch.remove();
    _startWatch.remove();
    _capsWatch.remove();
    _drainWatch.remove();
    _releaseSwitchHold();
    if (_fileSink) {
        GStreamer::flushAndDisposeChild(_fileSink.get());
        _fileSink.reset();
    }
    _state = State::Idle;
    _head.reset();
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
            if (_requested && (*_requested == _current)) {
                _releaseSwitchHold();
            } else if (_requested && _fileStarted.load(std::memory_order_acquire)) {
                // A file with frames ends where the next one can begin, so the frames in between aren't lost: at the
                // change of caps the stream is held at, or else at its next keyframe.
                if (_capsHold.load(std::memory_order_acquire) != 0) {
                    _end();
                } else {
                    _switchAtKeyframe();
                }
            } else if (!_requested && _fileStarted.load(std::memory_order_acquire)) {
                _drain();
            } else {
                _end();
            }
            break;
        case State::Draining:
        case State::Finalizing:
            break;
    }
}

void RecordingBranch::_start(Request request)
{
    // forward-sticky-events keeps the negotiated caps on the closed valve; the query covers the moment before.
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "src"));
    GStreamer::GstCapsPtr inputCaps = GStreamer::adoptCaps(gst_pad_get_current_caps(valveSrc.get()));
    if (!inputCaps) {
        inputCaps = GStreamer::adoptCaps(gst_pad_query_caps(valveSrc.get(), nullptr));
    }

    const std::optional<VideoReceiver::FileFormat> container = containerFor(request.format, inputCaps.get());
    if (container && (*container != request.format)) {
        qCDebug(GstVideoReceiverLog) << "The container of" << request.file << "can't hold"
                                     << PipelineStatsProbe::codecName(inputCaps.get()) << "video"
                                     << _context.redactedUri;
        // The listener's request for a file in that container starts it, from inside this call.
        _listener.onRecordingNeedsFormat(request.file, *container);
        if ((_state != State::Idle) || !_requested || (*_requested != request)) {
            return;
        }
    }

    _current = request;
    (void) _recording.fetch_add(1, std::memory_order_relaxed);
    _failure.clear();
    _fileStarted.store(false, std::memory_order_relaxed);
    _fedAt.store(0, std::memory_order_relaxed);
    _openedReported = false;
    _state = State::Recording;

    qCDebug(GstVideoReceiverLog) << "Starting recording" << request.file << _context.redactedUri;

    if (container != request.format) {
        _failure = QCoreApplication::translate("RecordingBranch", "No recording format can hold %1 video.")
                       .arg(PipelineStatsProbe::codecName(inputCaps.get()));
        _finish(QString());
        return;
    }

    _fileSink = makeFileSink(request.file, request.format, inputCaps.get());
    if (!_fileSink) {
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to create the recording sink.");
        _finish(QString());
        return;
    }
    TeeBranchHead::absorbFlowErrors(_fileSink.get());

    gst_util_set_object_arg(G_OBJECT(_head.queue()), "leaky", "no");
    if (!gst_bin_add(GST_BIN(_context.pipeline), _fileSink.get()) ||
        !gst_element_link(_head.valve(), _fileSink.get())) {
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
        _finalizeTimer.start();
        return;
    }

    // The file starts with a keyframe at t=0, so playback can decode from its first frame. The stream's first keyframe
    // lets it into the recorder, whose parser passes on nothing before a keyframe that comes with its parameter sets:
    // the first frame the file can hold.
    const GStreamer::GstElementPtr parser =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(_fileSink.get()), "parser"));
    const GStreamer::GstPadPtr parserSrc =
        GStreamer::adoptPad(parser ? gst_element_get_static_pad(parser.get(), "src") : nullptr);
    const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "sink"));
    const auto newWatch = [this]() {
        return new RecordingWatch{.branch = this, .recording = _recording.load(std::memory_order_relaxed)};
    };
    const GDestroyNotify deleteWatch = [](gpointer watch) { delete static_cast<RecordingWatch*>(watch); };
    if (!_keyframeWatch.add(valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, _keyframeProbe, this) ||
        !_startWatch.add(parserSrc ? parserSrc.get() : valveSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, _startProbe,
                         newWatch(), deleteWatch) ||
        !_capsWatch.add(valveSink.get(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _capsProbe, newWatch(), deleteWatch)) {
        qCCritical(GstVideoReceiverLog) << "Failed to add the recording's probes" << _context.redactedUri;
        _failure = QCoreApplication::translate("RecordingBranch", "Unable to start recording.");
        _finish(QString());
        return;
    }

    if (_switching || (_capsHold.load(std::memory_order_acquire) != 0)) {
        // The switch asked for the keyframe this file starts with, and holds it or still waits for it; the one that
        // comes with new caps follows them.
        g_object_set(_head.valve(), "drop", FALSE, nullptr);
    } else {
        _head.open();
    }
    _watchTimer.start();
    _context.dotDumper->dump(_context.pipeline, "pipeline-with-filesink");
}

void RecordingBranch::_switchAtKeyframe()
{
    if (_switching) {
        return;
    }
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "src"));
    if (!_switchHold.add(valveSrc.get(),
                         static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_BUFFER),
                         _switchProbe, new SwitchWatch{.branch = this, .hold = ++_switchHoldId},
                         [](gpointer watch) { delete static_cast<SwitchWatch*>(watch); })) {
        qCWarning(GstVideoReceiverLog) << "Unable to hold the next keyframe; frames up to it are not recorded";
        _end();
        return;
    }
    _switching = true;
    // Not left to the stream: with a long GOP, or a camera that sends keyframes on request only, the current file
    // would go on growing meanwhile.
    _head.requestKeyframe();
}

void RecordingBranch::_onSwitchKeyframe(quint64 hold)
{
    if (!_switching || (hold != _switchHoldId) || (_state != State::Recording)) {
        return;
    }
    // Its _finish() starts the requested file and lets the keyframe go.
    _end();
}

void RecordingBranch::_releaseSwitchHold()
{
    _switchHold.remove();
    _switching = false;
    // Both at once: a pad lets go of what it holds only once its last blocking probe is gone.
    if (const gulong capsHold = _capsHold.exchange(0, std::memory_order_acq_rel); (capsHold != 0) && _head.valve()) {
        const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "src"));
        gst_pad_remove_probe(valveSrc.get(), capsHold);
    }
}

void RecordingBranch::_drain()
{
    // No file follows: what is held for a switch goes into this one, and nothing more is held or asked of it.
    _capsWatch.remove();
    _releaseSwitchHold();
    _watchTimer.stop();
    _state = State::Draining;

    const quint64 recording = _recording.load(std::memory_order_relaxed);
    const GStreamer::GstPadPtr valveSink = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "sink"));
    if (!_drainWatch.add(valveSink.get(), GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, _drainProbe,
                         new RecordingWatch{.branch = this, .recording = recording},
                         [](gpointer watch) { delete static_cast<RecordingWatch*>(watch); })) {
        qCWarning(GstVideoReceiverLog) << "Unable to mark the recording's end;"
                                       << "the frames still queued are not recorded";
        _end();
        return;
    }
    // Closing the valve now would drop the backlog the queue holds for slow storage. The mark is serialized with the
    // frames, so sending it waits for the tee's streaming thread: not sent from here.
    const GStreamer::GstPadPtr queueSink = GStreamer::adoptPad(gst_element_get_static_pad(_head.queue(), "sink"));
    GStreamer::sendEventAsync(
        queueSink.get(),
        gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM,
                             gst_structure_new(kEndMark, "recording", G_TYPE_UINT64, recording, nullptr)));
    // Storage too slow to take the backlog in that time loses it, and the file is reported as not finalized.
    _finalizeTimer.start();
}

void RecordingBranch::_onDrained(quint64 recording)
{
    if ((_state != State::Draining) || (recording != _recording.load(std::memory_order_relaxed))) {
        return;
    }
    _end();
}

void RecordingBranch::_end(const QString& failure)
{
    if ((_state != State::Recording) && (_state != State::Draining)) {
        return;
    }
    if (!failure.isEmpty()) {
        _failure = failure;
    }

    _head.close();
    _drainWatch.remove();
    _keyframeWatch.remove();
    _watchTimer.stop();

    if (!_fileStarted.load(std::memory_order_acquire)) {
        _finish(QString());
        return;
    }

    // Unlinked, so the next recording can take the valve while this one drains parser -> muxer -> file. Not sent from
    // here: the EOS waits for the recorder's streaming thread, which slow storage can hold up.
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(_head.valve(), "src"));
    const GStreamer::GstPadPtr recorderSink = GStreamer::adoptPad(gst_pad_get_peer(valveSrc.get()));
    if (!recorderSink || !gst_pad_unlink(valveSrc.get(), recorderSink.get())) {
        qCWarning(GstVideoReceiverLog) << "Unable to unlink the recording sink";
        _finish(notFinalizedError());
        return;
    }
    GStreamer::sendEventAsync(recorderSink.get(), gst_event_new_eos());
    _state = State::Finalizing;
    _finalizeTimer.start();
}

void RecordingBranch::_finish(const QString& error, bool fileClosed)
{
    _finalizeTimer.stop();
    _watchTimer.stop();
    _keyframeWatch.remove();
    _startWatch.remove();
    _capsWatch.remove();
    _drainWatch.remove();
    _head.close();

    if (_fileSink) {
        // Removed before going NULL, so nothing it posts while stopping reaches the bus. Flushed unless it closed its
        // file: going NULL would otherwise wait for a streaming thread stuck inside it.
        if (fileClosed) {
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

    // A keyframe or a change of caps held for a switch goes into the file _reconcile() started. Without one it finds
    // the valve closed, which hides that nothing is linked behind it.
    _releaseSwitchHold();
}

bool RecordingBranch::beginTeardown()
{
    _requested.reset();
    // What is held for a switch that won't happen goes into the current file, ahead of the EOS that drains it, and
    // nothing more is held: no file follows.
    _capsWatch.remove();
    _releaseSwitchHold();
    // A recording being drained ends with the EOS as well: the valve lets it through behind the end mark.
    if ((_state != State::Recording) && (_state != State::Draining)) {
        return finalizing();
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

    // Before the flush, which a recorder stuck in the kernel can hold up.
    warnNotFinalized();
    // Not serialized, so it doesn't wait for the stream lock: wakes the tee's thread if the full queue holds it, and
    // has the queue stop pushing into the recorder.
    const GStreamer::GstPadPtr queueSink = GStreamer::adoptPad(gst_element_get_static_pad(_head.queue(), "sink"));
    (void) gst_pad_send_event(queueSink.get(), gst_event_new_flush_start());
    _finish(notFinalizedError());
}

bool RecordingBranch::_fromRecorder(GstObject* source) const
{
    return _fileSink && source && gst_object_has_as_ancestor(source, GST_OBJECT_CAST(_fileSink.get()));
}

void RecordingBranch::onBusError(GstMessage* message, const GError* error, const QString& text, const char* debug)
{
    if (!_fromRecorder(GST_MESSAGE_SRC(message))) {
        qCDebug(GstVideoReceiverLog) << "Ignoring an error from an earlier recording:" << text;
        return;
    }

    // The detail names the cause behind a generic text such as "Could not multiplex stream."
    if (debug) {
        qCWarning(GstVideoReceiverLog) << "Recording error:" << text << "-" << debug;
    } else {
        qCWarning(GstVideoReceiverLog) << "Recording error:" << text;
    }
    // GStreamer's text for it names the file, not the cause.
    _failure = reportsFullStorage(error, debug) ? QCoreApplication::translate("RecordingBranch", "The storage is full.")
                                                : text;
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
    if (!_fromRecorder(GST_MESSAGE_SRC(forwarded.get()))) {
        return;
    }
    if ((_state == State::Recording) || (_state == State::Draining)) {
        // The stream's own EOS came through the open valve. Nothing more will stream through this pipeline, so the
        // request waits for the next one rather than start over in the file just closed.
        _streaming = false;
    }
    qCDebug(GstVideoReceiverLog) << "Recording finalized" << _current.file;
    _finish(QString(), /*fileClosed=*/true);
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

void RecordingBranch::_onCapsChanged(quint64 recording)
{
    if ((_capsHold.load(std::memory_order_acquire) == 0) || (_state == State::Finalizing)) {
        // Let go already, or held until _finish() has started what follows the file that is finishing.
        return;
    }
    if ((_state != State::Recording) || (recording != _recording.load(std::memory_order_relaxed))) {
        // The file the change was held for is gone: it goes on to what is linked by now.
        _releaseSwitchHold();
        _reconcile();
        return;
    }

    qCDebug(GstVideoReceiverLog) << "The stream's caps changed while recording" << _current.file;
    if (_requested && (*_requested == _current)) {
        // The listener counts a file from this report on.
        _reportOpened();
        _listener.onRecordingNeedsNewFile(_current.file);
    }
    // Ends the file where the stream is held, for the one requested by now. Without one the change goes on into the
    // file, whose muxer decides whether it can take it.
    _reconcile();
}

void RecordingBranch::_checkStarted()
{
    if ((_state != State::Recording) || _fileStarted.load(std::memory_order_acquire)) {
        return;
    }
    const gint64 fedAt = _fedAt.load(std::memory_order_relaxed);
    const std::chrono::milliseconds timeout = _context.timings.recordingStartTimeout;
    if ((fedAt == 0) || ((g_get_monotonic_time() - fedAt) < std::chrono::microseconds(timeout).count())) {
        return;
    }
    qCWarning(GstVideoReceiverLog) << "No keyframe to start the recording with within" << timeout.count()
                                   << "ms of the stream's first one, as when keyframes come without their parameter"
                                   << "sets; ending the recording" << _current.file;
    _end(
        QCoreApplication::translate("RecordingBranch", "The video stream has no keyframe a recording can start from."));
}

void RecordingBranch::_checkHeaderSpace()
{
    // A switch under way leaves the file soon enough. Until its second header update the muxer has its estimate from
    // the file's first frame alone: seconds, when that frame and the next arrived together.
    if ((_state != State::Recording) || _switching || !_fileSink || !_openedReported ||
        !_openedFor.hasExpired(_context.timings.recordingHeaderEstimateDelay.count())) {
        return;
    }
    const GStreamer::GstElementPtr muxer =
        GStreamer::adoptElement(gst_bin_get_by_name(GST_BIN(_fileSink.get()), "muxer"));
    if (!muxer || !g_object_class_find_property(G_OBJECT_GET_CLASS(muxer.get()), "reserved-duration-remaining")) {
        return;
    }
    // The muxer's estimate from the space the recording used so far.
    guint64 remainingNs = 0;
    g_object_get(muxer.get(), "reserved-duration-remaining", &remainingNs, nullptr);
    if (remainingNs >= static_cast<guint64>(std::chrono::nanoseconds(kHeaderSpaceMargin).count())) {
        return;
    }
    qCDebug(GstVideoReceiverLog) << "Header space left for about" << (remainingNs / GST_SECOND) << "s in"
                                 << _current.file;
    _listener.onRecordingNeedsNewFile(_current.file);
}

void RecordingBranch::_reportOpened()
{
    if (_openedReported) {
        return;
    }
    _openedReported = true;
    _openedFor.start();
    qCDebug(GstVideoReceiverLog) << "Recording started" << _current.file;
    _listener.onRecordingFileOpened(_current.file);
}

GstPadProbeReturn RecordingBranch::_keyframeProbe([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* const self = static_cast<RecordingBranch*>(data);
    if (self->_fedAt.load(std::memory_order_relaxed) != 0) {
        return GST_PAD_PROBE_OK;
    }

    // Upstream of the tee is always a parser (GstBaseParse), which pushes single buffers, never lists.
    const GstBuffer* const buffer = gst_pad_probe_info_get_buffer(info);
    if (!buffer || GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) {
        return GST_PAD_PROBE_DROP;
    }
    self->_fedAt.store(g_get_monotonic_time(), std::memory_order_relaxed);
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RecordingBranch::_startProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    const auto* const watch = static_cast<const RecordingWatch*>(data);
    RecordingBranch* const self = watch->branch;
    if (self->_fileStarted.load(std::memory_order_relaxed)) {
        return GST_PAD_PROBE_OK;
    }

    // As the one upstream of the tee, the recorder's parser pushes single buffers.
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

    // Starts the file at 0; only the elements downstream of this pad see the offset.
    gst_pad_set_offset(pad, -static_cast<gint64>(runningTime));

    self->_fileStarted.store(true, std::memory_order_release);
    // Reported from the worker, which knows whether this recording is still the current one.
    const quint64 recording = watch->recording;
    self->_context.post([self, recording]() { self->_onFileOpened(recording); });
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RecordingBranch::_capsProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    const auto* const watch = static_cast<const RecordingWatch*>(data);
    RecordingBranch* const self = watch->branch;
    GstEvent* const event = gst_pad_probe_info_get_event(info);
    if (isEndMark(event)) {
        // Of a recording that was closed before its queue let the mark go.
        return GST_PAD_PROBE_DROP;
    }
    // Until its file starts the recorder takes the caps as they come, and one change is held at a time.
    if (!event || (GST_EVENT_TYPE(event) != GST_EVENT_CAPS) || !self->_fileStarted.load(std::memory_order_acquire) ||
        (self->_capsHold.load(std::memory_order_acquire) != 0)) {
        return GST_PAD_PROBE_OK;
    }

    // A sink pad takes up an event after its probes, so it still has the caps the file records.
    GstCaps* caps = nullptr;
    gst_event_parse_caps(event, &caps);
    const GStreamer::GstCapsPtr recorded = GStreamer::adoptCaps(gst_pad_get_current_caps(pad));
    const GStreamer::GstElementPtr valve = GStreamer::adoptElement(gst_pad_get_parent_element(pad));
    if (!valve || (recorded && gst_caps_is_equal(recorded.get(), caps))) {
        return GST_PAD_PROBE_OK;
    }

    // matroskamux takes no other caps in a file it has begun, and qtmux only some (a resolution, not a pixel aspect
    // ratio), which not every player follows. The event is held where the valve passes it on next, so the file ends
    // before it and the next one begins with it. Not held by a probe that stays on that pad: one blocking probe left
    // on a pad keeps it from letting go of what another one held.
    const GStreamer::GstPadPtr valveSrc = GStreamer::adoptPad(gst_element_get_static_pad(valve.get(), "src"));
    self->_capsHold.store(
        gst_pad_add_probe(valveSrc.get(),
                          static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM),
                          holdCapsEvent, gst_caps_ref(caps),
                          [](gpointer held) { gst_caps_unref(static_cast<GstCaps*>(held)); }),
        std::memory_order_release);
    const quint64 recording = watch->recording;
    self->_context.post([self, recording]() { self->_onCapsChanged(recording); });
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RecordingBranch::_switchProbe([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    const GstBuffer* const buffer = gst_pad_probe_info_get_buffer(info);
    if (!buffer || GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT) || !GST_BUFFER_PTS_IS_VALID(buffer)) {
        // Passes, and the probe is asked again for the next buffer.
        return GST_PAD_PROBE_PASS;
    }

    const auto* const watch = static_cast<const SwitchWatch*>(data);
    RecordingBranch* const self = watch->branch;
    const quint64 hold = watch->hold;
    self->_context.post([self, hold]() { self->_onSwitchKeyframe(hold); });
    // Blocks, with the keyframe, until the probe is removed.
    return GST_PAD_PROBE_OK;
}

GstPadProbeReturn RecordingBranch::_drainProbe(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstEvent* const event = gst_pad_probe_info_get_event(info);
    if (!isEndMark(event)) {
        return GST_PAD_PROBE_OK;
    }
    const auto* const watch = static_cast<const RecordingWatch*>(data);
    guint64 recording = 0;
    if (gst_structure_get_uint64(gst_event_get_structure(event), "recording", &recording) &&
        (recording == watch->recording)) {
        // Every frame queued ahead of the mark is in the recorder; what follows isn't the file's.
        if (const GStreamer::GstElementPtr valve = GStreamer::adoptElement(gst_pad_get_parent_element(pad))) {
            g_object_set(valve.get(), "drop", TRUE, nullptr);
        }
        RecordingBranch* const self = watch->branch;
        self->_context.post([self, recording]() { self->_onDrained(recording); });
    }
    return GST_PAD_PROBE_DROP;
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
