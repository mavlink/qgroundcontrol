#include "gstqgcqvideosink.h"

#include <atomic>
#include <gst/video/video-info.h>
#include <gst/video/video.h>
#include <memory>
#include <new>
#include <utility>

#include <QtCore/QSize>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "GstQgcAllocation.h"
#include "GstQgcCaps.h"
#include "GstQgcFrameDelivery.h"
#include "GstQgcFrameMap.h"
#include "GstQgcPathSession.h"
#include "GstQgcSinkCounters.h"
#include "GstQgcSinkStats.h"
#include "HwBuffers/common/GstHwVideoInfo.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "gstqgcelements.h"
#include "gstqgcvideosinkbin.h"

QGC_LOGGING_CATEGORY(GstQgcQVideoSinkLog, "Video.GStreamer.QgcQVideoSink")

#define GST_CAT_DEFAULT gst_qgc_debug

namespace {

/// The element's C++ state, placement-constructed in instance_init and destroyed in finalize.
struct PrivState
{
    // The QVideoSink frames go to, set by the controller on the GUI thread.
    GstQgc::FrameDelivery delivery;
    GstQgc::SinkCounters counters;
    // Read lock-free by show_frame.
    std::atomic<bool> active{true};
    // start() derives hw_context from it.
    bool gpu_zerocopy = false;
    // Set by qgcvideosinkbin before linking (configureSinkCaps); with gpu_zerocopy, they decide what get_caps offers.
    GstQgc::GpuBinMode gpu_bin = GstQgc::GpuBinMode::Passthrough;
    bool square_pixels = true;
    // What get_caps last offered, and for which policy; under GST_OBJECT_LOCK. Rebuilt when the policy changes, e.g.
    // the render target or the DMABuf import state: building may query EGL.
    GstCaps* offered_caps = nullptr;
    GstQgc::SinkCapsPolicy offered_policy;
    // Negotiated state: set_caps/stop write it and show_frame reads it, all serialised by GstBaseSink.
    std::atomic<bool> caps_valid{false};
    GstVideoInfo video_info = {};
    QVideoFrameFormat format;
    // Written by start() under GST_OBJECT_LOCK and snapshotted by show_frame, which adds the current DMABuf display.
    HwVideoBufferContext hw_context = {};
    // Whether the negotiated caps carry a GPU memory feature. Without one, show_frame skips path selection for frames
    // in plain system memory.
    bool gpu_caps = false;
    // GstVideoOrientationMethod from the stream's image-orientation tag, for frames without an orientation meta.
    std::atomic<int> stream_orientation{GST_VIDEO_ORIENTATION_IDENTITY};
    // Live zero-copy CPU frames pinning this sink's decoder buffers; shared because Qt may hold frames past finalize.
    CpuVideoFramePool::PinnedFrameCounter pinned_cpu_frames = std::make_shared<std::atomic<int>>(0);
    // Negotiated caps held from set_caps; avoids per-frame allocation and preserves DRM modifiers.
    GstCaps* cached_caps{nullptr};
    // The path frames take through the caps epoch; set_caps starts a new one, so a format change re-runs path
    // selection.
    GstQgc::PathSession path_session;
};

}  // namespace

struct _GstQgcQVideoSink
{
    GstVideoSink parent;
    PrivState state;
};

namespace {

inline PrivState* priv_of(GstQgcQVideoSink* self)
{
    return &self->state;
}

/// Copies the GPU context under GST_OBJECT_LOCK and reports whether a QVideoSink is bound, so show_frame skips mapping
/// when nothing would consume the frame. Never touches the QVideoSink itself: its owner thread may be destroying it.
bool snapshot_sink(GstQgcQVideoSink* self, HwVideoBufferContext& hwOut)
{
    GST_OBJECT_LOCK(self);
    hwOut = priv_of(self)->hw_context;
    GST_OBJECT_UNLOCK(self);
    return priv_of(self)->delivery.sink() != nullptr;
}

/// The caps policy for @p target and the display the DMABuf path imports with (HwBuffers::dmaBufImportDisplay()); under
/// GST_OBJECT_LOCK. Neither decides the caps of a CPU sink or of the glupload and d3d12convert variants, so they are
/// left out there: equal policies offer equal caps.
GstQgc::SinkCapsPolicy caps_policy_locked(const PrivState* p, const GstQgc::RenderTarget& target, void* dmaBufDisplay)
{
    const bool followsRenderer = p->gpu_zerocopy && (p->gpu_bin != GstQgc::GpuBinMode::GlUpload) &&
                                 (p->gpu_bin != GstQgc::GpuBinMode::D3D12Convert);
    return {.gpuZeroCopy = p->gpu_zerocopy,
            .gpuBin = p->gpu_bin,
            .target = followsRenderer ? target : GstQgc::RenderTarget{},
            .dmaBufDisplay = followsRenderer ? dmaBufDisplay : nullptr,
            .squarePixels = p->square_pixels};
}

/// The caps get_caps offers now (transfer-full); rebuilt only when the policy differs from the last call's.
GstCaps* offered_caps(GstQgcQVideoSink* self)
{
    PrivState* const p = priv_of(self);
    const GstQgc::RenderTarget target = GstQgc::currentRenderTarget();
    void* const dmaBufDisplay = HwBuffers::dmaBufImportDisplay();
    GST_OBJECT_LOCK(self);
    const GstQgc::SinkCapsPolicy policy = caps_policy_locked(p, target, dmaBufDisplay);
    if (p->offered_caps && (p->offered_policy == policy)) {
        GstCaps* const cached = gst_caps_ref(p->offered_caps);
        GST_OBJECT_UNLOCK(self);
        return cached;
    }
    GST_OBJECT_UNLOCK(self);

    GstCaps* caps = gst_caps_from_string(GstQgc::buildSinkCaps(policy).c_str());
    if (!caps) {
        GST_ERROR_OBJECT(self, "sink caps did not parse");
        return gst_caps_new_empty();
    }
    GST_DEBUG_OBJECT(self, "offering %" GST_PTR_FORMAT " for render backend %s (EGL %d, DMABuf import %d)", caps,
                     GstQgc::toString(target.backend), target.egl, policy.dmaBufDisplay != nullptr);
    GST_OBJECT_LOCK(self);
    gst_caps_replace(&p->offered_caps, caps);
    p->offered_policy = policy;
    GST_OBJECT_UNLOCK(self);
    return caps;
}

/// The caps policy now. Asks upstream to renegotiate when it differs from the one get_caps last offered caps for: the
/// render target changed (the scene graph came up after negotiation, or Qt fell back to another graphics API), or the
/// DMABuf path started or stopped importing (Qt's EGLDisplay became known, or an import failure disabled the path).
GstQgc::SinkCapsPolicy reconfigure_on_policy_change(GstQgcQVideoSink* self)
{
    PrivState* const p = priv_of(self);
    const GstQgc::RenderTarget target = GstQgc::currentRenderTarget();
    void* const dmaBufDisplay = HwBuffers::dmaBufImportDisplay();
    GST_OBJECT_LOCK(self);
    const GstQgc::SinkCapsPolicy policy = caps_policy_locked(p, target, dmaBufDisplay);
    const bool changed = p->offered_caps && (p->offered_policy != policy);
    if (changed) {
        // Rebuilt by the next get_caps, which also re-arms this check.
        gst_clear_caps(&p->offered_caps);
    }
    GST_OBJECT_UNLOCK(self);
    if (changed) {
        GST_INFO_OBJECT(self, "render backend now %s (EGL %d), DMABuf import %d; asking upstream to renegotiate",
                        GstQgc::toString(target.backend), target.egl, policy.dmaBufDisplay != nullptr);
        (void) gst_pad_push_event(GST_BASE_SINK_PAD(self), gst_event_new_reconfigure());
    }
    return policy;
}

/// True if @p caps name a memory feature other than system memory (memory:DMABuf, memory:GLMemory, ...).
bool caps_carry_gpu_memory(GstCaps* caps)
{
    const GstCapsFeatures* const features = gst_caps_get_features(caps, 0);
    if (!features || gst_caps_features_is_any(features)) {
        return false;
    }
    for (guint i = 0; i < gst_caps_features_get_size(features); ++i) {
        const gchar* const feature = gst_caps_features_get_nth(features, i);
        if (g_str_has_prefix(feature, "memory:") && (g_strcmp0(feature, GST_CAPS_FEATURE_MEMORY_SYSTEM_MEMORY) != 0)) {
            return true;
        }
    }
    return false;
}

/// True when no GPU path can import @p buf: it has no memory, or its first memory is plain system memory.
bool nothing_to_import(GstBuffer* buf)
{
    if (gst_buffer_n_memory(buf) == 0) {
        return true;
    }
    GstMemory* const mem = gst_buffer_peek_memory(buf, 0);
    return mem && gst_memory_is_type(mem, GST_ALLOCATOR_SYSMEM);
}

}  // namespace

/// get_caps narrows this to the formats and memory the sink takes for its render target.
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw(ANY)"));

enum
{
    PROP_0,
    PROP_QVIDEOSINK,
    PROP_ACTIVE,
    PROP_GPU_ZEROCOPY,
    PROP_FRAMES_INPUT,
    PROP_FRAMES_DROPPED,
    PROP_FRAMES_DELIVERED,
    PROP_FRAME_STATS,
};

G_DEFINE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST_TYPE_VIDEO_SINK)
GST_ELEMENT_REGISTER_DEFINE(qgcqvideosink, "qgcqvideosink", GST_RANK_NONE, GST_TYPE_QGC_Q_VIDEO_SINK)

static void gst_qgc_q_video_sink_init(GstQgcQVideoSink* self)
{
    new (&self->state) PrivState();
    gst_video_info_init(&self->state.video_info);
    // sync=FALSE: drone video is "as fast as decoded"; GstBaseSink clock-sync would
    // stall on live RTSP. async=FALSE skips preroll wait so caps changes don't stall.
    gst_base_sink_set_sync(GST_BASE_SINK(self), FALSE);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(self), FALSE);
    // Don't retain the last buffer: it would pin an upstream pool slot for the stream's lifetime.
    gst_base_sink_set_last_sample_enabled(GST_BASE_SINK(self), FALSE);
}

// GstVideoSink shows a preroll buffer and then renders the same buffer, so the first buffer after each preroll would
// be mapped and delivered twice. QGC drives pipelines straight to PLAYING, where render follows preroll at once, so
// the preroll frame adds nothing. show-preroll-frame is a construct property: override it once construction is done.
static void gst_qgc_q_video_sink_constructed(GObject* obj)
{
    G_OBJECT_CLASS(gst_qgc_q_video_sink_parent_class)->constructed(obj);
    g_object_set(obj, "show-preroll-frame", FALSE, nullptr);
}

static void gst_qgc_q_video_sink_finalize(GObject* obj)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(obj);
    gst_clear_caps(&priv_of(self)->cached_caps);
    gst_clear_caps(&priv_of(self)->offered_caps);
    self->state.~PrivState();
    G_OBJECT_CLASS(gst_qgc_q_video_sink_parent_class)->finalize(obj);
}

static void gst_qgc_q_video_sink_set_property(GObject* obj, guint id, const GValue* val, GParamSpec* pspec)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(obj);
    PrivState* p = priv_of(self);
    if (id == PROP_QVIDEOSINK) {
        p->delivery.setSink(static_cast<QVideoSink*>(g_value_get_pointer(val)));
        return;
    }
    GST_OBJECT_LOCK(self);
    switch (id) {
        case PROP_ACTIVE:
            p->active.store(g_value_get_boolean(val), std::memory_order_relaxed);
            break;
        case PROP_GPU_ZEROCOPY:
            p->gpu_zerocopy = g_value_get_boolean(val);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(self);
}

static void gst_qgc_q_video_sink_get_property(GObject* obj, guint id, GValue* val, GParamSpec* pspec)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(obj);
    GST_OBJECT_LOCK(self);
    switch (id) {
        case PROP_QVIDEOSINK:
            g_value_set_pointer(val, priv_of(self)->delivery.sink());
            break;
        case PROP_ACTIVE:
            g_value_set_boolean(val, priv_of(self)->active.load(std::memory_order_relaxed));
            break;
        case PROP_GPU_ZEROCOPY:
            g_value_set_boolean(val, priv_of(self)->gpu_zerocopy);
            break;
        case PROP_FRAMES_INPUT:
            g_value_set_uint64(val, priv_of(self)->counters.input());
            break;
        case PROP_FRAMES_DROPPED:
            g_value_set_uint64(val, priv_of(self)->counters.dropped());
            break;
        case PROP_FRAMES_DELIVERED:
            g_value_set_uint64(val, priv_of(self)->counters.delivered());
            break;
        case PROP_FRAME_STATS:
            g_value_take_boxed(val, priv_of(self)->counters.frameStats());
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(self);
}

static GstCaps* gst_qgc_q_video_sink_get_caps(GstBaseSink* bsink, GstCaps* filter)
{
    GstCaps* caps = offered_caps(GST_QGC_Q_VIDEO_SINK(bsink));
    if (filter) {
        GstCaps* const filtered = gst_caps_intersect_full(filter, caps, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(caps);
        caps = filtered;
    }
    return caps;
}

static gboolean gst_qgc_q_video_sink_set_caps(GstBaseSink* bsink, GstCaps* caps)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    PrivState* p = priv_of(self);

    GstVideoInfo parsedInfo = {};
    if (!GstHw::dmaDrmAwareVideoInfo(caps, &parsedInfo)) {
        qCWarning(GstQgcQVideoSinkLog) << "set_caps: failed to parse video info from caps";
        return FALSE;
    }

    const QVideoFrameFormat::PixelFormat pixelFormat = toQtPixelFormat(GST_VIDEO_INFO_FORMAT(&parsedInfo));
    if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
        qCWarning(GstQgcQVideoSinkLog) << "set_caps: unsupported video format"
                                       << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&parsedInfo));
        return FALSE;
    }

    const int w = GST_VIDEO_INFO_WIDTH(&parsedInfo);
    const int h = GST_VIDEO_INFO_HEIGHT(&parsedInfo);
    if (w <= 0 || h <= 0) {
        qCWarning(GstQgcQVideoSinkLog) << "set_caps: invalid dimensions" << w << "x" << h;
        return FALSE;
    }

    QVideoFrameFormat fmt(QSize(w, h), pixelFormat);
    applyColorimetry(fmt, parsedInfo, caps);
    const int fpsN = GST_VIDEO_INFO_FPS_N(&parsedInfo);
    const int fpsD = GST_VIDEO_INFO_FPS_D(&parsedInfo);
    if (fpsN > 0 && fpsD > 0) {
        fmt.setStreamFrameRate(static_cast<qreal>(fpsN) / static_cast<qreal>(fpsD));
    }

    p->video_info = parsedInfo;
    p->format = std::move(fmt);
    gst_clear_caps(&p->cached_caps);
    // Hold the negotiated caps verbatim: rebuilding from video_info drops DRM modifiers and other
    // negotiated fields the downstream frame mapping relies on.
    p->cached_caps = gst_caps_ref(caps);
    p->gpu_caps = caps_carry_gpu_memory(caps);
    p->path_session.startEpoch();
    p->caps_valid.store(true, std::memory_order_release);

    qCInfo(GstQgcQVideoSinkLog).noquote()
        << "set_caps: format=" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&parsedInfo)) << w << "x" << h
        << "pixfmt=" << int(pixelFormat);
    return TRUE;
}

static GstFlowReturn gst_qgc_q_video_sink_show_frame(GstVideoSink* vsink, GstBuffer* buf)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(vsink);
    PrivState* p = priv_of(self);

    p->counters.noteInput();
    const GstQgc::SinkCapsPolicy policy = reconfigure_on_policy_change(self);

    if (!p->caps_valid.load(std::memory_order_acquire)) {
        // Should never happen — GstBaseSink calls set_caps before show_frame.
        return GST_FLOW_NOT_NEGOTIATED;
    }
    if (!p->active.load(std::memory_order_relaxed)) {
        p->counters.noteDropped();
        return GST_FLOW_OK;  // drop silently — controller drives the active flag
    }

    HwVideoBufferContext hwCtx;
    if (!snapshot_sink(self, hwCtx)) {
        p->counters.noteDropped();
        return GST_FLOW_OK;  // no destination yet; drop
    }
    hwCtx.dmaBufEglDisplay = policy.dmaBufDisplay;
    // Software-decoded frames skip GPU selection. Caps alone can't tell: GPU memory also arrives under plain
    // video/x-raw (vtdec's CoreVideo/IOSurface buffers, V4L2 dmabuf export), so frames in other memory still try it.
    hwCtx.gpuEnabled = hwCtx.gpuEnabled && (p->gpu_caps || !nothing_to_import(buf));

    // Build a cropped format copy only when crop meta is present (rare); otherwise pass
    // p->format by reference to avoid a per-frame QVideoFrameFormat refcount bump.
    QVideoFrameFormat croppedFmt;
    const bool hasCrop = (gst_buffer_get_video_crop_meta(buf) != nullptr);
    if (hasCrop) {
        croppedFmt = applyCropMeta(p->format, buf);
    }
    MappedFrame mapped = mapSampleToFrame(buf, p->cached_caps, p->video_info, hasCrop ? croppedFmt : p->format, hwCtx,
                                          p->pinned_cpu_frames, p->path_session.pathCache());
    if (!mapped.frame.isValid()) {
        p->counters.noteDropped();
        if (p->path_session.onMapFailed(mapped, p->counters) == GstQgc::PathSession::Verdict::Error) {
            const quint64 failures = p->path_session.consecutiveMapFailures();
            const char* const format = gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&p->video_info));
            // Without an element error upstream reports only a generic "Internal data stream error".
            GST_ELEMENT_ERROR(
                self, STREAM, FAILED, ("Video frames could not be mapped for display"),
                ("%" G_GUINT64_FORMAT " consecutive map failures (path %s, format %s)", static_cast<guint64>(failures),
                 GstHw::toString(mapped.gpuPath), format ? format : "unknown"));
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;  // drop transient failure, keep the stream alive
    }
    p->path_session.onMapped();

    applyOrientationAndTiming(mapped.frame, buf, p->stream_orientation.load(std::memory_order_relaxed));

    const QSize frameSize = mapped.frame.size();
    const GstQgc::FrameDelivery::Result delivery = p->delivery.deliver(std::move(mapped.frame));
    if (!delivery.delivered) {
        p->counters.noteDropped();
        return GST_FLOW_OK;  // QVideoSink unbound while mapping
    }
    const quint64 delivered = p->counters.noteDelivered(mapped.gpuPath, delivery.superseded);
    p->path_session.onDelivered(mapped, p->counters);

    if (delivered == 1) {
        qCInfo(GstQgcQVideoSinkLog).noquote()
            << "first frame delivered via" << GstHw::toString(mapped.gpuPath) << "path"
            << QStringLiteral("%1x%2").arg(frameSize.width()).arg(frameSize.height());
    } else if ((delivered % 300) == 0) {
        qCDebug(GstQgcQVideoSinkLog).noquote()
            << "frame flow:" << GstHw::toString(mapped.gpuPath) << "delivered=" << delivered
            << "input=" << p->counters.input() << "dropped=" << p->counters.dropped()
            << "cpuFrames=" << p->counters.pathFrames(HwVideoBufferPath::None);
    }
    return GST_FLOW_OK;
}

// Tracks the stream's orientation tag and releases an undelivered frame on flush.
static gboolean gst_qgc_q_video_sink_event(GstBaseSink* bsink, GstEvent* event)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_TAG: {
            // A camera mounted sideways tags the stream (e.g. "rotate-90") instead of rotating every frame.
            GstTagList* tags = nullptr;
            gst_event_parse_tag(event, &tags);
            GstVideoOrientationMethod method = GST_VIDEO_ORIENTATION_IDENTITY;
            if (tags && gst_video_orientation_from_tag(tags, &method)) {
                priv_of(self)->stream_orientation.store(method, std::memory_order_relaxed);
            }
            break;
        }
        case GST_EVENT_FLUSH_START:
            priv_of(self)->delivery.dropPending();
            break;
        case GST_EVENT_STREAM_START:
            // A new stream starts untagged; its tags follow this event.
            priv_of(self)->stream_orientation.store(GST_VIDEO_ORIENTATION_IDENTITY, std::memory_order_relaxed);
            break;
        default:
            break;
    }
    return GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class)->event(bsink, event);
}

// NULL->READY: build the GPU context from gpu-zerocopy, so the element maps GPU memory without any controller glue.
static gboolean gst_qgc_q_video_sink_start(GstBaseSink* bsink)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    const HwVideoBufferContext hwContext = HwBuffers::makeAdapterContext(priv_of(self)->gpu_zerocopy);
    GST_OBJECT_LOCK(self);
    priv_of(self)->hw_context = hwContext;
    GST_OBJECT_UNLOCK(self);

    GstBaseSinkClass* parentClass = GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class);
    return parentClass->start ? parentClass->start(bsink) : TRUE;
}

// READY->NULL: streaming has stopped, so drop all negotiated and per-stream state; a restart renegotiates from scratch
// and no undelivered frame keeps a decoder buffer alive.
static gboolean gst_qgc_q_video_sink_stop(GstBaseSink* bsink)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    PrivState* p = priv_of(self);
    p->caps_valid.store(false, std::memory_order_release);
    gst_clear_caps(&p->cached_caps);
    p->gpu_caps = false;
    p->path_session.stop();
    p->stream_orientation.store(GST_VIDEO_ORIENTATION_IDENTITY, std::memory_order_relaxed);
    p->delivery.dropPending();

    GstBaseSinkClass* parentClass = GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class);
    return parentClass->stop ? parentClass->stop(bsink) : TRUE;
}

// Add qgcqvideosink's consumed metas + min-buffer pool hint, then chain so GstBaseSink's default
// allocation bookkeeping still runs.
static gboolean gst_qgc_q_video_sink_propose_allocation(GstBaseSink* bsink, GstQuery* query)
{
    GstQgc::populateAllocationQuery(query);
    // GstBaseSink/GstVideoSink install no default propose_allocation vmethod, so the parent pointer
    // is NULL for a direct subclass; chaining unconditionally would call 0x0 on the first ALLOCATION query.
    GstBaseSinkClass* parentClass = GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class);
    if (parentClass->propose_allocation) {
        return parentClass->propose_allocation(bsink, query);
    }
    return TRUE;
}

// Answer CONTEXT (gst.gl.GLDisplay/app_context, Vulkan, D3D devices) synchronously so upstream GPU elements share Qt's
// RHI device; the bus NEED_CONTEXT fallback races state changes. ALLOCATION reaches propose_allocation via the parent.
static gboolean gst_qgc_q_video_sink_query(GstBaseSink* bsink, GstQuery* query)
{
    if ((GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) && HwBuffers::answerSinkBinContextQuery(query)) {
        return TRUE;
    }
    if (GST_QUERY_TYPE(query) == GST_QUERY_ACCEPT_CAPS) {
        // GstBaseSink wants a subset of get_caps, which rejects caps without a pixel-aspect-ratio (square pixels).
        GstCaps* caps = nullptr;
        gst_query_parse_accept_caps(query, &caps);
        GstCaps* const offered = offered_caps(GST_QGC_Q_VIDEO_SINK(bsink));
        gst_query_set_accept_caps_result(query, caps && gst_caps_can_intersect(caps, offered));
        gst_caps_unref(offered);
        return TRUE;
    }
    return GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class)->query(bsink, query);
}

static void gst_qgc_q_video_sink_class_init(GstQgcQVideoSinkClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);
    GstBaseSinkClass* basesink_class = GST_BASE_SINK_CLASS(klass);
    GstVideoSinkClass* videosink_class = GST_VIDEO_SINK_CLASS(klass);

    gobject_class->set_property = gst_qgc_q_video_sink_set_property;
    gobject_class->get_property = gst_qgc_q_video_sink_get_property;
    gobject_class->constructed = gst_qgc_q_video_sink_constructed;
    gobject_class->finalize = gst_qgc_q_video_sink_finalize;

    g_object_class_install_property(
        gobject_class, PROP_QVIDEOSINK,
        g_param_spec_pointer("qvideosink", "QVideoSink target",
                             "QVideoSink* to push frames into. Caller-owned; element never unrefs.",
                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        gobject_class, PROP_ACTIVE,
        g_param_spec_boolean("active", "Active",
                             "When FALSE, show_frame drops buffers instead of pushing to the QVideoSink.", TRUE,
                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        gobject_class, PROP_GPU_ZEROCOPY,
        g_param_spec_boolean("gpu-zerocopy", "GPU zero-copy",
                             "Import frames in GPU memory without a copy when a zero-copy path supports them; "
                             "FALSE maps every frame through the CPU.",
                             FALSE,
                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        gobject_class, PROP_FRAMES_INPUT,
        g_param_spec_uint64("frames-input", "Frames input", "Total buffers seen by show_frame, including drops.", 0,
                            G_MAXUINT64, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        gobject_class, PROP_FRAMES_DROPPED,
        g_param_spec_uint64("frames-dropped", "Frames dropped",
                            "Buffers rejected by show_frame (inactive sink, missing QVideoSink, or map failure). "
                            "Detailed map failures are tracked separately in GstHwPathTelemetry.",
                            0, G_MAXUINT64, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(
        gobject_class, PROP_FRAMES_DELIVERED,
        g_param_spec_uint64("frames-delivered", "Frames delivered",
                            "Buffers that survived every drop check and were handed to the QVideoSink. Delivery keeps "
                            "only the latest frame, so frames superseded before the GUI thread ran are counted too. "
                            "Per-element — the GUI controller reads this for the QML frameCount.",
                            0, G_MAXUINT64, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    // Not "stats": that is GstBaseSink's own property (average-rate, dropped, rendered), which generic tools read.
    g_object_class_install_property(
        gobject_class, PROP_FRAME_STATS,
        g_param_spec_boxed("frame-stats", "Frame statistics",
                           "This sink's own frame accounting, as a qgc-video-sink-stats structure of guint64 fields: "
                           "frames-<path> per delivery path (frames-cpu, frames-dmabuf, frames-gl, frames-d3d11, "
                           "frames-d3d12, frames-iosurface, frames-ahwbuf, frames-vulkan); gpu-fallbacks (frames a GPU "
                           "path was tried for but the CPU delivered), stream-demotions (caps epochs that fell back "
                           "from a GPU path), tiled-drops (frames in a tiled DMA_DRM layout that no GPU path "
                           "imported and the CPU can't read) and superseded-frames (delivered frames a newer one "
                           "replaced before the QVideoSink took them).",
                           GST_TYPE_STRUCTURE, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(element_class, "QGC QVideoSink", "Sink/Video",
                                          "Pushes decoded GstVideoFrames into a Qt QVideoSink",
                                          "QGroundControl <https://qgroundcontrol.com/>");
    gst_element_class_add_static_pad_template(element_class, &sink_template);

    basesink_class->get_caps = gst_qgc_q_video_sink_get_caps;
    basesink_class->set_caps = gst_qgc_q_video_sink_set_caps;
    basesink_class->event = gst_qgc_q_video_sink_event;
    basesink_class->start = gst_qgc_q_video_sink_start;
    basesink_class->stop = gst_qgc_q_video_sink_stop;
    basesink_class->propose_allocation = gst_qgc_q_video_sink_propose_allocation;
    basesink_class->query = gst_qgc_q_video_sink_query;
    videosink_class->show_frame = gst_qgc_q_video_sink_show_frame;
}

namespace GstQgc {

void configureSinkCaps(GstElement* sink, GpuBinMode gpuBin, bool squarePixels)
{
    if (!sink || !GST_IS_QGC_Q_VIDEO_SINK(sink)) {
        return;
    }
    GstQgcQVideoSink* const self = GST_QGC_Q_VIDEO_SINK(sink);
    GST_OBJECT_LOCK(self);
    priv_of(self)->gpu_bin = gpuBin;
    priv_of(self)->square_pixels = squarePixels;
    gst_clear_caps(&priv_of(self)->offered_caps);
    GST_OBJECT_UNLOCK(self);
}

}  // namespace GstQgc
