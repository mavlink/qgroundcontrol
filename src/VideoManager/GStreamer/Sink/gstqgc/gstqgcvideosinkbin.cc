#include "gstqgcvideosinkbin.h"

#include "GstQgcBinChain.h"
#include "GstQgcGpuBin.h"
#include "GstQgcRenderTarget.h"
#include "GstScoped.h"
#include "gstqgcelements.h"
#include "gstqgcqvideosink.h"

#define GST_CAT_DEFAULT gst_qgc_debug

struct _GstQgcVideoSinkBin
{
    GstBin parent;
    /// qgcqvideosink terminal element; caller-set "qvideosink" property targets a QVideoSink.
    GstElement* videosink;
    gboolean gpu_zerocopy;
    /// Construct-only converter factory override; empty/NULL auto-probes
    /// (imxvideoconvert_g2d/nvvidconv -> videoconvertscale).
    gchar* conversion_element;
    /// Lets the sink take non-square pixels (v4l2 drivers without VIDIOC_CROPCAP deadlock when PAR is forced).
    gboolean disable_par;
    /// Proxied to the inner basesink's "sync" so callers can configure clock sync on the bin.
    gboolean sync;
    /// Proxied to the inner basesink's "qos"; default FALSE (off) preserves the existing no-QoS behavior.
    gboolean qos;
    /// Proxied to the inner basesink's "processing-deadline" (ns); default matches the basesink default.
    guint64 processing_deadline;
};

enum
{
    PROP_0,
    PROP_GPU_ZEROCOPY,
    PROP_CONVERSION_ELEMENT,
    PROP_DISABLE_PAR,
    PROP_SYNC,
    PROP_QOS,
    PROP_PROCESSING_DEADLINE,
    PROP_LAST
};

static GParamSpec* properties[PROP_LAST];

// video/x-raw(ANY) accepts every memory feature plus system catch-all; narrows from CAPS_ANY so
// non-raw links fail at link time (not PAUSED negotiation) without dropping any zero-copy path.
static GstStaticPadTemplate sink_factory =
    GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw(ANY)"));

#define gst_qgc_video_sink_bin_parent_class parent_class
G_DEFINE_FINAL_TYPE(GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST_TYPE_BIN);

GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(qgcvideosinkbin, "qgcvideosinkbin", GST_RANK_NONE, GST_TYPE_QGC_VIDEO_SINK_BIN,
                                      qgc_element_init(plugin));

static void gst_qgc_video_sink_bin_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);
static void gst_qgc_video_sink_bin_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);
static void gst_qgc_video_sink_bin_constructed(GObject* object);
static void gst_qgc_video_sink_bin_dispose(GObject* object);
static GstStateChangeReturn gst_qgc_video_sink_bin_change_state(GstElement* element, GstStateChange transition);

static void gst_qgc_video_sink_bin_class_init(GstQgcVideoSinkBinClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GstElementClass* element_class = GST_ELEMENT_CLASS(klass);

    object_class->set_property = gst_qgc_video_sink_bin_set_property;
    object_class->get_property = gst_qgc_video_sink_bin_get_property;
    object_class->constructed = gst_qgc_video_sink_bin_constructed;
    object_class->dispose = gst_qgc_video_sink_bin_dispose;
    element_class->change_state = gst_qgc_video_sink_bin_change_state;

    properties[PROP_GPU_ZEROCOPY] = g_param_spec_boolean(
        "gpu-zerocopy", "GPU zero-copy", "Enable the platform GPU zero-copy path. Construct-only.", FALSE,
        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_CONVERSION_ELEMENT] =
        g_param_spec_string("conversion-element", "Conversion element factory",
                            "Factory name of the converter at the head of the bin. Empty/NULL = auto-probe.", NULL,
                            (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_DISABLE_PAR] = g_param_spec_boolean(
        "disable-par", "Allow non-square pixels",
        "Let the sink take non-square pixels instead of having the converter scale them to pixel-aspect-ratio=1/1 "
        "(workaround for v4l2 drivers without VIDIOC_CROPCAP). Construct-only.",
        FALSE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_SYNC] =
        g_param_spec_boolean("sync", "Sync", "Proxied to the inner basesink: sync rendering to the clock.", FALSE,
                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    // Opt-in QoS: default FALSE leaves the existing no-QoS behavior unchanged (sync=FALSE means the
    // basesink generates no QoS events anyway until a caller enables it).
    properties[PROP_QOS] =
        g_param_spec_boolean("qos", "QoS", "Proxied to the inner basesink: generate QoS events upstream.", FALSE,
                             (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties[PROP_PROCESSING_DEADLINE] = g_param_spec_uint64(
        "processing-deadline", "Processing deadline",
        "Proxied to the inner basesink: maximum buffer processing time in nanoseconds.", 0, G_MAXUINT64,
        G_GUINT64_CONSTANT(20000000), (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(object_class, PROP_LAST, properties);

    gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass), &sink_factory);

    gst_element_class_set_static_metadata(element_class, "QGC Video Sink Bin", "Sink/Video/Bin",
                                          "QVideoSink-based video sink for QGroundControl", "QGroundControl team");
}

static void gst_qgc_video_sink_bin_init(GstQgcVideoSinkBin* self)
{
    self->videosink = NULL;
    self->gpu_zerocopy = FALSE;
    self->conversion_element = NULL;
    self->disable_par = FALSE;
    self->sync = FALSE;
    self->qos = FALSE;
    self->processing_deadline = G_GUINT64_CONSTANT(20000000);
}

// A single-element converter: the conversion-element override, else a SoC converter, else videoconvertscale (GStreamer
// 1.22+). Null when none is available.
static GstElement* gst_qgc_video_sink_bin_make_conversion_element(GstQgcVideoSinkBin* self)
{
    if (self->conversion_element && self->conversion_element[0] != '\0') {
        GstElement* e = gst_element_factory_make(self->conversion_element, NULL);
        if (e) {
            GST_INFO_OBJECT(self, "Using conversion-element override '%s'", self->conversion_element);
            return e;
        }
        GST_WARNING_OBJECT(self, "conversion-element='%s' factory missing — falling through to defaults",
                           self->conversion_element);
    }
    // These SoCs' decoders output proprietary layouts under plain video/x-raw formats, which only their converter
    // reads.
    static const char* kSoCFactories[] = {"imxvideoconvert_g2d", "nvvidconv", NULL};
    for (int i = 0; kSoCFactories[i] != NULL; ++i) {
        if (GstElement* e = gst_element_factory_make(kSoCFactories[i], NULL)) {
            GST_INFO_OBJECT(self, "Using SoC conversion element '%s'", kSoCFactories[i]);
            return e;
        }
    }
    return gst_element_factory_make("videoconvertscale", NULL);
}

// Adds the bin's head, which converts and scales system memory into what qgcqvideosink offers (scaling lets its
// square-pixel caps take non-square-pixel sources) and passes GPU memory through. Before GStreamer 1.22 that's
// videoconvert ! videoscale; @p head and @p tail are its ends.
static gboolean add_converter(GstQgcVideoSinkBin* self, GstQgc::BinChain& chain, GstElement*& head, GstElement*& tail)
{
    if (GstElement* const converter = gst_qgc_video_sink_bin_make_conversion_element(self)) {
        head = tail = chain.adopt(converter);
        return head != NULL;
    }
    head = chain.adopt(gst_element_factory_make("videoconvert", NULL));
    tail = chain.adopt(gst_element_factory_make("videoscale", NULL));
    return head && tail && chain.linkChain({head, tail});
}

// The GPU variant for the render target (GstQgc::selectGpuBin). Any other renderer gets the passthrough variant, whose
// sink offers only memory that renderer imports (a glupload bin under Vulkan would download every frame and upload it
// again).
static GstQgc::GpuBinMode select_gpu_bin(GstQgcVideoSinkBin* self)
{
    const GstQgc::GpuBinPreference preference = GstQgc::gpuBinPreference();
    const GstQgc::RenderTarget target = GstQgc::currentRenderTarget();
    const GstQgc::GpuBinMode mode = GstQgc::selectGpuBin(preference, target);
    if ((mode == GstQgc::GpuBinMode::D3D12Convert) &&
        (!GStreamer::hasElementFactory("d3d12upload") || !GStreamer::hasElementFactory("d3d12convert"))) {
        GST_WARNING_OBJECT(self,
                           "Qt renders with Direct3D12 but d3d12upload/d3d12convert are missing: using the "
                           "passthrough GPU bin, which takes system memory");
        return GstQgc::GpuBinMode::Passthrough;
    }
    if (mode != preference.mode) {
        GST_INFO_OBJECT(self, "Qt renders with %s (EGL %d): using the %s GPU bin", GstQgc::toString(target.backend),
                        target.egl, GstQgc::gpuBinName(mode));
    } else if (preference.explicitChoice && (GstQgc::selectGpuBin({.mode = mode}, target) != mode)) {
        GST_WARNING_OBJECT(self, "QGC_GST_GPU_BIN forces a GPU bin Qt's renderer doesn't suit (%s, EGL %d)",
                           GstQgc::toString(target.backend), target.egl);
    }
    return mode;
}

// Builds [converter ! (glupload ! glcolorconvert | d3d12upload ! d3d12convert !) qgcqvideosink]. qgcqvideosink's
// get_caps decides what negotiates:
// the formats Qt renders, the GPU memory the render target imports, square pixels unless disable-par.
static void gst_qgc_video_sink_bin_setup(GstQgcVideoSinkBin* self)
{
    // gpu-zerocopy is construct-only — pass it to construction; g_object_set after the fact is rejected.
    GstElement* videosink = gst_element_factory_make_full("qgcqvideosink", "name", "qgcqvideosink", "gpu-zerocopy",
                                                          self->gpu_zerocopy, NULL);
    if (!videosink) {
        GST_ERROR_OBJECT(self, "Failed to create qgcqvideosink element");
        return;
    }
    g_object_set(videosink, "active", (gboolean) TRUE, "sync", (gboolean) self->sync, "qos", (gboolean) self->qos,
                 "processing-deadline", self->processing_deadline, NULL);
    const GstQgc::GpuBinMode gpuBin = self->gpu_zerocopy ? select_gpu_bin(self) : GstQgc::GpuBinMode::Passthrough;
    GstQgc::configureSinkCaps(videosink, gpuBin, !self->disable_par);

    // On failure the BinChain rolls the bin back; self->videosink stays NULL so change_state surfaces the construction
    // error.
    GstQgc::BinChain chain(GST_BIN(self));
    if (!chain.adopt(videosink)) {
        GST_ERROR_OBJECT(self, "Failed to add qgcqvideosink to bin");
        return;
    }
    GstElement* head = NULL;
    GstElement* tail = NULL;
    if (!add_converter(self, chain, head, tail)) {
        GST_ERROR_OBJECT(self, "Failed to create the video conversion element");
        return;
    }
    if (gpuBin == GstQgc::GpuBinMode::GlUpload) {
        GstElement* const glupload = chain.adopt(gst_element_factory_make("glupload", NULL));
        // glcolorconvert honours the sink's texture-target=2D; a passthrough when upstream is already 2D.
        GstElement* const glcolorconvert = chain.adopt(gst_element_factory_make("glcolorconvert", NULL));
        if (!glupload || !glcolorconvert) {
            GST_ERROR_OBJECT(self, "Failed to create glupload/glcolorconvert");
            return;
        }
        tail = chain.linkChain({tail, glupload, glcolorconvert}) ? glcolorconvert : NULL;
    } else if (gpuBin == GstQgc::GpuBinMode::D3D12Convert) {
        // Decoder output passes the converter as D3D12Memory; d3d12upload uploads software-decoded frames.
        GstElement* const upload = chain.adopt(gst_element_factory_make("d3d12upload", NULL));
        GstElement* const convert = chain.adopt(gst_element_factory_make("d3d12convert", NULL));
        if (!upload || !convert) {
            GST_ERROR_OBJECT(self, "Failed to create d3d12upload/d3d12convert");
            return;
        }
        tail = chain.linkChain({tail, upload, convert}) ? convert : NULL;
    }
    if (!tail || !chain.linkChain({tail, videosink}) || !chain.ghostSink(head)) {
        GST_ERROR_OBJECT(self, "Failed to link/ghost the qgcqvideosink chain");
        return;
    }
    chain.commit();
    self->videosink = videosink;
    GST_INFO_OBJECT(self, "qgcqvideosink %s bin%s", self->gpu_zerocopy ? GstQgc::gpuBinName(gpuBin) : "CPU",
                    self->disable_par ? " (non-square pixels allowed)" : "");
}

static void gst_qgc_video_sink_bin_constructed(GObject* object)
{
    G_OBJECT_CLASS(gst_qgc_video_sink_bin_parent_class)->constructed(object);
    gst_qgc_video_sink_bin_setup(GST_QGC_VIDEO_SINK_BIN(object));
}

// GstBin dispose unrefs children; NULL our cached pointer BEFORE chaining so a concurrent
// property accessor sees NULL instead of freed memory.
static void gst_qgc_video_sink_bin_dispose(GObject* object)
{
    GstQgcVideoSinkBin* self = GST_QGC_VIDEO_SINK_BIN(object);
    self->videosink = NULL;
    g_clear_pointer(&self->conversion_element, g_free);
    G_OBJECT_CLASS(gst_qgc_video_sink_bin_parent_class)->dispose(object);
}

// Surfaces _setup() failures to the parent bus on NULL->READY; without it the bin sits without
// a ghost pad and the parent reports a generic "no compatible pad" instead of the real cause.
static GstStateChangeReturn gst_qgc_video_sink_bin_change_state(GstElement* element, GstStateChange transition)
{
    GstQgcVideoSinkBin* self = GST_QGC_VIDEO_SINK_BIN(element);
    if (transition == GST_STATE_CHANGE_NULL_TO_READY && !self->videosink) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                          ("qgcvideosinkbin construction failed; cannot transition to READY"),
                          ("see prior GST_ERROR messages from this element for the underlying cause"));
        return GST_STATE_CHANGE_FAILURE;
    }
    return GST_ELEMENT_CLASS(gst_qgc_video_sink_bin_parent_class)->change_state(element, transition);
}

static void gst_qgc_video_sink_bin_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    GstQgcVideoSinkBin* self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
        case PROP_GPU_ZEROCOPY:
            self->gpu_zerocopy = g_value_get_boolean(value);
            break;
        case PROP_CONVERSION_ELEMENT:
            GST_OBJECT_LOCK(self);
            g_free(self->conversion_element);
            self->conversion_element = g_value_dup_string(value);
            GST_OBJECT_UNLOCK(self);
            break;
        case PROP_DISABLE_PAR:
            self->disable_par = g_value_get_boolean(value);
            break;
        case PROP_SYNC:
            self->sync = g_value_get_boolean(value);
            if (self->videosink)
                g_object_set(self->videosink, "sync", (gboolean) self->sync, NULL);
            break;
        case PROP_QOS:
            self->qos = g_value_get_boolean(value);
            if (self->videosink)
                g_object_set(self->videosink, "qos", (gboolean) self->qos, NULL);
            break;
        case PROP_PROCESSING_DEADLINE:
            self->processing_deadline = g_value_get_uint64(value);
            if (self->videosink)
                g_object_set(self->videosink, "processing-deadline", self->processing_deadline, NULL);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

static void gst_qgc_video_sink_bin_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    GstQgcVideoSinkBin* self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
        case PROP_GPU_ZEROCOPY:
            g_value_set_boolean(value, self->gpu_zerocopy);
            break;
        case PROP_CONVERSION_ELEMENT:
            GST_OBJECT_LOCK(self);
            g_value_set_string(value, self->conversion_element);
            GST_OBJECT_UNLOCK(self);
            break;
        case PROP_DISABLE_PAR:
            g_value_set_boolean(value, self->disable_par);
            break;
        case PROP_SYNC:
            g_value_set_boolean(value, self->sync);
            break;
        case PROP_QOS:
            g_value_set_boolean(value, self->qos);
            break;
        case PROP_PROCESSING_DEADLINE:
            g_value_set_uint64(value, self->processing_deadline);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
            break;
    }
}

GstElement* gst_qgc_video_sink_bin_get_qvideosink(GstQgcVideoSinkBin* self)
{
    g_return_val_if_fail(GST_IS_QGC_VIDEO_SINK_BIN(self), NULL);
    return self->videosink ? GST_ELEMENT(gst_object_ref(self->videosink)) : NULL;
}
