#include "gstqgcvideosinkbin.h"
#include "gstqgcelements.h"

#include <gst/app/gstappsink.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video-info.h>
#if GST_CHECK_VERSION(1, 24, 0)
#  include <gst/video/video-info-dma.h>
#endif

#include <string>

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#  include "../HwBuffers/GstGlContextBridge.h"
#endif

#define GST_CAT_DEFAULT gst_qgc_debug

#define DEFAULT_ENABLE_LAST_SAMPLE FALSE
#define DEFAULT_SYNC FALSE
#define DEFAULT_MAX_LATENESS G_GINT64_CONSTANT(-1)

#define PROP_ENABLE_LAST_SAMPLE_NAME    "enable-last-sample"
#define PROP_LAST_SAMPLE_NAME           "last-sample"
#define PROP_SYNC_NAME                  "sync"
#define PROP_MAX_LATENESS_NAME          "max-lateness"

enum
{
    PROP_0,
    PROP_ENABLE_LAST_SAMPLE,
    PROP_LAST_SAMPLE,
    PROP_SYNC,
    PROP_MAX_LATENESS,
    PROP_GPU_ZEROCOPY,
    PROP_CONVERSION_ELEMENT,
    PROP_DISABLE_PAR,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE(
    "sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#define gst_qgc_video_sink_bin_parent_class parent_class
G_DEFINE_TYPE(GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST_TYPE_BIN);

GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(qgcvideosinkbin,"qgcvideosinkbin",
                                      GST_RANK_NONE,
                                      GST_TYPE_QGC_VIDEO_SINK_BIN,
                                      qgc_element_init(plugin));

static void gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_constructed(GObject *object);
static void gst_qgc_video_sink_bin_dispose(GObject *object);
static GstStateChangeReturn gst_qgc_video_sink_bin_change_state(GstElement *element, GstStateChange transition);

static void
gst_qgc_video_sink_bin_class_init(GstQgcVideoSinkBinClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    object_class->set_property = gst_qgc_video_sink_bin_set_property;
    object_class->get_property = gst_qgc_video_sink_bin_get_property;
    object_class->constructed = gst_qgc_video_sink_bin_constructed;
    object_class->dispose = gst_qgc_video_sink_bin_dispose;
    element_class->change_state = gst_qgc_video_sink_bin_change_state;

    properties[PROP_ENABLE_LAST_SAMPLE] = g_param_spec_boolean(
        "enable-last-sample", "Enable last sample",
        "Retain the most recent buffer for UI snapshotting",
        DEFAULT_ENABLE_LAST_SAMPLE,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_LAST_SAMPLE] = g_param_spec_boxed(
        "last-sample", "Last sample",
        "Last preroll/played sample held by the sink",
        GST_TYPE_SAMPLE,
        (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_SYNC] = g_param_spec_boolean(
        "sync", "Sync",
        "Synchronise frame presentation to the pipeline clock",
        DEFAULT_SYNC,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_MAX_LATENESS] = g_param_spec_int64(
        "max-lateness", "Max lateness",
        "Maximum number of nanoseconds a buffer can be late before it is dropped (-1 unlimited)",
        -1, G_MAXINT64,
        DEFAULT_MAX_LATENESS,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_GPU_ZEROCOPY] = g_param_spec_boolean(
        "gpu-zerocopy",
        "GPU zero-copy",
        "Enable DMABuf zero-copy path (Linux only). Construct-only.",
        FALSE,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_CONVERSION_ELEMENT] = g_param_spec_string(
        "conversion-element",
        "Conversion element factory",
        "Factory name to use for the CPU branch's color conversion. Empty/NULL = auto-probe.",
        NULL,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_DISABLE_PAR] = g_param_spec_boolean(
        "disable-par",
        "Disable PAR=1/1 capsfilter",
        "Skip the pixel-aspect-ratio=1/1 capsfilter on the CPU branch (workaround for "
        "v4l2 drivers without VIDIOC_CROPCAP). Construct-only.",
        FALSE,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(object_class, PROP_LAST, properties);

    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &sink_factory);

    gst_element_class_set_static_metadata(element_class,
        "QGC Video Sink Bin", "Sink/Video/Bin",
        "Appsink-based video sink for QGroundControl (frames pushed to a QVideoSink)",
        "QGroundControl team"
    );
}

// VAAPI/H.264 ref-frame queue typically 4–8; min=2 forced fallback allocations.
constexpr guint kProposedMinBuffers = 4;

static GstPadProbeReturn
gst_qgc_handle_allocation_query(GstQuery *query)
{
    GstCaps *caps = nullptr;
    gboolean need_pool = FALSE;
    gst_query_parse_allocation(query, &caps, &need_pool);
    if (!caps) {
        return GST_PAD_PROBE_OK;
    }

    GstVideoInfo vinfo;
    // DMA_DRM caps need the dma_drm parser; plain gst_video_info_from_caps fails and the
    // min-buffer hint silently disappears, leaving va on its copy-threshold pool.
#if GST_CHECK_VERSION(1, 24, 0)
    if (gst_video_is_dma_drm_caps(caps)) {
        GstVideoInfoDmaDrm drmInfo;
        gst_video_info_dma_drm_init(&drmInfo);
        if (!gst_video_info_dma_drm_from_caps(&drmInfo, caps)
            || !gst_video_info_dma_drm_to_video_info(&drmInfo, &vinfo)) {
            return GST_PAD_PROBE_OK;
        }
    } else
#endif
    if (!gst_video_info_from_caps(&vinfo, caps)) {
        return GST_PAD_PROBE_OK;
    }
    const gsize size = GST_VIDEO_INFO_SIZE(&vinfo);

    GstCapsFeatures *features = gst_caps_get_features(caps, 0);
    const bool is_system_memory = !features
        || gst_caps_features_is_any(features)
        || gst_caps_features_is_equal(features, GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY);

    if (!is_system_memory) {
        // Advertise min-buffer hint for HW memory (DMABuf/GL/D3D/NVMM/AHB) so upstream v4l2/VA does not enable copy threshold.
        gst_query_add_allocation_pool(query, NULL, size, kProposedMinBuffers, 0);
        gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
        return GST_PAD_PROBE_OK;
    }

    if (!need_pool) {
        return GST_PAD_PROBE_OK;
    }

    GstBufferPool *pool = gst_buffer_pool_new();
    if (!pool) {
        return GST_PAD_PROBE_OK;
    }
    GstStructure *config = gst_buffer_pool_get_config(pool);
    gst_buffer_pool_config_set_params(config, caps, size, kProposedMinBuffers, 0);
    gst_buffer_pool_config_add_option(config, GST_BUFFER_POOL_OPTION_VIDEO_META);
    if (gst_buffer_pool_set_config(pool, config)) {
        gst_query_add_allocation_pool(query, pool, size, kProposedMinBuffers, 0);
        gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    }
    gst_object_unref(pool);
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
gst_qgc_appsink_query_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    (void)pad;
    (void)user_data;
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (!query) {
        return GST_PAD_PROBE_OK;
    }

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_ALLOCATION:
        return gst_qgc_handle_allocation_query(query);
    case GST_QUERY_CONTEXT:
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        // Synchronous answer for gst.gl.GLDisplay/app_context — bus NEED_CONTEXT fallback races state changes and can isolate glupload from Qt's RHI context.
        if (GstGlContextBridge::answerContextQuery(query)) {
            return GST_PAD_PROBE_HANDLED;
        }
#endif
        return GST_PAD_PROBE_OK;
    default:
        return GST_PAD_PROBE_OK;
    }
}

static gboolean
gst_qgc_video_sink_bin_ghost_pad(GstQgcVideoSinkBin *self, GstElement *inner)
{
    GstPad *sinkpad = gst_element_get_static_pad(inner, "sink");
    if (!sinkpad) {
        GST_ERROR_OBJECT(self, "gst_element_get_static_pad('sink') failed");
        return FALSE;
    }

    GstPad *ghostpad = gst_ghost_pad_new("sink", sinkpad);
    gst_object_unref(sinkpad);
    if (!ghostpad) {
        GST_ERROR_OBJECT(self, "gst_ghost_pad_new('sink') failed");
        return FALSE;
    }

    if (!gst_element_add_pad(GST_ELEMENT(self), ghostpad)) {
        GST_ERROR_OBJECT(self, "gst_element_add_pad() failed");
        gst_object_unref(ghostpad);
        return FALSE;
    }

    return TRUE;
}

static void
gst_qgc_video_sink_bin_init(GstQgcVideoSinkBin *self)
{
    self->videoconvert = NULL;
    self->glupload = NULL;
    self->appsink = NULL;
    self->par_capsfilter = NULL;
    self->gpu_zerocopy = FALSE;
    self->conversion_element = NULL;
    self->disable_par = FALSE;
}

// Probe: caller override → SoC-native (imxvideoconvert_g2d/nvvidconv) → videoconvert.
static GstElement *
gst_qgc_video_sink_bin_make_conversion_element(GstQgcVideoSinkBin *self)
{
    if (self->conversion_element && self->conversion_element[0] != '\0') {
        GstElement *e = gst_element_factory_make(self->conversion_element, NULL);
        if (e) {
            GST_INFO_OBJECT(self, "Using conversion-element override '%s'", self->conversion_element);
            return e;
        }
        GST_WARNING_OBJECT(self,
            "conversion-element='%s' factory missing — falling through to defaults",
            self->conversion_element);
    }
    static const char *kSoCFactories[] = { "imxvideoconvert_g2d", "nvvidconv", NULL };
    for (int i = 0; kSoCFactories[i] != NULL; ++i) {
        if (GstElement *e = gst_element_factory_make(kSoCFactories[i], NULL)) {
            GST_INFO_OBJECT(self, "Using SoC conversion element '%s'", kSoCFactories[i]);
            return e;
        }
    }
    return gst_element_factory_make("videoconvert", NULL);
}

static void
gst_qgc_video_sink_bin_setup(GstQgcVideoSinkBin *self)
{
    self->appsink = gst_element_factory_make("appsink", "qgcappsink");
    if (!self->appsink) {
        GST_ERROR_OBJECT(self, "Failed to create appsink element");
        return;
    }

    // Attach probe before the gpu/cpu branch so both paths share the same downstream query handler.
    if (GstPad *appsinkPad = gst_element_get_static_pad(self->appsink, "sink")) {
        // probe_id == 0 means add failed (e.g. pad in dispose) — without this probe, GL-context queries fall back to the slower bus NEED_CONTEXT path that races with state changes.
        const gulong probe_id = gst_pad_add_probe(appsinkPad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
                                                  gst_qgc_appsink_query_probe, NULL, NULL);
        if (probe_id == 0) {
            GST_WARNING_OBJECT(self, "gst_pad_add_probe(QUERY_DOWNSTREAM) returned 0 — appsink query interception disabled");
        }
        gst_object_unref(appsinkPad);
    }

    if (self->gpu_zerocopy) {
        // List only features the build can consume zero-copy; stale features waste a caps-intersection pass on every link.
        // Y444 omitted: Qt 6.10 has no Format_YUV444* and toQtPixelFormat returns Invalid
        // → onNewSample errors out. Re-add when Qt grows the enum.
        static constexpr const char kFormats[] =
            "{ NV12, NV21, I420, YV12, Y42B, P010_10LE, AYUV, YUY2, UYVY, "
            "GRAY8, GRAY16_LE, BGRA, RGBA }";
        std::string capsStr;
#if defined(QGC_GST_BIN_USE_GLUPLOAD)
        // Linux desktop: va decoder produces DMA_DRM DMABuf which appsink can't consume directly; routing through glupload imports DMABuf into GL textures (still zero-copy via EGLImage) and feeds GLMemory to the appsink, which the adapter unwraps via GstGlVideoBuffer.
        capsStr = "video/x-raw(memory:GLMemory), format=";
        capsStr += kFormats;
#else
#  if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        // Legacy memory:DMABuf,format=NV12 only — covers v4l2h264dec / Mali / V3D LINEAR
        // DMABuf paths. DMA_DRM caps deliberately omitted: gst-va on Intel iHD negotiates
        // tiled+CCS layouts that crash both GPU and CPU paths. The system catch-all below
        // routes va to GstVaMemory whose map() detiles via libva.
        capsStr += "video/x-raw(memory:DMABuf), format=";
        capsStr += kFormats;
        capsStr += "; ";
#  endif
#  if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) && !defined(QGC_GST_BIN_USE_DMABUF)
        // No glupload in USE_DMABUF bin — offering GLMemory lets upstream try (and fail) it.
        capsStr += "video/x-raw(memory:GLMemory), format=";
        capsStr += kFormats;
        capsStr += "; ";
#  endif
#  if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        capsStr += "video/x-raw(memory:D3D11Memory), format=";
        capsStr += kFormats;
        capsStr += "; ";
#  endif
#  if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        capsStr += "video/x-raw(memory:D3D12Memory), format=";
        capsStr += kFormats;
        capsStr += "; ";
#  endif
#  if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        capsStr += "video/x-raw(memory:AHardwareBuffer), format=";
        capsStr += kFormats;
        capsStr += "; ";
#  endif
        // System catch-all required: dropping it returns GST_PAD_LINK_NOFORMAT (-4) when
        // upstream offers only system caps, and the receiver tears down.
        capsStr += "video/x-raw, format=";
        capsStr += kFormats;
#endif
        GstCaps *caps = gst_caps_from_string(capsStr.c_str());
        if (!caps) {
            GST_ERROR_OBJECT(self, "gst_caps_from_string() returned NULL for GPU caps");
            gst_clear_object(&self->appsink);
            return;
        }
        // emit-signals=FALSE: GstAppSinkAdapter installs callbacks via gst_app_sink_set_callbacks() — flipping this to TRUE silently breaks frame delivery (samples queue with no consumer).
        g_object_set(self->appsink,
                     "caps", caps,
                     "emit-signals", FALSE,
                     "max-buffers", 1,
                     "drop", TRUE,
                     "sync", FALSE,
                     "wait-on-eos", FALSE,
                     NULL);
        gst_caps_unref(caps);

#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
        gst_app_sink_set_max_time(GST_APP_SINK(self->appsink), 33 * GST_MSECOND);
#endif

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
        self->glupload = gst_element_factory_make("glupload", NULL);
        if (!self->glupload) {
            GST_ERROR_OBJECT(self, "Failed to create glupload element");
            gst_clear_object(&self->appsink);
            return;
        }
        gst_bin_add_many(GST_BIN(self), self->glupload, self->appsink, NULL);
        if (!gst_element_link(self->glupload, self->appsink)
            || !gst_qgc_video_sink_bin_ghost_pad(self, self->glupload)) {
            GST_ERROR_OBJECT(self, "Failed to link/ghost glupload→appsink GPU path");
            gst_bin_remove(GST_BIN(self), self->glupload);
            gst_bin_remove(GST_BIN(self), self->appsink);
            self->glupload = NULL;
            self->appsink = NULL;
            return;
        }
        GST_INFO_OBJECT(self, "Using glupload→appsink GPU path (DMABuf→GL EGLImage import)");
#else
        gst_bin_add(GST_BIN(self), self->appsink);
        if (!gst_qgc_video_sink_bin_ghost_pad(self, self->appsink)) {
            GST_ERROR_OBJECT(self, "Failed to ghost-pad appsink (GPU path)");
            gst_bin_remove(GST_BIN(self), self->appsink);
            self->appsink = NULL;
            return;
        }
#  if defined(QGC_GST_BIN_USE_DMABUF)
        GST_INFO_OBJECT(self, "Using appsink GPU path (direct DMABuf import, no glupload)");
#  else
        GST_INFO_OBJECT(self, "Using appsink GPU path (native memory passthrough)");
#  endif
#endif
    } else {
        self->videoconvert = gst_qgc_video_sink_bin_make_conversion_element(self);
        if (!self->videoconvert) {
            GST_ERROR_OBJECT(self, "Failed to create video conversion element");
            gst_clear_object(&self->appsink);
            return;
        }

        // QVideoSink renders these natively; listing them avoids forcing videoconvert to BGRA.
        GstCaps *caps = gst_caps_from_string(
            "video/x-raw,format={ NV12, NV21, I420, YV12, Y42B, P010_10LE, "
            "AYUV, YUY2, UYVY, GRAY8, GRAY16_LE, BGRA, RGBA }");
        if (!caps) {
            GST_ERROR_OBJECT(self, "gst_caps_from_string() returned NULL for CPU caps");
            gst_clear_object(&self->videoconvert);
            gst_clear_object(&self->appsink);
            return;
        }
        // emit-signals=FALSE: GstAppSinkAdapter installs callbacks via gst_app_sink_set_callbacks() — flipping this to TRUE silently breaks frame delivery (samples queue with no consumer).
        g_object_set(self->appsink,
                     "caps", caps,
                     "emit-signals", FALSE,
                     "max-buffers", 1,
                     "drop", TRUE,
                     "sync", FALSE,
                     "wait-on-eos", FALSE,
                     NULL);
        gst_caps_unref(caps);

#if defined(QGC_GST_BUILD_VERSION_MAJOR) && \
    (QGC_GST_BUILD_VERSION_MAJOR > 1 || \
     (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR >= 24))
        gst_app_sink_set_max_time(GST_APP_SINK(self->appsink), 33 * GST_MSECOND);
#endif

        // PAR=1/1 capsfilter normalizes non-square-pixel sources. disable-par for v4l2
        // drivers without VIDIOC_CROPCAP that deadlock negotiation (GStreamer MR #6242).
        if (!self->disable_par) {
            self->par_capsfilter = gst_element_factory_make("capsfilter", NULL);
            if (!self->par_capsfilter) {
                GST_WARNING_OBJECT(self, "capsfilter factory missing — PAR normalization disabled");
            } else {
                GstCaps *par_caps = gst_caps_from_string(
                    "video/x-raw, pixel-aspect-ratio=(fraction)1/1");
                g_object_set(self->par_capsfilter, "caps", par_caps, NULL);
                gst_caps_unref(par_caps);
            }
        }

        if (self->par_capsfilter) {
            gst_bin_add_many(GST_BIN(self), self->videoconvert, self->par_capsfilter,
                             self->appsink, NULL);
            if (!gst_element_link_many(self->videoconvert, self->par_capsfilter, self->appsink, NULL)
                || !gst_qgc_video_sink_bin_ghost_pad(self, self->videoconvert)) {
                GST_ERROR_OBJECT(self, "Failed to link/ghost appsink path (with PAR filter)");
                gst_bin_remove(GST_BIN(self), self->videoconvert);
                gst_bin_remove(GST_BIN(self), self->par_capsfilter);
                gst_bin_remove(GST_BIN(self), self->appsink);
                self->videoconvert = NULL;
                self->par_capsfilter = NULL;
                self->appsink = NULL;
                return;
            }
        } else {
            gst_bin_add_many(GST_BIN(self), self->videoconvert, self->appsink, NULL);
            if (!gst_element_link(self->videoconvert, self->appsink)
                || !gst_qgc_video_sink_bin_ghost_pad(self, self->videoconvert)) {
                GST_ERROR_OBJECT(self, "Failed to link/ghost appsink path");
                gst_bin_remove(GST_BIN(self), self->videoconvert);
                gst_bin_remove(GST_BIN(self), self->appsink);
                self->videoconvert = NULL;
                self->appsink = NULL;
                return;
            }
        }

        GST_INFO_OBJECT(self, "Using appsink (videoconvert%s → appsink → QVideoSink)",
                        self->par_capsfilter ? " → PAR=1/1" : "");
    }
}

static void
gst_qgc_video_sink_bin_constructed(GObject *object)
{
    G_OBJECT_CLASS(gst_qgc_video_sink_bin_parent_class)->constructed(object);
    gst_qgc_video_sink_bin_setup(GST_QGC_VIDEO_SINK_BIN(object));
}

// GstBin's dispose unrefs all child elements; our cached self->appsink/videoconvert/glupload pointers
// would then dangle. NULL them BEFORE chaining so any concurrent property accessor (which checks
// G_LIKELY(self->appsink)) sees NULL instead of touching freed memory.
static void
gst_qgc_video_sink_bin_dispose(GObject *object)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);
    self->appsink = NULL;
    self->videoconvert = NULL;
    self->par_capsfilter = NULL;
    self->glupload = NULL;
    g_clear_pointer(&self->conversion_element, g_free);
    G_OBJECT_CLASS(gst_qgc_video_sink_bin_parent_class)->dispose(object);
}

// Surfaces _setup() failures to the parent pipeline's bus on NULL→READY; without this the bin sits half-constructed (no ghost pad) and the parent reports a generic "no compatible pad" instead of the real cause logged at construction time.
static GstStateChangeReturn
gst_qgc_video_sink_bin_change_state(GstElement *element, GstStateChange transition)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(element);
    if (transition == GST_STATE_CHANGE_NULL_TO_READY && !self->appsink) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
            ("qgcvideosinkbin construction failed; cannot transition to READY"),
            ("see prior GST_ERROR messages from this element for the underlying cause"));
        return GST_STATE_CHANGE_FAILURE;
    }
    return GST_ELEMENT_CLASS(gst_qgc_video_sink_bin_parent_class)->change_state(element, transition);
}

static void
gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        if (G_LIKELY(self->appsink))
            g_object_set(self->appsink,
                         PROP_ENABLE_LAST_SAMPLE_NAME,
                         g_value_get_boolean(value),
                         NULL);
        break;
    case PROP_SYNC:
        if (G_LIKELY(self->appsink))
            g_object_set(self->appsink,
                         PROP_SYNC_NAME,
                         g_value_get_boolean(value),
                         NULL);
        break;
    case PROP_MAX_LATENESS:
        if (G_LIKELY(self->appsink))
            g_object_set(self->appsink,
                         PROP_MAX_LATENESS_NAME,
                         g_value_get_int64(value),
                         NULL);
        break;
    case PROP_GPU_ZEROCOPY:
        self->gpu_zerocopy = g_value_get_boolean(value);
        break;
    case PROP_CONVERSION_ELEMENT:
        g_free(self->conversion_element);
        self->conversion_element = g_value_dup_string(value);
        break;
    case PROP_DISABLE_PAR:
        self->disable_par = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE: {
        gboolean enable = FALSE;
        if (G_LIKELY(self->appsink))
            g_object_get(self->appsink,
                         PROP_ENABLE_LAST_SAMPLE_NAME,
                         &enable,
                         NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_LAST_SAMPLE: {
        GstSample *sample = NULL;
        if (G_LIKELY(self->appsink))
            g_object_get(self->appsink,
                         PROP_LAST_SAMPLE_NAME,
                         &sample,
                         NULL);
        if (sample) {
            gst_value_set_sample(value, sample);
            gst_sample_unref(sample);
        }
        break;
    }
    case PROP_SYNC: {
        gboolean enable = FALSE;
        if (G_LIKELY(self->appsink))
            g_object_get(self->appsink,
                         PROP_SYNC_NAME,
                         &enable,
                         NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_MAX_LATENESS: {
        gint64 lateness = DEFAULT_MAX_LATENESS;
        if (G_LIKELY(self->appsink))
            g_object_get(self->appsink,
                         PROP_MAX_LATENESS_NAME,
                         &lateness,
                         NULL);
        g_value_set_int64(value, lateness);
        break;
    }
    case PROP_GPU_ZEROCOPY:
        g_value_set_boolean(value, self->gpu_zerocopy);
        break;
    case PROP_CONVERSION_ELEMENT:
        g_value_set_string(value, self->conversion_element);
        break;
    case PROP_DISABLE_PAR:
        g_value_set_boolean(value, self->disable_par);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

GstElement *
gst_qgc_video_sink_bin_get_appsink(GstQgcVideoSinkBin *self)
{
    g_return_val_if_fail(GST_IS_QGC_VIDEO_SINK_BIN(self), NULL);
    return self->appsink ? GST_ELEMENT(gst_object_ref(self->appsink)) : NULL;
}
