#include "gstqgcvideosinkbin.h"

#include "gstqgcelements.h"

#include <array>
#include <initializer_list>
#include <string>

#include "GstQgcAllocation.h"
#include "GstQgcCaps.h"

#define GST_CAT_DEFAULT gst_qgc_debug

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

namespace {

// Linked element chain with automatic rollback: adopt() elements (bin takes the floating ref),
// linkChain() in flow order, ghostSink() the head, commit() on success; ~BinChain unwinds the rest.
class BinChain
{
public:
    explicit BinChain(GstBin* bin) : m_bin(bin) {}

    ~BinChain()
    {
        if (m_committed) {
            return;
        }
        if (m_ghost) {
            gst_element_remove_pad(GST_ELEMENT(m_bin), m_ghost);
        }
        while (m_ownedCount > 0) {
            gst_bin_remove(m_bin, m_owned[--m_ownedCount]);
        }
    }

    BinChain(const BinChain&) = delete;
    BinChain& operator=(const BinChain&) = delete;

    GstElement* adopt(GstElement* element)
    {
        if (!element) {
            return nullptr;
        }
        if (m_ownedCount == m_owned.size()) {
            gst_object_unref(element);
            return nullptr;
        }
        if (!gst_bin_add(m_bin, element)) {
            gst_object_unref(element);
            return nullptr;
        }
        m_owned[m_ownedCount++] = element;
        return element;
    }

    bool linkChain(std::initializer_list<GstElement*> chain)
    {
        GstElement* prev = nullptr;
        for (GstElement* element : chain) {
            if (!element) {
                return false;
            }
            if (prev && !gst_element_link(prev, element)) {
                return false;
            }
            prev = element;
        }
        return true;
    }

    bool ghostSink(GstElement* head)
    {
        GstPad* sinkpad = gst_element_get_static_pad(head, "sink");
        if (!sinkpad) {
            return false;
        }
        m_ghost = gst_ghost_pad_new("sink", sinkpad);
        gst_object_unref(sinkpad);
        if (!m_ghost) {
            return false;
        }
        if (!gst_element_add_pad(GST_ELEMENT(m_bin), m_ghost)) {
            gst_object_unref(m_ghost);
            m_ghost = nullptr;
            return false;
        }
        return true;
    }

    void commit() { m_committed = true; }

private:
    GstBin* m_bin;
    std::array<GstElement*, 5> m_owned{};
    std::size_t m_ownedCount = 0;
    GstPad* m_ghost = nullptr;
    bool m_committed = false;
};

}  // namespace

#ifdef QGC_GST_BUILD_TESTING
gboolean gst_qgc_video_sink_bin_rejects_failed_adopt_for_test()
{
    GstElement* target = gst_bin_new("qgc-adopt-target");
    GstElement* parent = gst_bin_new("qgc-adopt-existing-parent");
    GstElement* element = gst_element_factory_make("identity", "preparented");
    if (!target || !parent || !element) {
        gst_clear_object(&element);
        gst_clear_object(&parent);
        gst_clear_object(&target);
        return FALSE;
    }

    if (!gst_bin_add(GST_BIN(parent), element)) {
        gst_object_unref(parent);
        gst_object_unref(target);
        return FALSE;
    }

    auto* chain = new BinChain(GST_BIN(target));
    gst_object_ref(element);
    GstElement* adopted = chain->adopt(element);
    chain->commit();
    delete chain;

    GstObject* currentParent = gst_object_get_parent(GST_OBJECT(element));
    const gboolean rejected = (adopted == nullptr) && (currentParent == GST_OBJECT(parent));
    if (currentParent) {
        gst_object_unref(currentParent);
    }
    gst_object_unref(parent);
    gst_object_unref(target);
    return rejected;
}
#endif

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
                            "Factory name to use for the CPU branch's color conversion. Empty/NULL = auto-probe.", NULL,
                            (GParamFlags) (G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

    properties[PROP_DISABLE_PAR] = g_param_spec_boolean(
        "disable-par", "Disable PAR=1/1 capsfilter",
        "Skip the pixel-aspect-ratio=1/1 capsfilter on the CPU branch (workaround for "
        "v4l2 drivers without VIDIOC_CROPCAP). Construct-only.",
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
    self->videoconvert = NULL;
    self->glupload = NULL;
    self->videosink = NULL;
    self->format_capsfilter = NULL;
    self->par_capsfilter = NULL;
    self->gpu_zerocopy = FALSE;
    self->conversion_element = NULL;
    self->disable_par = FALSE;
    self->sync = FALSE;
    self->qos = FALSE;
    self->processing_deadline = G_GUINT64_CONSTANT(20000000);
}

// Probe: caller override → SoC-native (imxvideoconvert_g2d/nvvidconv) → videoconvert.
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
    static const char* kSoCFactories[] = {"imxvideoconvert_g2d", "nvvidconv", NULL};
    for (int i = 0; kSoCFactories[i] != NULL; ++i) {
        if (GstElement* e = gst_element_factory_make(kSoCFactories[i], NULL)) {
            GST_INFO_OBJECT(self, "Using SoC conversion element '%s'", kSoCFactories[i]);
            return e;
        }
    }
    return gst_element_factory_make("videoconvert", NULL);
}

// Wire format-capsfilter -> qgcqvideosink (optional glupload prefix); self pointers set only on commit.
static gboolean wireGpuPath(GstQgcVideoSinkBin* self, GstElement* videosink, GstElement* capsf)
{
    BinChain chain(GST_BIN(self));
    if (!chain.adopt(videosink)) {
        gst_object_unref(capsf);
        GST_ERROR_OBJECT(self, "Failed to add qgcqvideosink GPU sink element to bin");
        return FALSE;
    }
    if (!chain.adopt(capsf)) {
        GST_ERROR_OBJECT(self, "Failed to add qgcqvideosink GPU capsfilter element to bin");
        return FALSE;
    }

    const std::string capsStr = GstQgc::buildGpuCapsString();
    GstCaps* caps = gst_caps_from_string(capsStr.c_str());
    if (!caps) {
        GST_ERROR_OBJECT(self, "gst_caps_from_string() returned NULL for GPU caps");
        return FALSE;
    }
    // format_capsfilter restricts negotiation to Qt-renderable formats; qgcqvideosink's pad
    // template is CAPS_ANY, so without it upstream could pick a format that fails in set_caps.
    g_object_set(capsf, "caps", caps, NULL);
    gst_caps_unref(caps);

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
    GstElement* glupload = chain.adopt(gst_element_factory_make("glupload", NULL));
    if (!glupload) {
        GST_ERROR_OBJECT(self, "Failed to create glupload element");
        return FALSE;
    }
    // Converts amcvideodec's external-OES Surface textures to GL_TEXTURE_2D (else the Qt RHI 2D sink
    // samples them black); no-op passthrough when upstream is already 2D (Linux va/DMABuf).
    GstElement* glcolorconvert = chain.adopt(gst_element_factory_make("glcolorconvert", NULL));
    if (!glcolorconvert) {
        GST_ERROR_OBJECT(self, "Failed to create glcolorconvert element");
        return FALSE;
    }
    if (!chain.linkChain({glupload, glcolorconvert, capsf, videosink}) || !chain.ghostSink(glupload)) {
        GST_ERROR_OBJECT(self, "Failed to link/ghost glupload→glcolorconvert→capsfilter→qgcqvideosink GPU path");
        return FALSE;
    }
#else
    GstElement* glupload = nullptr;
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    // Memory-aware D3D11 conversion: d3d11upload lifts a software decoder's system-memory frame into
    // D3D11Memory and d3d11convert normalizes any non-Qt-renderable format (e.g. 4:4:4/packed) to a sink
    // format. Both passthrough (no copy) when the HW decoder already delivers an advertised D3D11Memory
    // format, so the zero-copy fast path is preserved. Missing factories fall back to the direct link.
    // D3D11-only is safe: gpuZeroCopyAllowedForCurrentGraphicsApi() routes a D3D12 RHI to the CPU path, so
    // wireGpuPath only runs under D3D11 RHI where the decoder family is aligned to D3D11Memory output.
    GstElement* d3d11upload = gst_element_factory_make("d3d11upload", nullptr);
    GstElement* d3d11convert = gst_element_factory_make("d3d11convert", nullptr);
    if (d3d11upload && d3d11convert) {
        if (!chain.adopt(d3d11upload)) {
            gst_clear_object(&d3d11convert);
            GST_ERROR_OBJECT(self, "Failed to add d3d11upload to GPU path");
            return FALSE;
        }
        if (!chain.adopt(d3d11convert)) {
            GST_ERROR_OBJECT(self, "Failed to add d3d11convert to GPU path");
            return FALSE;
        }
        if (!chain.linkChain({d3d11upload, d3d11convert, capsf, videosink}) || !chain.ghostSink(d3d11upload)) {
            GST_ERROR_OBJECT(self, "Failed to link/ghost d3d11upload→d3d11convert→capsfilter→qgcqvideosink");
            return FALSE;
        }
    } else {
        gst_clear_object(&d3d11upload);
        gst_clear_object(&d3d11convert);
        if (!chain.linkChain({capsf, videosink}) || !chain.ghostSink(capsf)) {
            GST_ERROR_OBJECT(self, "Failed to link/ghost qgcqvideosink (GPU path, d3d11 convert unavailable)");
            return FALSE;
        }
    }
#else
    if (!chain.linkChain({capsf, videosink}) || !chain.ghostSink(capsf)) {
        GST_ERROR_OBJECT(self, "Failed to link/ghost qgcqvideosink (GPU path)");
        return FALSE;
    }
#endif
#endif

    chain.commit();
    self->glupload = glupload;
    self->format_capsfilter = capsf;
    self->videosink = videosink;

#if defined(QGC_GST_BIN_USE_GLUPLOAD)
    GST_INFO_OBJECT(self, "Using glupload→qgcqvideosink GPU path (DMABuf→GL EGLImage import)");
#elif defined(QGC_GST_BIN_USE_DMABUF)
    GST_INFO_OBJECT(self, "Using qgcqvideosink GPU path (direct DMABuf import, no glupload)");
#else
    GST_INFO_OBJECT(self, "Using qgcqvideosink GPU path (native memory passthrough)");
#endif
    return TRUE;
}

// Wire videoconvert -> (optional PAR=1/1) -> format-capsfilter -> qgcqvideosink; self pointers set only on commit.
static gboolean wireCpuPath(GstQgcVideoSinkBin* self, GstElement* videosink, GstElement* capsf)
{
    BinChain chain(GST_BIN(self));
    if (!chain.adopt(videosink)) {
        gst_object_unref(capsf);
        GST_ERROR_OBJECT(self, "Failed to add qgcqvideosink CPU sink element to bin");
        return FALSE;
    }
    if (!chain.adopt(capsf)) {
        GST_ERROR_OBJECT(self, "Failed to add qgcqvideosink CPU capsfilter element to bin");
        return FALSE;
    }

    GstElement* videoconvert = chain.adopt(gst_qgc_video_sink_bin_make_conversion_element(self));
    if (!videoconvert) {
        GST_ERROR_OBJECT(self, "Failed to create video conversion element");
        return FALSE;
    }

    // QVideoSink renders these natively; listing them avoids forcing videoconvert to BGRA.
    GstCaps* caps = gst_caps_from_string(GstQgc::buildCpuCapsString().c_str());
    if (!caps) {
        GST_ERROR_OBJECT(self, "gst_caps_from_string() returned NULL for CPU caps");
        return FALSE;
    }
    g_object_set(capsf, "caps", caps, NULL);
    gst_caps_unref(caps);

    // PAR=1/1 capsfilter normalizes non-square pixels; disable-par for v4l2 drivers without
    // VIDIOC_CROPCAP that deadlock negotiation (GStreamer MR #6242).
    GstElement* par = nullptr;
    if (!self->disable_par) {
        par = chain.adopt(gst_element_factory_make("capsfilter", "qgc-par-filter"));
        if (!par) {
            GST_WARNING_OBJECT(self, "capsfilter factory missing — PAR normalization disabled");
        } else {
            GstCaps* parCaps = gst_caps_from_string("video/x-raw, pixel-aspect-ratio=(fraction)1/1");
            if (parCaps) {
                g_object_set(par, "caps", parCaps, NULL);
                gst_caps_unref(parCaps);
            }
        }
    }

    const bool linked = par ? chain.linkChain({videoconvert, par, capsf, videosink})
                            : chain.linkChain({videoconvert, capsf, videosink});
    if (!linked || !chain.ghostSink(videoconvert)) {
        GST_ERROR_OBJECT(self, "Failed to link/ghost CPU path");
        return FALSE;
    }

    chain.commit();
    self->videoconvert = videoconvert;
    self->par_capsfilter = par;
    self->format_capsfilter = capsf;
    self->videosink = videosink;

    GST_INFO_OBJECT(self, "Using qgcqvideosink CPU path (videoconvert%s → caps → qgcqvideosink → QVideoSink)",
                    par ? " → PAR=1/1" : "");
    return TRUE;
}

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

    GstElement* capsf = gst_element_factory_make("capsfilter", "qgc-format-filter");
    if (!capsf) {
        GST_ERROR_OBJECT(self, "Failed to create capsfilter for qgcqvideosink");
        gst_clear_object(&videosink);
        return;
    }

    // On failure the wire*Path() BinChain rolls the bin back and frees videosink/capsf; self
    // pointers stay NULL so change_state surfaces the construction error.
    if (!(self->gpu_zerocopy ? wireGpuPath(self, videosink, capsf) : wireCpuPath(self, videosink, capsf))) {
        return;
    }

    // Probe the videosink sink pad so ALLOCATION/CONTEXT queries terminate here instead of racing
    // the bus NEED_CONTEXT fallback. Installed post-commit so a wire failure leaves no orphan probe.
    if (GstPad* videosinkPad = gst_element_get_static_pad(videosink, "sink")) {
        const gulong probe_id = gst_pad_add_probe(videosinkPad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM,
                                                  GstQgc::videosinkQueryProbe, NULL, NULL);
        if (probe_id == 0) {
            GST_WARNING_OBJECT(
                self, "gst_pad_add_probe(QUERY_DOWNSTREAM) returned 0 — qgcqvideosink query interception disabled");
        }
        gst_object_unref(videosinkPad);
    }
}

static void gst_qgc_video_sink_bin_constructed(GObject* object)
{
    G_OBJECT_CLASS(gst_qgc_video_sink_bin_parent_class)->constructed(object);
    gst_qgc_video_sink_bin_setup(GST_QGC_VIDEO_SINK_BIN(object));
}

// GstBin dispose unrefs children; NULL our cached pointers BEFORE chaining so a concurrent
// property accessor sees NULL instead of freed memory.
static void gst_qgc_video_sink_bin_dispose(GObject* object)
{
    GstQgcVideoSinkBin* self = GST_QGC_VIDEO_SINK_BIN(object);
    self->videosink = NULL;
    self->videoconvert = NULL;
    self->par_capsfilter = NULL;
    self->format_capsfilter = NULL;
    self->glupload = NULL;
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

gboolean gst_qgc_video_sink_bin_get_gpu_zerocopy(GstElement* bin)
{
    if (!bin || !GST_IS_QGC_VIDEO_SINK_BIN(bin)) {
        return FALSE;
    }
    return GST_QGC_VIDEO_SINK_BIN(bin)->gpu_zerocopy;
}
