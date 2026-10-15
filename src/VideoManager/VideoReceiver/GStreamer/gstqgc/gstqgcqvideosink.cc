#include "gstqgcqvideosink.h"

#include <array>
#include <atomic>
#include <gst/video/video-info.h>
#include <gst/video/video.h>
#include <memory>
#include <new>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QMetaObject>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtMultimedia/QVideoFrame>
#include <QtMultimedia/QVideoFrameFormat>
#include <QtMultimedia/QVideoSink>

#include "GstQgcAllocation.h"
#include "GstQgcFrameMap.h"
#include "HwBuffers/common/GstHwVideoInfo.h"
#include "HwBuffers/common/HwBuffers.h"
#include "QGCLoggingCategory.h"
#include "gstqgcelements.h"
#include "gstqgcvideosinkbin.h"

QGC_LOGGING_CATEGORY(GstQgcQVideoSinkLog, "Video.GStreamer.QgcQVideoSink")

#define GST_CAT_DEFAULT gst_qgc_debug

namespace {

/// Latest-frame-only hand-off to the GUI thread. At most one invoke is queued (`posted`); the GUI side takes whatever
/// frame is newest when it runs, so a stalled GUI thread never accumulates frames (or the decoder buffers they pin).
struct PendingFrameSlot
{
    QMutex mutex;
    QVideoFrame frame;
    bool posted = false;
};

QVideoFrame take_pending_frame(PendingFrameSlot& slot)
{
    QMutexLocker locker(&slot.mutex);
    return std::exchange(slot.frame, QVideoFrame());
}

/// The element's C++ state, placement-constructed in instance_init and destroyed in finalize.
struct PrivState
{
    // Caller-owned and never unref'd; written by the controller on the GUI thread under GST_OBJECT_LOCK.
    QVideoSink* qvideosink = nullptr;
    // Read lock-free by show_frame.
    std::atomic<bool> active{true};
    // start() derives hw_context from it.
    bool gpu_zerocopy = false;
    // Negotiated state: set_caps/stop write it and show_frame reads it, all serialised by GstBaseSink.
    std::atomic<bool> caps_valid{false};
    GstVideoInfo video_info = {};
    QVideoFrameFormat format;
    // Written by start() under GST_OBJECT_LOCK and snapshotted by show_frame.
    HwVideoBufferContext hw_context = {};
    // Whether the negotiated caps carry a GPU memory feature. Without one, show_frame skips path selection for frames
    // in plain system memory.
    bool gpu_caps = false;
    std::atomic<int64_t> last_pts_ns{-1};
    std::atomic<quint64> input_frames{0};
    std::atomic<quint64> dropped_frames{0};
    std::atomic<quint64> consecutive_map_failures{0};  // sustained run escalates show_frame to error
    // Per-element render counter (read via `frames-delivered`) so multi-receiver setups
    // don't see a shared process-global total.
    std::atomic<quint64> delivered_frames{0};
    // Delivered frames per path (None counts CPU frames), frames a GPU path was tried for but delivered through the
    // CPU, and caps epochs that started on a GPU path and fell back to the CPU; reported through "frame-stats".
    std::array<std::atomic<quint64>, GstHw::kPathCount> path_frames{};
    std::atomic<quint64> gpu_fallbacks{0};
    std::atomic<quint64> stream_demotions{0};
    // One stream demotion per caps epoch; reset with the path cache.
    bool demotion_recorded = false;
    // Live zero-copy CPU frames pinning this sink's decoder buffers; shared because Qt may hold frames past finalize.
    CpuVideoFramePool::PinnedFrameCounter pinned_cpu_frames = std::make_shared<std::atomic<int>>(0);
    // Replaced (under GST_OBJECT_LOCK) whenever `qvideosink` changes: an invoke queued to a destroyed QVideoSink is
    // purged without running, which would otherwise leave `posted` latched and starve the next sink.
    std::shared_ptr<PendingFrameSlot> pending = std::make_shared<PendingFrameSlot>();
    // Negotiated caps held from set_caps; avoids per-frame allocation and preserves DRM modifiers.
    GstCaps* cached_caps{nullptr};
    // Per-caps-epoch resolved-path cache; reset on set_caps so a format change re-runs path selection.
    HwResolvedPathCache resolved_path_cache = {};
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
    const bool hasSink = (priv_of(self)->qvideosink != nullptr);
    hwOut = priv_of(self)->hw_context;
    GST_OBJECT_UNLOCK(self);
    return hasSink;
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

constexpr const char* kGpuFallbacksField = "gpu-fallbacks";
constexpr const char* kStreamDemotionsField = "stream-demotions";

/// The "frame-stats" property value (transfer-full).
GstStructure* make_stats(const PrivState& p)
{
    GstStructure* const stats = gst_structure_new_empty("qgc-video-sink-stats");
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        gst_structure_set(stats, GstQgc::sinkFramesField(HwVideoBufferPath(i)), G_TYPE_UINT64,
                          static_cast<guint64>(p.path_frames[i].load(std::memory_order_relaxed)), nullptr);
    }
    gst_structure_set(stats, kGpuFallbacksField, G_TYPE_UINT64,
                      static_cast<guint64>(p.gpu_fallbacks.load(std::memory_order_relaxed)), kStreamDemotionsField,
                      G_TYPE_UINT64, static_cast<guint64>(p.stream_demotions.load(std::memory_order_relaxed)), nullptr);
    return stats;
}

/// Releases an undelivered frame (and the decoder buffer it pins); a queued invoke then finds the slot empty.
void drop_pending_frame(GstQgcQVideoSink* self)
{
    QVideoFrame dropped;
    GST_OBJECT_LOCK(self);
    dropped = take_pending_frame(*priv_of(self)->pending);
    GST_OBJECT_UNLOCK(self);
}

/// Stores @p frame as the latest pending frame and posts a GUI-thread invoke only if none is queued. Returns false
/// when no QVideoSink is bound. Runs under GST_OBJECT_LOCK: the controller clears `qvideosink` under the same lock
/// (destroyed-handler/setVideoSink on the GUI thread), so the clear cannot interleave between the null-check and the
/// post; an invoke posted while the destroyed-handler blocks here is purged by ~QObject's removePostedEvents.
bool push_frame_queued(GstQgcQVideoSink* self, QVideoFrame&& frame)
{
    QVideoFrame superseded;
    GST_OBJECT_LOCK(self);
    QVideoSink* const sink = priv_of(self)->qvideosink;
    if (!sink) {
        GST_OBJECT_UNLOCK(self);
        return false;
    }
    const std::shared_ptr<PendingFrameSlot> slot = priv_of(self)->pending;
    bool needPost = false;
    {
        QMutexLocker locker(&slot->mutex);
        superseded = std::exchange(slot->frame, std::move(frame));
        needPost = !std::exchange(slot->posted, true);
    }
    if (needPost) {
        QMetaObject::invokeMethod(
            sink,
            [sink, slot]() {
                QVideoFrame latest;
                {
                    QMutexLocker locker(&slot->mutex);
                    latest = std::exchange(slot->frame, QVideoFrame());
                    slot->posted = false;
                }
                if (latest.isValid()) {
                    sink->setVideoFrame(latest);
                }
            },
            Qt::QueuedConnection);
    }
    GST_OBJECT_UNLOCK(self);
    return true;
}

}  // namespace

/// Pad template — sink accepts any video caps. The downstream conversion
/// (videoconvert/glupload) already happens upstream in qgcvideosinkbin.
static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

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
    self->state.~PrivState();
    G_OBJECT_CLASS(gst_qgc_q_video_sink_parent_class)->finalize(obj);
}

static void gst_qgc_q_video_sink_set_property(GObject* obj, guint id, const GValue* val, GParamSpec* pspec)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(obj);
    PrivState* p = priv_of(self);
    QVideoFrame undelivered;
    GST_OBJECT_LOCK(self);
    switch (id) {
        case PROP_QVIDEOSINK: {
            auto* const target = static_cast<QVideoSink*>(g_value_get_pointer(val));
            if (target != p->qvideosink) {
                // A frame bound for the previous sink must not reach the new one; a fresh slot also detaches any
                // invoke still queued to the old sink.
                undelivered = take_pending_frame(*p->pending);
                p->pending = std::make_shared<PendingFrameSlot>();
            }
            p->qvideosink = target;
            // Reset PTS guard on sink swap so a new sink doesn't see a stale last_pts_ns
            // from the previous sink and erroneously drop the first frames.
            p->last_pts_ns.store(-1, std::memory_order_relaxed);
            break;
        }
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
            g_value_set_pointer(val, priv_of(self)->qvideosink);
            break;
        case PROP_ACTIVE:
            g_value_set_boolean(val, priv_of(self)->active.load(std::memory_order_relaxed));
            break;
        case PROP_GPU_ZEROCOPY:
            g_value_set_boolean(val, priv_of(self)->gpu_zerocopy);
            break;
        case PROP_FRAMES_INPUT:
            g_value_set_uint64(val, priv_of(self)->input_frames.load(std::memory_order_relaxed));
            break;
        case PROP_FRAMES_DROPPED:
            g_value_set_uint64(val, priv_of(self)->dropped_frames.load(std::memory_order_relaxed));
            break;
        case PROP_FRAMES_DELIVERED:
            g_value_set_uint64(val, priv_of(self)->delivered_frames.load(std::memory_order_relaxed));
            break;
        case PROP_FRAME_STATS:
            g_value_take_boxed(val, make_stats(*priv_of(self)));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, id, pspec);
            break;
    }
    GST_OBJECT_UNLOCK(self);
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
    p->resolved_path_cache = HwResolvedPathCache{};
    p->demotion_recorded = false;
    // New caps = new segment; clear PTS history so a restart/format change that resumes at a
    // lower PTS isn't wedged by the monotonic-PTS guard in show_frame.
    p->last_pts_ns.store(-1, std::memory_order_relaxed);
    p->caps_valid.store(true, std::memory_order_release);

    // Posted on the bus so the controller can mirror negotiation state into Q_PROPERTY.
    GstStructure* const capsInfo =
        gst_structure_new(GstQgc::kCapsInfoMessage, "width", G_TYPE_INT, w, "height", G_TYPE_INT, h, "format",
                          G_TYPE_STRING, gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&parsedInfo)), nullptr);
    gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), capsInfo));

    qCInfo(GstQgcQVideoSinkLog).noquote()
        << "set_caps: format=" << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&parsedInfo)) << w << "x" << h
        << "pixfmt=" << int(pixelFormat);
    return TRUE;
}

// Sustained run of map failures (not a transient hiccup) means the import is broken — tear down + restart.
constexpr quint64 kMaxConsecutiveMapFailures = 120;

// A backward PTS jump larger than this is a new timeline (source restart, loop), not reordered output to drop.
constexpr int64_t kPtsResyncThresholdNs = static_cast<int64_t>(GST_SECOND);

static GstFlowReturn gst_qgc_q_video_sink_show_frame(GstVideoSink* vsink, GstBuffer* buf)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(vsink);
    PrivState* p = priv_of(self);

    p->input_frames.fetch_add(1, std::memory_order_relaxed);

    if (!p->caps_valid.load(std::memory_order_acquire)) {
        // Should never happen — GstBaseSink calls set_caps before show_frame.
        return GST_FLOW_NOT_NEGOTIATED;
    }
    if (!p->active.load(std::memory_order_relaxed)) {
        p->dropped_frames.fetch_add(1, std::memory_order_relaxed);
        return GST_FLOW_OK;  // drop silently — controller drives the active flag
    }

    HwVideoBufferContext hwCtx;
    if (!snapshot_sink(self, hwCtx)) {
        p->dropped_frames.fetch_add(1, std::memory_order_relaxed);
        return GST_FLOW_OK;  // no destination yet; drop
    }
    // Software-decoded frames skip GPU selection. Caps alone can't tell: GPU memory also arrives under plain
    // video/x-raw (vtdec's CoreVideo/IOSurface buffers, V4L2 dmabuf export), so frames in other memory still try it.
    hwCtx.gpuEnabled = hwCtx.gpuEnabled && (p->gpu_caps || !nothing_to_import(buf));

    // PTS regression guard ahead of mapping: a regressed timestamp wedges QVideoOutput's internal
    // advance, and checking first avoids a wasted full-frame map on a buffer we'd drop anyway.
    // last_pts_ns is advanced only once the frame is actually delivered (below) so a transient map
    // failure doesn't push it past a buffer we never rendered. DISCONT and large jumps resync instead.
    const bool hasPts = GST_BUFFER_PTS_IS_VALID(buf);
    const int64_t pts = hasPts ? static_cast<int64_t>(GST_BUFFER_PTS(buf)) : -1;
    if (hasPts) {
        const int64_t lastPts = p->last_pts_ns.load(std::memory_order_acquire);
        const bool resync =
            GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DISCONT) || (lastPts - pts) > kPtsResyncThresholdNs;
        if (lastPts >= 0 && pts < lastPts && !resync) {
            p->dropped_frames.fetch_add(1, std::memory_order_relaxed);
            return GST_FLOW_OK;
        }
    }

    // Build a cropped format copy only when crop meta is present (rare); otherwise pass
    // p->format by reference to avoid a per-frame QVideoFrameFormat refcount bump.
    QVideoFrameFormat croppedFmt;
    const bool hasCrop = (gst_buffer_get_video_crop_meta(buf) != nullptr);
    if (hasCrop) {
        croppedFmt = applyCropMeta(p->format, buf);
    }
    MappedFrame mapped = mapSampleToFrame(buf, p->cached_caps, p->video_info, hasCrop ? croppedFmt : p->format, hwCtx,
                                          p->pinned_cpu_frames, &p->resolved_path_cache);
    if (!mapped.frame.isValid()) {
        p->dropped_frames.fetch_add(1, std::memory_order_relaxed);
        const quint64 c = p->consecutive_map_failures.fetch_add(1, std::memory_order_relaxed) + 1;
        if ((c & 0x3F) == 1) {
            qCWarning(GstQgcQVideoSinkLog) << "mapping failed, consecutive=" << c;
        }
        if (c >= kMaxConsecutiveMapFailures) {
            const char* const format = gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&p->video_info));
            qCWarning(GstQgcQVideoSinkLog) << c << "consecutive map failures — erroring out";
            // Without an element error upstream reports only a generic "Internal data stream error".
            GST_ELEMENT_ERROR(self, STREAM, FAILED, ("Video frames could not be mapped for display"),
                              ("%" G_GUINT64_FORMAT " consecutive map failures (path %s, format %s)",
                               static_cast<guint64>(c), GstHw::toString(mapped.gpuPath), format ? format : "unknown"));
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;  // drop transient failure, keep the stream alive
    }
    p->consecutive_map_failures.store(0, std::memory_order_relaxed);

    // Stream orientation is always IDENTITY here; tag-driven orientation lives in the
    // controller (GST_TAG_IMAGE_ORIENTATION). Per-buffer GstVideoOrientationMeta still wins.
    applyOrientationAndTiming(mapped.frame, buf, static_cast<int>(GST_VIDEO_ORIENTATION_IDENTITY));

    const QSize frameSize = mapped.frame.size();
    if (!push_frame_queued(self, std::move(mapped.frame))) {
        p->dropped_frames.fetch_add(1, std::memory_order_relaxed);
        return GST_FLOW_OK;  // QVideoSink unbound while mapping
    }

    p->path_frames[static_cast<std::size_t>(mapped.gpuPath)].fetch_add(1, std::memory_order_relaxed);
    if (mapped.demoted) {
        p->gpu_fallbacks.fetch_add(1, std::memory_order_relaxed);
        if (!std::exchange(p->demotion_recorded, true)) {
            p->stream_demotions.fetch_add(1, std::memory_order_relaxed);
        }
    }

    const quint64 delivered = p->delivered_frames.fetch_add(1, std::memory_order_relaxed) + 1;
    if (delivered == 1) {
        qCInfo(GstQgcQVideoSinkLog).noquote()
            << "first frame delivered via" << GstHw::toString(mapped.gpuPath) << "path"
            << QStringLiteral("%1x%2").arg(frameSize.width()).arg(frameSize.height());
    } else if ((delivered % 300) == 0) {
        const auto& cpuFrames = p->path_frames[static_cast<std::size_t>(HwVideoBufferPath::None)];
        qCDebug(GstQgcQVideoSinkLog).noquote()
            << "frame flow:" << GstHw::toString(mapped.gpuPath) << "delivered=" << delivered
            << "input=" << p->input_frames.load(std::memory_order_relaxed)
            << "dropped=" << p->dropped_frames.load(std::memory_order_relaxed)
            << "cpuFrames=" << cpuFrames.load(std::memory_order_relaxed);
    }
    if (hasPts) {
        p->last_pts_ns.store(pts, std::memory_order_release);
    }
    return GST_FLOW_OK;
}

// Resets the PTS guard on every timeline boundary GstBaseSink does not route through set_caps (it skips set_caps when
// the caps are unchanged), and releases an undelivered frame on flush.
static gboolean gst_qgc_q_video_sink_event(GstBaseSink* bsink, GstEvent* event)
{
    GstQgcQVideoSink* self = GST_QGC_Q_VIDEO_SINK(bsink);
    switch (GST_EVENT_TYPE(event)) {
        case GST_EVENT_FLUSH_START:
            drop_pending_frame(self);
            break;
        case GST_EVENT_FLUSH_STOP:
        case GST_EVENT_SEGMENT:
        case GST_EVENT_STREAM_START:
            priv_of(self)->last_pts_ns.store(-1, std::memory_order_release);
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
    p->resolved_path_cache = HwResolvedPathCache{};
    p->demotion_recorded = false;
    p->last_pts_ns.store(-1, std::memory_order_release);
    p->consecutive_map_failures.store(0, std::memory_order_relaxed);
    drop_pending_frame(self);

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
                            "Buffers rejected by show_frame (inactive sink, missing QVideoSink, "
                            "PTS regression, or map failure). Detailed map failures are tracked separately "
                            "in GstHwPathTelemetry.",
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
                           "path was tried for but the CPU delivered) and stream-demotions (caps epochs that fell back "
                           "from a GPU path).",
                           GST_TYPE_STRUCTURE, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(element_class, "QGC QVideoSink", "Sink/Video",
                                          "Pushes decoded GstVideoFrames into a Qt QVideoSink",
                                          "QGroundControl <https://qgroundcontrol.com/>");
    gst_element_class_add_static_pad_template(element_class, &sink_template);

    basesink_class->set_caps = gst_qgc_q_video_sink_set_caps;
    basesink_class->event = gst_qgc_q_video_sink_event;
    basesink_class->start = gst_qgc_q_video_sink_start;
    basesink_class->stop = gst_qgc_q_video_sink_stop;
    basesink_class->propose_allocation = gst_qgc_q_video_sink_propose_allocation;
    basesink_class->query = gst_qgc_q_video_sink_query;
    videosink_class->show_frame = gst_qgc_q_video_sink_show_frame;
}

namespace GstQgc {

CapsInfo parseCapsInfo(GstMessage* message)
{
    const GstStructure* const structure = gst_message_get_structure(message);
    gint width = 0;
    gint height = 0;
    (void) gst_structure_get_int(structure, "width", &width);
    (void) gst_structure_get_int(structure, "height", &height);
    return {.format = QString::fromUtf8(gst_structure_get_string(structure, "format")), .size = QSize(width, height)};
}

const char* sinkFramesField(HwVideoBufferPath path) noexcept
{
    static const std::array<QByteArray, GstHw::kPathCount> s_fields = [] {
        std::array<QByteArray, GstHw::kPathCount> fields;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            fields[i] = QByteArray("frames-") + QByteArray(GstHw::toString(HwVideoBufferPath(i))).toLower();
        }
        return fields;
    }();
    const auto index = static_cast<std::size_t>(path);
    return (index < s_fields.size()) ? s_fields[index].constData() : nullptr;
}

GStreamer::GstStructurePtr frameStats(GstElement* sinkOrBin)
{
    GStreamer::GstElementPtr inner;
    if (sinkOrBin && GST_IS_QGC_VIDEO_SINK_BIN(sinkOrBin)) {
        inner = GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkOrBin)));
        sinkOrBin = inner.get();
    }
    if (!sinkOrBin || !GST_IS_QGC_Q_VIDEO_SINK(sinkOrBin)) {
        return {};
    }
    GstStructure* stats = nullptr;
    g_object_get(sinkOrBin, "frame-stats", &stats, nullptr);
    return GStreamer::adoptStructure(stats);
}

PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since) noexcept
{
    PathStats out;
    if (!stats) {
        return out;
    }
    const auto countOf = [stats, since](const char* field) -> quint64 {
        guint64 now = 0;
        guint64 before = 0;
        (void) gst_structure_get_uint64(stats, field, &now);
        if (since) {
            (void) gst_structure_get_uint64(since, field, &before);
        }
        return (now > before) ? (now - before) : 0;
    };
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        if (const quint64 frames = countOf(sinkFramesField(path)); frames > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(GstHw::toString(path))).arg(frames);
            if (path != HwVideoBufferPath::None) {
                out.totalDelivered += frames;
            }
        }
    }
    for (const char* const field : {kGpuFallbacksField, kStreamDemotionsField}) {
        if (const quint64 count = countOf(field); count > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(field)).arg(count);
        }
    }
    return out;
}

}  // namespace GstQgc
