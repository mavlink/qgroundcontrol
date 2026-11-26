#include "gstqgcqvideosink.h"

#include <atomic>
#include <gst/video/video-info.h>
#include <gst/video/video.h>
#include <memory>
#include <new>
#include <optional>
#include <utility>

#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QtEnvironmentVariables>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "GstQgcFrameDelivery.h"
#include "GstQgcFrameMap.h"
#include "GstQgcSinkPolicy.h"
#include "GstQgcSinkStats.h"
#include "GstQgcVideoFormats.h"
#include "GstScoped.h"
#include "HwBuffers/HwBuffers.h"
#include "QGCLoggingCategory.h"
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include <gst/gl/gstglsyncmeta.h>
#endif

QGC_LOGGING_CATEGORY(GstQgcQVideoSinkLog, "Video.GStreamer.QgcQVideoSink")

namespace {

/// The element's C++ state, placement-constructed in instance_init and destroyed in finalize.
struct PrivState
{
    // The QVideoSink frames go to, set by the controller on the GUI thread.
    GstQgc::FrameDelivery delivery;
    GstQgc::SinkCounters counters;
    // Read lock-free by show_frame.
    std::atomic<bool> active{true};
    // Set by makeVideoSink() before the sink is shared, so show_frame reads it without the lock.
    bool gpu_zerocopy = false;
    // Set by the sink bin before linking (configureSinkCaps); with gpu_zerocopy, they decide what get_caps offers.
    GstQgc::GpuBinMode gpu_bin = GstQgc::GpuBinMode::Passthrough;
    bool square_pixels = true;
    // Read in place of the process's environment once set (setSinkEnvironment); under GST_OBJECT_LOCK.
    std::optional<GstQgc::SinkEnvironment> environment;
    // What get_caps last offered, and for which policy; under GST_OBJECT_LOCK. Rebuilt when the policy changes, e.g.
    // the render target or the DMABuf import state: building may query EGL.
    GstCaps* offered_caps = nullptr;
    GstQgc::SinkCapsPolicy offered_policy;
    // Negotiated state: set_caps/stop write it and show_frame reads it, all serialised by GstBaseSink.
    GstVideoInfo video_info = {};
    QVideoFrameFormat format;
    // The geometry of the last buffer and format for frames of it. Buffers of one stream share theirs, so show_frame
    // rebuilds the format only when a buffer's differs; set_caps starts from the caps' own.
    FrameGeometry frame_geometry;
    QVideoFrameFormat frame_format;
    // frame_geometry's pictureSize, for sinkPictureSize().
    std::atomic<int> picture_width{0};
    std::atomic<int> picture_height{0};
    // Whether the negotiated caps carry a GPU memory feature. Without one, show_frame skips path selection for frames
    // in plain system memory.
    bool gpu_caps = false;
    // GstVideoOrientationMethod from the stream's image-orientation tag.
    std::atomic<int> stream_orientation{GST_VIDEO_ORIENTATION_IDENTITY};
    // Negotiated caps held from set_caps, null before it and after stop; avoids per-frame allocation and preserves
    // DRM modifiers.
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

/// The state of @p element, when it is a qgcqvideosink; null otherwise.
PrivState* sink_state(GstElement* element)
{
    return (element && GST_IS_QGC_Q_VIDEO_SINK(element)) ? priv_of(GST_QGC_Q_VIDEO_SINK(element)) : nullptr;
}

/// What the sink reads now: the environment it was given (setSinkEnvironment()), else the process's. Either is
/// without its DMABuf display while an import failure has that path disabled.
GstQgc::SinkEnvironment environment_now(GstQgcQVideoSink* self)
{
    GST_OBJECT_LOCK(self);
    std::optional<GstQgc::SinkEnvironment> environment = priv_of(self)->environment;
    GST_OBJECT_UNLOCK(self);
    if (!environment) {
        environment = GstQgc::currentSinkEnvironment();
    }
    if (HwBuffers::dmaBufImportDisabled()) {
        environment->dmaBufDisplay = nullptr;
    }
    return *environment;
}

/// The caps policy under @p environment; under GST_OBJECT_LOCK. The environment doesn't decide the caps of a CPU sink
/// or of the glupload and d3d12convert variants, so it is left out there: equal policies offer equal caps.
GstQgc::SinkCapsPolicy caps_policy_locked(const PrivState* p, const GstQgc::SinkEnvironment& environment)
{
    const bool followsRenderer = p->gpu_zerocopy && (p->gpu_bin != GstQgc::GpuBinMode::GlUpload) &&
                                 (p->gpu_bin != GstQgc::GpuBinMode::D3D12Convert);
    return {.gpuZeroCopy = p->gpu_zerocopy,
            .gpuBin = p->gpu_bin,
            .target = followsRenderer ? environment.target : GstQgc::RenderTarget{},
            .dmaBufDisplay = followsRenderer ? environment.dmaBufDisplay : nullptr,
            .d3d11OpensD3D12 = followsRenderer && environment.d3d11OpensD3D12,
            .squarePixels = p->square_pixels};
}

/// The caps get_caps offers now (transfer-full); rebuilt only when the policy differs from the last call's.
GstCaps* offered_caps(GstQgcQVideoSink* self)
{
    PrivState* const p = priv_of(self);
    const GstQgc::SinkEnvironment environment = environment_now(self);
    const GstQgc::RenderTarget& target = environment.target;
    GST_OBJECT_LOCK(self);
    const GstQgc::SinkCapsPolicy policy = caps_policy_locked(p, environment);
    if (p->offered_caps && (p->offered_policy == policy)) {
        GstCaps* const cached = gst_caps_ref(p->offered_caps);
        GST_OBJECT_UNLOCK(self);
        return cached;
    }
    GST_OBJECT_UNLOCK(self);

    GstCaps* caps = gst_caps_from_string(GstQgc::buildSinkCaps(policy).c_str());
    if (!caps) {
        qCCritical(GstQgcQVideoSinkLog) << "sink caps did not parse";
        return gst_caps_new_empty();
    }
    qCDebug(GstQgcQVideoSinkLog) << "offering" << GStreamer::adoptGChar(gst_caps_to_string(caps)).get()
                                 << "for render backend" << GstQgc::toString(target.backend) << "EGL" << target.egl
                                 << "DMABuf import" << (policy.dmaBufDisplay != nullptr) << "D3D12 import"
                                 << policy.d3d11OpensD3D12;
    GST_OBJECT_LOCK(self);
    gst_caps_replace(&p->offered_caps, caps);
    p->offered_policy = policy;
    GST_OBJECT_UNLOCK(self);
    return caps;
}

/// The caps policy now. Asks upstream to renegotiate when it differs from the one get_caps last offered caps for: the
/// render target changed (the scene graph came up after negotiation, or Qt fell back to another graphics API), the
/// DMABuf path started or stopped importing (Qt's EGLDisplay became known, or an import failure disabled the path), or
/// Qt's Direct3D 11 device turned out to open D3D12 resources.
GstQgc::SinkCapsPolicy reconfigure_on_policy_change(GstQgcQVideoSink* self)
{
    PrivState* const p = priv_of(self);
    const GstQgc::SinkEnvironment environment = environment_now(self);
    const GstQgc::RenderTarget& target = environment.target;
    GST_OBJECT_LOCK(self);
    const GstQgc::SinkCapsPolicy policy = caps_policy_locked(p, environment);
    const bool changed = p->offered_caps && (p->offered_policy != policy);
    if (changed) {
        // Rebuilt by the next get_caps, which also re-arms this check.
        gst_clear_caps(&p->offered_caps);
    }
    GST_OBJECT_UNLOCK(self);
    if (changed) {
        qCDebug(GstQgcQVideoSinkLog) << "render backend now" << GstQgc::toString(target.backend) << "EGL" << target.egl
                                     << "DMABuf import" << (policy.dmaBufDisplay != nullptr) << "D3D12 import"
                                     << policy.d3d11OpensD3D12 << "- asking upstream to renegotiate";
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

/// Where @p mapped, which has no frame, ended up; for the element error. Its gpuPath is None whatever was tried.
const char* map_failure_stage(const MappedFrame& mapped)
{
    if (mapped.noCpuLayout) {
        return "tiled layout that no GPU path imported";
    }
    return mapped.demoted ? "CPU fallback from a GPU path" : "CPU path";
}

}  // namespace

/// get_caps narrows this to the formats and memory the sink takes for its render target.
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw(ANY)"));

G_DEFINE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST_TYPE_VIDEO_SINK)

static void gst_qgc_q_video_sink_init(GstQgcQVideoSink* self)
{
    new (&self->state) PrivState();
    gst_video_info_init(&self->state.video_info);
    // Unsynced until the receiver decides per stream (DecodingBranch syncs a live source unless low latency is asked
    // for). async=FALSE skips preroll wait so caps changes don't stall.
    gst_base_sink_set_sync(GST_BASE_SINK(self), FALSE);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(self), FALSE);
    // A frame that comes late for the clock is shown at once. GstVideoSink would drop it instead, when it is already
    // decoded and uploaded: a computer that falls behind would decode every frame and show one a second.
    gst_base_sink_set_max_lateness(GST_BASE_SINK(self), -1);
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
    p->frame_geometry = {.frameSize = QSize(w, h), .picture = QRect(0, 0, w, h), .pictureSize = QSize(w, h)};
    p->picture_width.store(w, std::memory_order_relaxed);
    p->picture_height.store(h, std::memory_order_relaxed);
    p->frame_format = p->format;
    gst_clear_caps(&p->cached_caps);
    // Hold the negotiated caps verbatim: rebuilding from video_info drops DRM modifiers and other
    // negotiated fields the downstream frame mapping relies on.
    p->cached_caps = gst_caps_ref(caps);
    p->gpu_caps = caps_carry_gpu_memory(caps);
    p->path_session.startEpoch();

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

    if (!p->cached_caps) {
        // Should never happen — GstBaseSink calls set_caps before show_frame.
        return GST_FLOW_NOT_NEGOTIATED;
    }
    if (!p->active.load(std::memory_order_relaxed)) {
        p->counters.noteDropped();
        return GST_FLOW_OK;  // drop silently — controller drives the active flag
    }

    // No mapping when no QVideoSink would take the frame. Only the pointer is read: the sink's owner thread may be
    // destroying it.
    if (!p->delivery.sink()) {
        p->counters.noteDropped();
        return GST_FLOW_OK;  // no destination yet; drop
    }
    // Software-decoded frames skip GPU selection. Caps alone can't tell: GPU memory also arrives under plain
    // video/x-raw (vtdec's CoreVideo/IOSurface buffers, V4L2 dmabuf export), so frames in other memory still try it.
    const HwVideoBufferContext hwCtx{.gpuEnabled = p->gpu_zerocopy && (p->gpu_caps || !nothing_to_import(buf)),
                                     .dmaBufImporter = policy.dmaBufDisplay};

    if (const FrameGeometry geometry = frameGeometry(buf, p->video_info); geometry != p->frame_geometry) {
        p->frame_geometry = geometry;
        p->frame_format = withFrameGeometry(p->format, geometry);
        p->picture_width.store(geometry.pictureSize.width(), std::memory_order_relaxed);
        p->picture_height.store(geometry.pictureSize.height(), std::memory_order_relaxed);
    }
    MappedFrame mapped =
        mapSampleToFrame(buf, p->cached_caps, p->video_info, p->frame_format, hwCtx, p->path_session.pathCache());
    if (!mapped.frame.isValid()) {
        p->counters.noteDropped();
        if (p->path_session.onMapFailed(mapped, p->counters) == GstQgc::PathSession::Verdict::Error) {
            const quint64 failures = p->path_session.consecutiveMapFailures();
            const char* const format = gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&p->video_info));
            // Without an element error upstream reports only a generic "Internal data stream error".
            GST_ELEMENT_ERROR(self, STREAM, FAILED, ("Video frames could not be mapped for display"),
                              ("%" G_GUINT64_FORMAT " consecutive map failures (%s, format %s)",
                               static_cast<guint64>(failures), map_failure_stage(mapped), format ? format : "unknown"));
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;  // drop transient failure, keep the stream alive
    }
    p->path_session.onMapped();

    applyOrientationAndTiming(mapped.frame, buf, p->stream_orientation.load(std::memory_order_relaxed));
    applyFlips(mapped.frame, p->frame_geometry);

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

// READY->NULL: streaming has stopped, so drop all negotiated and per-stream state; a restart renegotiates from scratch
// and no undelivered frame keeps a decoder buffer alive.
static gboolean gst_qgc_q_video_sink_stop(GstBaseSink* bsink)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    PrivState* p = priv_of(self);
    gst_clear_caps(&p->cached_caps);
    p->gpu_caps = false;
    p->path_session.stop();
    p->stream_orientation.store(GST_VIDEO_ORIENTATION_IDENTITY, std::memory_order_relaxed);
    p->delivery.dropPending();

    GstBaseSinkClass* parentClass = GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class);
    return parentClass->stop ? parentClass->stop(bsink) : TRUE;
}

namespace GstQgc {

namespace {

// The decoder buffers one sink's frames hold in steady flow: the frame being mapped, the one waiting for the GUI thread
// (FrameDelivery keeps only the latest), the frame Qt shows, the one before it, whose textures keep it until Qt reuses
// that in-flight slot (QRhi FramesInFlight is 2 on the common backends), and the textures Qt moves out of a reused slot
// and keeps until that render frame ends. Qt lets each go without waiting for a newer frame.
constexpr guint kProposedMinBuffers = 5;

// QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS overrides kProposedMinBuffers, like Qt's
// QT_GSTREAMER_PROPOSE_ALLOCATION_MIN_BUFFERS, e.g. to save CMA memory with 4K decoders on i.MX or Raspberry Pi.
guint proposedMinBuffers()
{
    bool ok = false;
    const int minBuffers = qEnvironmentVariableIntValue("QGC_GST_PROPOSE_ALLOCATION_MIN_BUFFERS", &ok);
    return (ok && (minBuffers > 0)) ? static_cast<guint>(minBuffers) : kProposedMinBuffers;
}

// Advertise every meta API qgcqvideosink/GstQgcFrameMap consume so upstream keeps them;
// else gst-vaapi/v4l2 crop into a copy of each frame instead of attaching the crop meta.
void addConsumedAllocationMetas(GstQuery* query, bool cropMeta = true)
{
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    if (cropMeta) {
        gst_query_add_allocation_meta(query, GST_VIDEO_CROP_META_API_TYPE, NULL);
    }
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    // As upstream's Qt sink asks: glcolorconvert then ends each frame it renders with a sync point, which flushes its
    // context. GstGlVideoBuffer doesn't wait on that fence: it sets one of its own once the frame is mapped.
    gst_query_add_allocation_meta(query, GST_GL_SYNC_META_API_TYPE, NULL);
#endif
}

// A GstVideoBufferPool, unlike a plain GstBufferPool, supports VIDEO_META and attaches GstVideoMeta per buffer, so
// decoders that check the option (avdec direct rendering) decode straight into it instead of copying.
bool tryProposeMetaPool(GstQuery* query, GstCaps* caps, gsize size, guint minBuffers)
{
    const std::unique_ptr<GstBufferPool, GStreamer::GstObjectDeleter> pool{gst_video_buffer_pool_new()};
    if (!pool) {
        return false;
    }

    GstStructure* config = gst_buffer_pool_get_config(pool.get());
    gst_buffer_pool_config_set_params(config, caps, size, minBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (!gst_buffer_pool_set_config(pool.get(), config)) {
        return false;
    }
    gst_query_add_allocation_pool(query, pool.get(), size, minBuffers, 0);
    return true;
}

}  // namespace

bool populateAllocationQuery(GstQuery* query)
{
    GstCaps* caps = nullptr;
    gboolean need_pool = FALSE;
    gst_query_parse_allocation(query, &caps, &need_pool);
    if (!caps) {
        return false;
    }

    GstVideoInfo vinfo;
    if (!GstHw::dmaDrmAwareVideoInfo(caps, &vinfo)) {
        return false;
    }
    const gsize size = GST_VIDEO_INFO_SIZE(&vinfo);
    const guint minBuffers = proposedMinBuffers();

    GstCapsFeatures* features = gst_caps_get_features(caps, 0);
    const bool is_system_memory =
        !features || gst_caps_features_is_equal(features, GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY);

    if (!is_system_memory) {
        if (!HwBuffers::proposeDeviceBoundPool(query, caps, &vinfo, size, minBuffers)) {
            gst_query_add_allocation_pool(query, NULL, size, minBuffers, 0);
        }
        // A d3d11 or d3d12 decoder offered crop meta hands over its whole texture when the picture starts off the
        // texture's origin, with the meta saying where; the D3D paths copy the picture from the origin. Without the
        // offer the decoder crops such a picture itself.
        const bool d3dMemory = gst_caps_features_contains(features, "memory:D3D11Memory") ||
                               gst_caps_features_contains(features, "memory:D3D12Memory");
        addConsumedAllocationMetas(query, !d3dMemory);
        return true;
    }

    // The entry carries the buffers the sink holds, which a producer with a pool of its own adds to that pool's size:
    // it goes in without a pool when none is asked for, as in upstream's sinks.
    if (!need_pool || !tryProposeMetaPool(query, caps, size, minBuffers)) {
        gst_query_add_allocation_pool(query, NULL, size, minBuffers, 0);
    }
    addConsumedAllocationMetas(query);
    return true;
}

}  // namespace GstQgc

// Adds qgcqvideosink's consumed metas and min-buffer pool hint. A query without video caps fails, as in upstream's
// sinks.
static gboolean gst_qgc_q_video_sink_propose_allocation(GstBaseSink* bsink, GstQuery* query)
{
    if (!GstQgc::populateAllocationQuery(query)) {
        return FALSE;
    }
    // GstBaseSink/GstVideoSink install no default propose_allocation vmethod, so the parent pointer
    // is NULL for a direct subclass; chaining unconditionally would call 0x0 on the first ALLOCATION query.
    GstBaseSinkClass* parentClass = GST_BASE_SINK_CLASS(gst_qgc_q_video_sink_parent_class);
    if (parentClass->propose_allocation) {
        return parentClass->propose_allocation(bsink, query);
    }
    return TRUE;
}

// ALLOCATION reaches propose_allocation via the parent.
static gboolean gst_qgc_q_video_sink_query(GstBaseSink* bsink, GstQuery* query)
{
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

    gobject_class->constructed = gst_qgc_q_video_sink_constructed;
    gobject_class->finalize = gst_qgc_q_video_sink_finalize;

    gst_element_class_set_static_metadata(element_class, "QGC QVideoSink", "Sink/Video",
                                          "Pushes decoded GstVideoFrames into a Qt QVideoSink",
                                          "QGroundControl <https://qgroundcontrol.com/>");
    gst_element_class_add_static_pad_template(element_class, &sink_template);

    basesink_class->get_caps = gst_qgc_q_video_sink_get_caps;
    basesink_class->set_caps = gst_qgc_q_video_sink_set_caps;
    basesink_class->event = gst_qgc_q_video_sink_event;
    basesink_class->stop = gst_qgc_q_video_sink_stop;
    basesink_class->propose_allocation = gst_qgc_q_video_sink_propose_allocation;
    basesink_class->query = gst_qgc_q_video_sink_query;
    videosink_class->show_frame = gst_qgc_q_video_sink_show_frame;
}

namespace GstQgc {

GstElement* makeVideoSink(bool gpuZeroCopy)
{
    // From its type: QGC alone makes the sink, so no plugin registers it and it has no element factory.
    GstQgcQVideoSink* const self = GST_QGC_Q_VIDEO_SINK(g_object_new(GST_TYPE_QGC_Q_VIDEO_SINK, nullptr));
    priv_of(self)->gpu_zerocopy = gpuZeroCopy;
    return GST_ELEMENT(self);
}

bool sinkGpuZeroCopy(GstElement* sink)
{
    const PrivState* const p = sink_state(sink);
    return p && p->gpu_zerocopy;
}

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

void setSinkEnvironment(GstElement* sink, const SinkEnvironment& environment)
{
    if (!sink || !GST_IS_QGC_Q_VIDEO_SINK(sink)) {
        return;
    }
    GstQgcQVideoSink* const self = GST_QGC_Q_VIDEO_SINK(sink);
    // The caps offered so far stay: the next frame compares their policy with the new one and asks for renegotiation.
    GST_OBJECT_LOCK(self);
    priv_of(self)->environment = environment;
    GST_OBJECT_UNLOCK(self);
}

void setSinkTarget(GstElement* sink, QVideoSink* target)
{
    if (PrivState* const p = sink_state(sink)) {
        p->delivery.setSink(target);
    }
}

QVideoSink* sinkTarget(GstElement* sink)
{
    const PrivState* const p = sink_state(sink);
    return p ? p->delivery.sink() : nullptr;
}

void setSinkActive(GstElement* sink, bool active)
{
    if (PrivState* const p = sink_state(sink)) {
        p->active.store(active, std::memory_order_relaxed);
    }
}

bool sinkActive(GstElement* sink)
{
    const PrivState* const p = sink_state(sink);
    return p && p->active.load(std::memory_order_relaxed);
}

QSize sinkPictureSize(GstElement* sink)
{
    const PrivState* const p = sink_state(sink);
    if (!p) {
        return {};
    }
    const QSize size(p->picture_width.load(std::memory_order_relaxed),
                     p->picture_height.load(std::memory_order_relaxed));
    return size.isEmpty() ? QSize() : size;
}

SinkStats sinkStats(GstElement* sink)
{
    const PrivState* const p = sink_state(sink);
    return p ? p->counters.snapshot() : SinkStats{};
}

}  // namespace GstQgc
