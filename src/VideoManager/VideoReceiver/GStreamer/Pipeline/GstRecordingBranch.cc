#include "GstRecordingBranch.h"

#include <QtMultimedia/QMediaFormat>
#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstRecordingBranchLog, "Video.GstRecordingBranch")

// Map QMediaFormat::FileFormat → GStreamer mux element name.
// Returns nullptr for unsupported formats.
static const char* gstMuxForFormat(QMediaFormat::FileFormat fmt)
{
    switch (fmt) {
        case QMediaFormat::Matroska:  return "matroskamux";
        case QMediaFormat::QuickTime: return "qtmux";
        case QMediaFormat::MPEG4:     return "mp4mux";
        default:                      return nullptr;
    }
}


VideoReceiver::STATUS GstRecordingBranch::start(GstElement* pipeline, GstElement* recorderValve, GstElement* tee,
                                                const QString& videoFile, QMediaFormat::FileFormat format,
                                                std::function<void(const QString&)> onKeyframe)
{
    _state = BranchState::Starting;

    qCDebug(GstRecordingBranchLog) << "New video file:" << videoFile;

    _fileSink = GstObjectPtr<GstElement>(makeFileSink(videoFile, format, tee));
    if (!_fileSink) {
        qCCritical(GstRecordingBranchLog) << "makeFileSink() failed";
        _state = BranchState::Off;
        return VideoReceiver::STATUS_FAIL;
    }

    gst_bin_add(GST_BIN(pipeline), _fileSink.get());

    if (!gst_element_link(recorderValve, _fileSink.get())) {
        qCCritical(GstRecordingBranchLog) << "Failed to link valve and file sink";
        gst_bin_remove(GST_BIN(pipeline), _fileSink.get());
        _fileSink = {};
        _state = BranchState::Off;
        return VideoReceiver::STATUS_FAIL;
    }

    (void)gst_element_sync_state_with_parent(_fileSink.get());

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-filesink");

    // Install a probe on the recording branch to drop buffers until we hit the first keyframe.
    // When we hit the first keyframe, offset timestamps so decoding starts immediately on playback.
    GstNonFloatingPtr<GstPad> probepad(gst_element_get_static_pad(recorderValve, "src"));
    if (!probepad) {
        qCCritical(GstRecordingBranchLog) << "gst_element_get_static_pad() failed";
        shutdown(pipeline);
        return VideoReceiver::STATUS_FAIL;
    }

    // Record the probe so shutdown() can remove it if EOS never arrives
    // before teardown (otherwise ctx leaks and the callback fires into
    // freed state).
    auto* ctx = new KeyframeProbeCtx{std::move(onKeyframe), videoFile};
    const gulong probeId = gst_pad_add_probe(probepad.get(), GST_PAD_PROBE_TYPE_BUFFER, keyframeWatch, ctx,
                                             [](gpointer data) { delete static_cast<KeyframeProbeCtx*>(data); });
    _keyframeProbe = GstPadProbeGuard(probepad.get(), probeId);

    g_object_set(recorderValve, "drop", FALSE, nullptr);

    _state = BranchState::Active;
    qCDebug(GstRecordingBranchLog) << "Recording started";

    return VideoReceiver::STATUS_OK;
}

VideoReceiver::STATUS GstRecordingBranch::stop(GstElement* recorderValve, std::function<bool(GstElement*)> unlinkFn)
{
    _state = BranchState::Stopping;

    if (recorderValve)
        g_object_set(recorderValve, "drop", TRUE, nullptr);

    if (!unlinkFn(recorderValve)) {
        return VideoReceiver::STATUS_FAIL;
    }

    return VideoReceiver::STATUS_OK;
}

void GstRecordingBranch::shutdown(GstElement* pipeline)
{
    // Remove the keyframe probe before teardown so the probe callback
    // cannot fire into freed ctx after the file sink disappears.
    _keyframeProbe.remove();

    GStreamer::gstRemoveFromParent(_fileSink.get());
    _fileSink = {};

    _state = BranchState::Off;

    if (pipeline)
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-recording-stopped");
}

const char* GstRecordingBranch::selectMuxForStream(GstElement* tee, QMediaFormat::FileFormat format) const
{
    const char* requested = gstMuxForFormat(format);
    if (!requested)
        return nullptr;

    if (!tee)
        return requested;

    // Probe stream codec from tee's sink pad to auto-correct the muxer choice.
    // mp4mux/qtmux require specific stream-format constraints that not all
    // codecs satisfy — fall back to a universal container when needed.
    GstNonFloatingPtr<GstPad> pad(gst_element_get_static_pad(tee, "sink"));
    if (!pad)
        return requested;

    GstCapsPtr caps(gst_pad_get_current_caps(pad.get()));
    if (!caps || gst_caps_get_size(caps.get()) == 0)
        return requested;

    const GstStructure* s = gst_caps_get_structure(caps.get(), 0);
    const gchar* mediaType = gst_structure_get_name(s);
    if (!mediaType)
        return requested;

    // H.265: mp4mux only handles hvc1 stream-format; fallback to matroskamux otherwise
    if (g_str_has_prefix(mediaType, "video/x-h265")) {
        if (format == QMediaFormat::MPEG4 || format == QMediaFormat::QuickTime) {
            const gchar* streamFmt = gst_structure_get_string(s, "stream-format");
            if (!streamFmt || g_strcmp0(streamFmt, "hvc1") != 0) {
                qCDebug(GstRecordingBranchLog) << "H.265 stream-format is" << (streamFmt ? streamFmt : "unknown")
                                               << "— using matroskamux instead of" << requested;
                return "matroskamux";
            }
        }
    }

    // AV1: mp4mux supports AV1 (via av01 codec tag) but only in recent GStreamer.
    // matroskamux/webmmux are safer — prefer mkv for AV1.
    if (g_str_has_prefix(mediaType, "video/x-av1")) {
        if (format != QMediaFormat::Matroska) {
            qCDebug(GstRecordingBranchLog) << "AV1 stream — using matroskamux instead of" << requested;
            return "matroskamux";
        }
    }

    // VP8/VP9: webmmux is the natural container, but matroskamux is a superset
    if (g_str_has_prefix(mediaType, "video/x-vp8") || g_str_has_prefix(mediaType, "video/x-vp9")) {
        if (format != QMediaFormat::Matroska) {
            qCDebug(GstRecordingBranchLog) << "VP8/VP9 stream — using matroskamux instead of" << requested;
            return "matroskamux";
        }
    }

    return requested;
}

GstElement* GstRecordingBranch::makeFileSink(const QString& videoFile, QMediaFormat::FileFormat format,
                                             GstElement* tee)
{
    if (!gstMuxForFormat(format)) {
        qCCritical(GstRecordingBranchLog) << "Unsupported file format" << static_cast<int>(format);
        return nullptr;
    }

    const char* muxName = selectMuxForStream(tee, format);
    if (!muxName) {
        qCCritical(GstRecordingBranchLog) << "No suitable mux for format";
        return nullptr;
    }

    GstObjectPtr<GstElement> mux(gst_element_factory_make(muxName, nullptr));
    if (!mux) {
        qCCritical(GstRecordingBranchLog) << "gst_element_factory_make('" << muxName << "') failed";
        return nullptr;
    }

    // Fragmented output for crash recovery (moov atom written incrementally)
    if (format == QMediaFormat::QuickTime || format == QMediaFormat::MPEG4)
        g_object_set(mux.get(), "fragment-duration", 1000, "streamable", TRUE, nullptr);

    GstObjectPtr<GstElement> sink(gst_element_factory_make("filesink", nullptr));
    if (!sink) {
        qCCritical(GstRecordingBranchLog) << "gst_element_factory_make('filesink') failed";
        return nullptr;
    }

    g_object_set(sink.get(), "location", qPrintable(videoFile), nullptr);

    GstObjectPtr<GstElement> bin(gst_bin_new("sinkbin"));
    if (!bin) {
        qCCritical(GstRecordingBranchLog) << "gst_bin_new('sinkbin') failed";
        return nullptr;
    }

    GstPadTemplate* padTemplate = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(mux.get()), "video_%u");
    if (!padTemplate) {
        qCCritical(GstRecordingBranchLog) << "gst_element_class_get_pad_template(mux) failed";
        return nullptr;
    }

    GstNonFloatingPtr<GstPad> pad(gst_element_request_pad(mux.get(), padTemplate, nullptr, nullptr));
    if (!pad) {
        qCCritical(GstRecordingBranchLog) << "gst_element_request_pad(mux) failed";
        return nullptr;
    }

    // Transfer ownership to bin — release from RAII wrappers
    GstElement* rawMux = mux.release();
    GstElement* rawSink = sink.release();
    gst_bin_add_many(GST_BIN(bin.get()), rawMux, rawSink, nullptr);

    GstPad* ghostpad = gst_ghost_pad_new("sink", pad.get());
    (void)gst_element_add_pad(bin.get(), ghostpad);

    if (!gst_element_link(rawMux, rawSink)) {
        qCCritical(GstRecordingBranchLog) << "gst_element_link() failed";
        return nullptr;  // bin RAII will clean up (including mux+sink it owns)
    }

    return bin.release();
}

GstPadProbeReturn GstRecordingBranch::keyframeWatch(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    if (!info || !user_data) {
        qCCritical(GstRecordingBranchLog) << "Invalid arguments";
        return GST_PAD_PROBE_DROP;
    }

    GstBuffer* buf = gst_pad_probe_info_get_buffer(info);
    if (!buf || GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
        // no buffer in this probe info, or not a keyframe — wait
        return GST_PAD_PROBE_DROP;
    }

    // set media file '0' offset to current timeline position
    gst_pad_set_offset(pad, -static_cast<gint64>(buf->pts));

    qCDebug(GstRecordingBranchLog) << "Got keyframe, stop dropping buffers";

    auto* ctx = static_cast<KeyframeProbeCtx*>(user_data);
    if (ctx->onKeyframe)
        ctx->onKeyframe(ctx->recordingOutput);
    // ctx is freed by the GDestroyNotify passed to gst_pad_add_probe

    return GST_PAD_PROBE_REMOVE;
}
