#include "RecordingSink.h"

#include <chrono>
#include <utility>

#include "BranchContext.h"
#include "GStreamerHelpers.h"
#include "GstReceiverTimings.h"
#include "GstVideoReceiver.h"
#include "RecordingPlan.h"

namespace GStreamer::RecordingSink {

namespace {

// MP4/MOV robust muxing: qtmux reserves header space at the start of the file and rewrites the moov
// there every update period, so a crash or power loss leaves a file playable up to the last update.
// The reservation (550 B/s per track by default, ~8 MB here) outlasts the nominal duration since the
// index grows ~140 B/s at 30 fps and ~260 B/s at 60 fps; overrunning it makes qtmux post an ERROR.
// No file is meant to get that far: its recording continues in a new part after half the nominal duration. The
// index grows faster at higher frame rates, and when no two frames last the same, as without a jitter buffer, so
// the muxer's own estimate of the room left is watched too (RecordingBranch::kHeaderSpaceMargin).
// The period bounds crash data loss without rewriting the growing moov too often.
constexpr guint64 kRecordingReservedMaxDurationNs = 2 * (3600 * GST_SECOND);
constexpr guint64 kRecordingMoovUpdatePeriodNs = 5 * GST_SECOND;
// matroskamux never has its file synced, as qtmux has with each header update. Storage that takes writes into a cache
// (an Android SD card, a network share) reports a full disk only to a sync: a recording there would lose everything
// without an error. A sync as often as the MP4's also bounds what a power loss takes.
constexpr gint64 kRecordingSyncPeriodUs = static_cast<gint64>(kRecordingMoovUpdatePeriodNs / GST_USECOND);
static_assert(std::chrono::nanoseconds(GstReceiverTimings{}.recordingHeaderEstimateDelay).count() >=
                  2 * static_cast<qint64>(kRecordingMoovUpdatePeriodNs),
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

}  // namespace

SinkBin create(const QString& file, VideoReceiver::FileFormat format, const GstCaps* inputCaps)
{
    const char* const muxerName = muxerFactory(format);
    if (!muxerName) {
        qCCritical(GstVideoReceiverLog) << "Unsupported file format" << static_cast<int>(format);
        return {};
    }

    GStreamer::GstElementPtr bin = GStreamer::adoptFloating(gst_bin_new("sinkbin"));

    GstElement* parser = nullptr;
    if (const RecordingParser* const recorderParser = recordingParser(inputCaps)) {
        GStreamer::GstElementPtr parserOwner = GStreamer::makeElement(recorderParser->factory, "parser");
        if (!parserOwner) {
            qCWarning(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << recorderParser->factory;
            return {};
        }
        g_object_set(parserOwner.get(), "config-interval", -1, nullptr);
        parser = GStreamer::addToBin(bin.get(), std::move(parserOwner), "parser");
        if (!parser) {
            return {};
        }
    }

    GStreamer::GstElementPtr muxerOwner = GStreamer::makeElement(muxerName, "muxer");
    GStreamer::GstElementPtr fileSinkOwner = GStreamer::makeElement("filesink", "filesink");
    if (!muxerOwner || !fileSinkOwner) {
        qCWarning(GstVideoReceiverLog) << "gst_element_factory_make() failed for" << muxerName << "or filesink";
        return {};
    }

    // Tagged with the time it starts and the application, as a USB camera's recordings are (QtVideoRecorder).
    GstDateTime* const started = gst_date_time_new_from_g_date_time(g_date_time_new_now_utc());
    gst_tag_setter_add_tags(GST_TAG_SETTER(muxerOwner.get()), GST_TAG_MERGE_REPLACE, GST_TAG_DATE_TIME, started,
                            GST_TAG_COMMENT, VideoReceiver::recordingComment().toUtf8().constData(), nullptr);
    gst_date_time_unref(started);
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
        return {};
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
        return {};
    }
    (void) gst_pad_add_probe(muxerPad.get(), GST_PAD_PROBE_TYPE_BUFFER, timestampForMuxer, nullptr, nullptr);

    GStreamer::GstPadPtr parserSinkPad;
    if (parser) {
        const GStreamer::GstPadPtr parserSrcPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "src"));
        parserSinkPad = GStreamer::adoptPad(gst_element_get_static_pad(parser, "sink"));

        const GstPadLinkReturn linkResult = gst_pad_link(parserSrcPad.get(), muxerPad.get());
        if (linkResult != GST_PAD_LINK_OK) {
            qCWarning(GstVideoReceiverLog) << "Failed to link parser and muxer:" << gst_pad_link_get_name(linkResult);
            return {};
        }
    }

    // Each element, not just the bin: a message can outlive the bin that held its source.
    for (GstElement* const element : {bin.get(), parser, muxer, fileSink}) {
        if (element) {
            GStreamer::markBranch(element, GStreamer::PipelineBranch::Recording);
        }
    }

    (void) gst_element_add_pad(bin.get(),
                               gst_ghost_pad_new("sink", parserSinkPad ? parserSinkPad.get() : muxerPad.get()));

    return {.bin = std::move(bin),
            .parser = parser ? GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(parser))) : nullptr,
            .muxer = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(muxer)))};
}

std::optional<VideoReceiver::FileFormat> containerFor(VideoReceiver::FileFormat format, const GstCaps* inputCaps)
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

}  // namespace GStreamer::RecordingSink
