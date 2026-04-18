#include "GstDecodingBranch.h"

#include <gst/gst.h>

#include "GStreamerHelpers.h"
#include "GstAppsinkBridge.h"
#include "QGCLoggingCategory.h"
#include "VideoFrameDelivery.h"

QGC_LOGGING_CATEGORY(GstDecodingBranchLog, "Video.GstDecodingBranch")

GstDecodingBranch::~GstDecodingBranch()
{
    teardownSink();
}

// ═══════════════════════════════════════════════════════════════════════════
// Sink setup
// ═══════════════════════════════════════════════════════════════════════════

void GstDecodingBranch::setupSink(VideoFrameDelivery* delivery, QObject* parent)
{
    teardownSink();

    _appsinkBridge = new GstAppsinkBridge(delivery, parent);
    GstElement* sinkElement = _appsinkBridge->element();

    if (!sinkElement) {
        qCCritical(GstDecodingBranchLog) << "Failed to create appsink";
        delete _appsinkBridge;
        _appsinkBridge = nullptr;
        return;
    }

    _videoSink = GstObjectPtr<GstElement>(sinkElement);
    qCDebug(GstDecodingBranchLog) << "Appsink bridge created";
}

void GstDecodingBranch::teardownSink()
{
    if (_appsinkBridge) {
        // Sink swaps can fire on the main thread while the pipeline is PLAYING.
        // Bound the NULL wait to avoid deadlocking against a streaming-thread
        // callback blocked on the appsink's internal lock.
        GstElement* el = _appsinkBridge->element();
        _appsinkBridge->detach();
        if (el) {
            GstObject* parent = gst_element_get_parent(el);
            if (parent) {
                (void)gst_bin_remove(GST_BIN(parent), el);
                gst_object_unref(parent);
            }
            (void)gst_element_set_state(el, GST_STATE_NULL);
            constexpr GstClockTime kTeardownTimeoutNs = 3 * GST_SECOND;
            const GstStateChangeReturn ret =
                gst_element_get_state(el, nullptr, nullptr, kTeardownTimeoutNs);
            if (ret != GST_STATE_CHANGE_SUCCESS) {
                qCCritical(GstDecodingBranchLog)
                    << "teardownSink: appsink NULL transition timed out after 3 s — proceeding";
            }
        }
        delete _appsinkBridge;
        _appsinkBridge = nullptr;
    }
    _videoSink = {};
}

// ═══════════════════════════════════════════════════════════════════════════
// Pipeline integration
// ═══════════════════════════════════════════════════════════════════════════

void GstDecodingBranch::ensureSinkInPipeline(GstElement* pipeline)
{
    if (!_videoSink || !pipeline)
        return;

    GstObject* parent = gst_element_get_parent(_videoSink.get());
    if (parent) {
        gst_object_unref(parent);
        return;  // already in pipeline
    }

    // Detect live vs non-live to set sync mode. The latency query can return
    // stale/wrong values at this stage (RTSP source hasn't finished SDP
    // negotiation yet), so default to LIVE unless we can positively confirm
    // a non-live pipeline. For QGC's use case every real stream is effectively
    // live; only synthetic/file-backed test pipelines are non-live, and those
    // tolerate sync=FALSE just fine.
    bool isLive = true;
    GstQueryPtr query(gst_query_new_latency());
    if (gst_element_query(pipeline, query.get())) {
        gboolean live = FALSE;
        GstClockTime minLatency = 0;
        gst_query_parse_latency(query.get(), &live, &minLatency, nullptr);
        // Only trust a non-live answer when the query actually reports a
        // latency (i.e. the source has negotiated enough to have an opinion).
        if (!live && minLatency != 0)
            isLive = false;
    }

    if (isLive) {
        g_object_set(_videoSink.get(), "sync", FALSE, "qos", FALSE, "max-lateness", G_GINT64_CONSTANT(-1), NULL);
    } else {
        g_object_set(_videoSink.get(), "sync", TRUE, "qos", TRUE, "max-lateness", static_cast<gint64>(20 * GST_MSECOND),
                     NULL);
        qCDebug(GstDecodingBranchLog) << "Non-live pipeline — sink sync enabled";
    }

    (void)gst_bin_add(GST_BIN(pipeline), _videoSink.get());
    // Sync with the pipeline's state — when the pipeline is already PLAYING
    // (RTSP startup case: pipeline goes PLAYING before the source pad arrives
    // and triggers decoding-branch activation), a newly-added appsink stuck in
    // PAUSED causes downstream chain() calls to return NOT_NEGOTIATED once the
    // decoder starts pushing. sync_state_with_parent transitions the sink to
    // PLAYING atomically, matching the rest of the pipeline.
    (void)gst_element_sync_state_with_parent(_videoSink.get());
}

bool GstDecodingBranch::addDecoder(GstElement* pipeline, GstElement* src, std::function<void(GstPad*)> onNewPad,
                                   GCallback padAddedCb, gpointer cbData)
{
    // decodebin3: requires GStreamer ≥ 1.22. Its `force-sw-decoders` property
    // also lands in 1.22; the scoped HW→SW fallback below assumes that version
    // floor. Older GStreamer would fail at gst_element_factory_make() here.
    _decoder = GstObjectPtr<GstElement>(gst_element_factory_make("decodebin3", nullptr));
    if (!_decoder) {
        qCCritical(GstDecodingBranchLog) << "gst_element_factory_make('decodebin3') failed";
        return false;
    }

    // Per-branch HW→SW fallback: if this branch has previously seen a HW
    // decoder fail, skip HW candidates for this pipeline instance only.
    // Unlike a global registry-rank mutation, this keeps sibling streams
    // on the same process free to pick HW decoders.
    if (_hwDecoderFailed) {
        g_object_set(_decoder.get(), "force-sw-decoders", TRUE, nullptr);
        qCDebug(GstDecodingBranchLog) << "decodebin3: force-sw-decoders=TRUE (scoped fallback)";
    }

    (void)gst_bin_add(GST_BIN(pipeline), _decoder.get());
    (void)gst_element_sync_state_with_parent(_decoder.get());

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-decoder");

    if (!gst_element_link(src, _decoder.get())) {
        qCCritical(GstDecodingBranchLog) << "Unable to link decoder";
        gst_element_set_state(_decoder.get(), GST_STATE_NULL);
        (void)gst_bin_remove(GST_BIN(pipeline), _decoder.get());
        _decoder = {};
        return false;
    }

    auto srcPad = GStreamer::gstFirstSrcPad(_decoder.get());
    if (srcPad) {
        onNewPad(srcPad.get());
    } else {
        (void)g_signal_connect(_decoder.get(), "pad-added", padAddedCb, cbData);
    }
    return true;
}

bool GstDecodingBranch::addVideoSink(GstElement* pipeline, GstPad* pad, GstElement* decoderValve)
{
    ensureSinkInPipeline(pipeline);

    GstNonFloatingPtr<GstPad> sinkPad(gst_element_get_static_pad(_videoSink.get(), "sink"));
    GstPadLinkReturn linkRet = sinkPad ? gst_pad_link(pad, sinkPad.get()) : GST_PAD_LINK_WRONG_HIERARCHY;
    if (linkRet != GST_PAD_LINK_OK) {
        qCCritical(GstDecodingBranchLog) << "Unable to link decoder pad to video sink, result:" << linkRet;
        GStreamer::gstRemoveFromParent(_videoSink.get());
        return false;
    }
    sinkPad.reset();

    (void)gst_element_sync_state_with_parent(_videoSink.get());

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-with-videosink");

    // Determine video size (non-fatal)
    QSize videoSize;
    if (decoderValve) {
        GstNonFloatingPtr<GstPad> valveSrcPad(gst_element_get_static_pad(decoderValve, "src"));
        if (valveSrcPad) {
            GstCapsPtr valveSrcPadCaps(gst_pad_query_caps(valveSrcPad.get(), nullptr));
            if (valveSrcPadCaps) {
                const GstStructure* structure = gst_caps_get_structure(valveSrcPadCaps.get(), 0);
                if (structure) {
                    gint width = 0, height = 0;
                    (void)gst_structure_get_int(structure, "width", &width);
                    (void)gst_structure_get_int(structure, "height", &height);
                    videoSize.setWidth(width);
                    videoSize.setHeight(height);
                }
            }
        }
    }

    // Caller is responsible for emitting videoSizeChanged(videoSize).
    _lastVideoSize = videoSize;

    return true;
}

QMediaMetaData GstDecodingBranch::logSelectedCodec()
{
    QMediaMetaData meta;
    if (!_decoder)
        return meta;

    _isHwDecoding = false;
    _activeDecoderName.clear();

    gstIteratorForEach<GstElement>(gst_bin_iterate_elements(GST_BIN(_decoder.get())), [this](GstElement* child) {
        GstElementFactory* factory = gst_element_get_factory(child);
        if (!factory || !gst_element_factory_list_is_type(factory, GST_ELEMENT_FACTORY_TYPE_DECODER))
            return;

        const gchar* decoderKlass = gst_element_factory_get_klass(factory);
        GstPluginFeature* feature = GST_PLUGIN_FEATURE(factory);
        const gchar* featureName = gst_plugin_feature_get_name(feature);
        const guint rank = gst_plugin_feature_get_rank(feature);
        const bool isHw = GStreamer::isHardwareDecoderFactory(factory);

        _activeDecoderName = QString::fromUtf8(featureName);
        _isHwDecoding = isHw;

        GstNonFloatingPtr<GstPlugin> plugin(gst_plugin_feature_get_plugin(feature));
        const QString pluginName = plugin ? gst_plugin_get_name(plugin.get()) : featureName;

        qCDebug(GstDecodingBranchLog) << "Decodebin3 selected codec:rank -" << pluginName << "/" << featureName << "-"
                                      << decoderKlass << (isHw ? "(HW)" : "(SW)") << ":" << rank;

        g_object_set(child, "qos", FALSE, nullptr);
        qCDebug(GstDecodingBranchLog) << "Disabled QoS on internal decoder" << featureName;
    });

    // Resolution + VideoCodec + VideoFrameRate are populated by the caller
    // from the pad caps via gstStructureToMediaMetaData(). The decoder name
    // remains available via activeDecoderName() for consumers that need it.
    if (_lastVideoSize.isValid())
        meta.insert(QMediaMetaData::Resolution, _lastVideoSize);
    return meta;
}

// ═══════════════════════════════════════════════════════════════════════════
// Branch teardown
// ═══════════════════════════════════════════════════════════════════════════

void GstDecodingBranch::shutdown(GstElement* pipeline)
{
    if (_decoder) {
        // Set decoder to NULL before removing so its internal threads (HW worker,
        // decodebin streaming) drain — otherwise they can crash during the subsequent
        // unref, especially on stop/start cycles.
        (void)gst_element_set_state(_decoder.get(), GST_STATE_NULL);
        constexpr GstClockTime kShutdownTimeoutNs = 3 * GST_SECOND;
        const GstStateChangeReturn ret =
            gst_element_get_state(_decoder.get(), nullptr, nullptr, kShutdownTimeoutNs);
        if (ret != GST_STATE_CHANGE_SUCCESS) {
            qCCritical(GstDecodingBranchLog)
                << "shutdown: decoder NULL transition timed out after 3 s — proceeding with removal";
        }
        GStreamer::gstRemoveFromParent(_decoder.get());
        _decoder = {};
    }

    _videoSinkProbeGuard.remove();
    _lastVideoFrameTime.store(0, std::memory_order_relaxed);

    GStreamer::gstRemoveFromParent(_videoSink.get());
    // Don't clear _videoSink — it's reusable if the branch restarts.
    // The appsink element itself survives; only its pipeline membership is removed.

    _state = BranchState::Off;

    if (pipeline)
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline-decoding-stopped");
}

// ═══════════════════════════════════════════════════════════════════════════
// HW decoder fallback
// ═══════════════════════════════════════════════════════════════════════════

bool GstDecodingBranch::isHwDecoderError(GstMessage* msg) const
{
    if (_hwDecoderFailed)
        return false;  // already tried fallback

    GstElement* src = GST_ELEMENT(GST_MESSAGE_SRC(msg));
    if (!src || !_decoder)
        return false;

    // Walk parent chain to verify src is inside our decoder bin
    GstObject* parent = gst_element_get_parent(src);
    bool isInDecoder = false;
    while (parent) {
        if (GST_ELEMENT(parent) == _decoder.get()) {
            isInDecoder = true;
            gst_object_unref(parent);
            break;
        }
        GstObject* grandparent = gst_element_get_parent(parent);
        gst_object_unref(parent);
        parent = grandparent;
    }
    if (!isInDecoder)
        return false;

    GstElementFactory* factory = gst_element_get_factory(src);
    return factory && GStreamer::isHardwareDecoderFactory(factory);
}
