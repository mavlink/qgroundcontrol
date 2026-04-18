#include "GstAppsinkBridge.h"

#include <QtMultimedia/QVideoFrame>
#include <optional>
#include <gst/gst.h>
#include <gst/video/gstvideometa.h>
#include <utility>

#include "GstVideoBufferFactory.h"
#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(GstAppsinkBridgeLog, "Video.GstAppsinkBridge")

GstAppsinkBridge::GstAppsinkBridge(VideoFrameDelivery* delivery, QObject* parent)
    : QObject(parent), _delivery(delivery)
{
    _sink = GST_APP_SINK(gst_element_factory_make("appsink", "qgc_video_sink"));
    if (!_sink) {
        qCCritical(GstAppsinkBridgeLog) << "Failed to create appsink element";
        return;
    }
    // Sink the floating ref - we own this element until destruction.
    // GstVideoReceiver::_videoSink (GstObjectPtr) will add its own ref.
    gst_object_ref_sink(_sink);

    // Configure appsink for low-latency live streaming:
    // - emit-signals=false: use callbacks instead (lower overhead, no GObject signal dispatch)
    // - drop=true + max-buffers=1: backpressure - drop stale frames rather than queueing
    // - leaky-type=downstream (GStreamer 1.20+): explicit drop policy, supersedes the
    //   combination above and documents intent; keep drop=true for older builds.
    // - sync=false: don't block on clock (live stream, not file playback)
    // - async=false: skip basesink's async preroll round-trip. Appsink already
    //   consumes via pull - preroll adds a state-change hop per pipeline start
    //   that is pure latency for live streams.
    // - wait-on-eos=false: don't block pipeline shutdown waiting for EOS
    gst_app_sink_set_emit_signals(_sink, FALSE);
    gst_app_sink_set_drop(_sink, TRUE);
    gst_app_sink_set_max_buffers(_sink, 1);
    // `leaky-type` lives in gst-plugins-base (GstAppSink); the GST_CHECK_VERSION
    // macro only reflects core lib version, so guard by runtime property probe -
    // otherwise older plugin builds paired with ≥1.20 core spam a GObject warning.
    // 2 == GST_APP_LEAKY_TYPE_DOWNSTREAM; avoids including the versioned enum header.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(_sink), "leaky-type") != nullptr) {
        g_object_set(G_OBJECT(_sink), "leaky-type", 2, nullptr);
    }
    gst_base_sink_set_sync(GST_BASE_SINK(_sink), FALSE);
    gst_base_sink_set_qos_enabled(GST_BASE_SINK(_sink), FALSE);
    gst_base_sink_set_max_lateness(GST_BASE_SINK(_sink), -1);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(_sink), FALSE);
    g_object_set(G_OBJECT(_sink), "wait-on-eos", FALSE, nullptr);

    // Accept DMA-BUF memory in addition to CPU formats so HW decoders that
    // output DMA-BUF (vah264dec, v4l2h264dec, ...) can hand buffers over
    // without decodebin inserting a software videoconvert / CPU download.
    // Order matters in caps: DMA-BUF variants first get preferred during
    // pad negotiation when both paths are available.
    const QByteArray capsStr = GstVideoBufferFactory::appsinkCaps();
    GstCaps* caps = gst_caps_from_string(capsStr.constData());
    gst_app_sink_set_caps(_sink, caps);
    gst_caps_unref(caps);

    // Callbacks fire on the GStreamer streaming thread — not the Qt main thread.
    GstAppSinkCallbacks callbacks{};
    callbacks.new_sample = &GstAppsinkBridge::onNewSample;
    callbacks.new_event = &GstAppsinkBridge::onNewEvent;
    callbacks.propose_allocation = &GstAppsinkBridge::onProposeAllocation;
    gst_app_sink_set_callbacks(_sink, &callbacks, this, nullptr);

    qCDebug(GstAppsinkBridgeLog) << "Created appsink bridge";
}

GstAppsinkBridge::~GstAppsinkBridge()
{
    // Signal the sentinel so any in-flight streaming-thread callback that checks
    // _destroyed will bail out before dereferencing this object.
    _destroyed.store(true, std::memory_order_release);

    // Clear callbacks IMMEDIATELY after setting _destroyed, and BEFORE
    // gst_element_set_state(NULL). This closes the race where a callback
    // fires (seeing _destroyed == false from a stale CPU register) after the
    // state drain times out. gst_app_sink_set_callbacks is thread-safe (holds
    // the sink's internal lock). After this point no new callbacks can be queued.
    if (_sink) {
        GstAppSinkCallbacks emptyCallbacks{};
        gst_app_sink_set_callbacks(_sink, &emptyCallbacks, nullptr, nullptr);
    }

    // Force the appsink to GST_STATE_NULL with a bounded 2 s wait.
    // Ensures any callback already executing (past the _destroyed check) has
    // returned before we call detach() / unref.
    if (_sink) {
        GstState state = GST_STATE_NULL;
        (void)gst_element_get_state(GST_ELEMENT(_sink), &state, nullptr, 0);
        if (state != GST_STATE_NULL) {
            qCWarning(GstAppsinkBridgeLog)
                << "GstAppsinkBridge: appsink not in NULL state (state =" << gst_element_state_get_name(state)
                << ") - forcing NULL with 2 s timeout";
            gst_element_set_state(GST_ELEMENT(_sink), GST_STATE_NULL);
            constexpr GstClockTime kTimeoutNs = 2 * GST_SECOND;
            const GstStateChangeReturn ret = gst_element_get_state(GST_ELEMENT(_sink), nullptr, nullptr, kTimeoutNs);
            if (ret != GST_STATE_CHANGE_SUCCESS) {
                qCCritical(GstAppsinkBridgeLog)
                    << "GstAppsinkBridge: forced NULL transition timed out - proceeding with detach()";
            }
        }
    }

    detach();
    if (_sink) {
        gst_object_unref(GST_OBJECT(_sink));
        _sink = nullptr;
    }
}

GstElement* GstAppsinkBridge::element() const
{
    return _sink ? GST_ELEMENT(_sink) : nullptr;
}

void GstAppsinkBridge::detach()
{
    QMutexLocker lock(&_mutex);
    _delivery = nullptr;
    _converter.reset();
}

// ═════════════════════════════════════════════════════════════════════
// Appsink callbacks - called on the GStreamer streaming thread
// ═════════════════════════════════════════════════════════════════════

GstFlowReturn GstAppsinkBridge::onNewSample(GstAppSink* sink, gpointer userData)
{
    auto* self = static_cast<GstAppsinkBridge*>(userData);

    // Guard against use-after-free: destructor sets _destroyed before tearing down.
    if (self->_destroyed.load(std::memory_order_acquire))
        return GST_FLOW_OK;

    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_OK;

    self->handleSample(sample);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean GstAppsinkBridge::onNewEvent(GstAppSink* /*sink*/, gpointer userData)
{
    auto* self = static_cast<GstAppsinkBridge*>(userData);

    // Guard against use-after-free: destructor sets _destroyed before tearing down.
    if (self->_destroyed.load(std::memory_order_acquire))
        return FALSE;

    // Pull and process events (tags for rotation/mirror)
    GstMiniObject* obj = gst_app_sink_pull_object(GST_APP_SINK(self->_sink));
    if (!obj)
        return FALSE;

    if (GST_IS_EVENT(obj)) {
        auto* event = GST_EVENT(obj);
        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG)
            self->_converter.handleTagEvent(event);
    }
    gst_mini_object_unref(obj);
    // Return FALSE: GstAppSinkCallbacks::new_event semantics (GStreamer 1.0) —
    // FALSE means "not consumed, let normal appsink event handling continue"
    // (i.e., the event is NOT dropped). TRUE would mean "consumed / drop".
    // We want tag events to continue propagating normally downstream.
    return FALSE;
}

gboolean GstAppsinkBridge::onProposeAllocation(GstAppSink* /*sink*/, GstQuery* query, gpointer userData)
{
    auto* self = static_cast<GstAppsinkBridge*>(userData);

    // Add video meta support so upstream can use per-plane strides
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, nullptr);

    const GstSampleFrameConverter::AllocationInfo allocation = self->_converter.allocationInfo();
    if (!allocation.valid)
        return TRUE;

    // Propose a CPU buffer pool with enough buffers for the full pipeline depth:
    //   decode (1-2) + queue (1) + render (1) + Qt texture pool (2-3)
    guint minBuffers = 6;
    if (allocation.fpsD > 0 && allocation.fpsN > 0) {
        const double fps = static_cast<double>(allocation.fpsN) / allocation.fpsD;
        if (fps > 120.0)
            minBuffers = 10;
        else if (fps > 60.0)
            minBuffers = 8;
    }

    GstCaps* caps = nullptr;
    gst_query_parse_allocation(query, &caps, nullptr);

    GstBufferPool* pool = gst_video_buffer_pool_new();
    GstStructure* config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_params(config, caps, allocation.bufferSize, minBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);

    // Advertise stride alignment so HW decoders (VA-API, NVMM, V4L2_MPLANE) can
    // use their native-aligned output without decodebin inserting a software
    // `videoconvert` to re-stride. 64-byte alignment (stride_align = 63 in
    // GStreamer's mask form - meaning "pad up to the next multiple of 64")
    // covers the common cases: VA 64B, NV12 on Tegra 256B (auto-promoted by
    // the decoder), V4L2_MPLANE pages. VIDEO_ALIGNMENT requires VIDEO_META,
    // which we've already added above.
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_ALIGNMENT);
    GstVideoAlignment align;
    gst_video_alignment_reset(&align);
    for (int plane = 0; plane < GST_VIDEO_MAX_PLANES; ++plane) {
        align.stride_align[plane] = 63;
    }
    gst_buffer_pool_config_set_video_alignment(config, &align);

    gst_buffer_pool_set_config(pool, config);
    gst_query_add_allocation_pool(query, pool, allocation.bufferSize, minBuffers, 0);
    gst_object_unref(pool);

    return TRUE;
}

void GstAppsinkBridge::handleSample(GstSample* sample)
{
    VideoFrameDelivery* delivery = nullptr;
    {
        QMutexLocker lock(&_mutex);
        delivery = _delivery;
    }

    if (!delivery)
        return;

    std::optional<QVideoFrame> frame = _converter.convert(sample, _sink, delivery);
    if (frame && frame->isValid())
        delivery->deliverFrame(std::move(*frame));
}
