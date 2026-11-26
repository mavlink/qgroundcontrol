#include "GstQgcSinkBin.h"

#include <atomic>
#include <initializer_list>

#include "GstQgcGpuBin.h"
#include "GstQgcRenderTarget.h"
#include "GstScoped.h"
#include "gstqgcelements.h"
#include "gstqgcqvideosink.h"

#define GST_CAT_DEFAULT gst_qgc_debug

namespace {

constexpr char kVideoSinkName[] = "qgcqvideosink";

// A single-element converter, floating: the one of @p factoryName, else a SoC converter, else videoconvertscale
// (GStreamer 1.22+). Null when none is available.
GstElement* makeConversionElement(GstElement* bin, const QByteArray& factoryName)
{
    if (!factoryName.isEmpty()) {
        const char* const name = factoryName.constData();
        const GStreamer::GstFactoryPtr factory = GStreamer::adoptFactory(gst_element_factory_find(name));
        const GstQgc::RenderTarget target = GstQgc::currentRenderTarget();
        if (factory && !GstQgc::converterSuitsRenderTarget(
                           gst_plugin_feature_get_plugin_name(GST_PLUGIN_FEATURE(factory.get())), name, target)) {
            GST_WARNING_OBJECT(bin,
                               "conversion element '%s' would draw on the device Qt renders with (%s) — falling "
                               "through to defaults",
                               name, GstQgc::toString(target.backend));
        } else if (GstElement* e = factory ? gst_element_factory_create(factory.get(), NULL) : NULL) {
            GST_INFO_OBJECT(bin, "Using conversion element override '%s'", name);
            return e;
        } else {
            GST_WARNING_OBJECT(bin, "conversion element '%s' factory missing — falling through to defaults", name);
        }
    }
    // These SoCs' decoders output proprietary layouts under plain video/x-raw formats, which only their converter
    // reads.
    static const char* kSoCFactories[] = {"imxvideoconvert_g2d", "nvvidconv", NULL};
    for (int i = 0; kSoCFactories[i] != NULL; ++i) {
        if (GstElement* e = gst_element_factory_make(kSoCFactories[i], NULL)) {
            GST_INFO_OBJECT(bin, "Using SoC conversion element '%s'", kSoCFactories[i]);
            return e;
        }
    }
    return gst_element_factory_make("videoconvertscale", NULL);
}

// Adds @p element, floating, to @p bin, which then owns it: the element, or null when there is none or the bin refuses
// it.
GstElement* addToBin(GstElement* bin, GstElement* element)
{
    return (element && gst_bin_add(GST_BIN(bin), element)) ? element : NULL;
}

// Links @p chain in flow order; false when an element is missing or two of them don't link.
bool linkChain(std::initializer_list<GstElement*> chain)
{
    GstElement* previous = NULL;
    for (GstElement* const element : chain) {
        if (!element || (previous && !gst_element_link(previous, element))) {
            return false;
        }
        previous = element;
    }
    return true;
}

// Adds the bin's head, which converts and scales system memory into what qgcqvideosink offers (scaling lets its
// square-pixel caps take non-square-pixel sources) and passes GPU memory through. Before GStreamer 1.22 that's
// videoconvert ! videoscale; @p head and @p tail are its ends.
bool addConverter(GstElement* bin, const QByteArray& factoryName, GstElement*& head, GstElement*& tail)
{
    if (GstElement* const converter = makeConversionElement(bin, factoryName)) {
        head = tail = addToBin(bin, converter);
        return head != NULL;
    }
    head = addToBin(bin, gst_element_factory_make("videoconvert", NULL));
    tail = addToBin(bin, gst_element_factory_make("videoscale", NULL));
    return linkChain({head, tail});
}

// switchbin offers upstream what its paths take. Before GStreamer 1.24 it didn't ask the sink what a path without an
// element takes, so it would offer every DMABuf layout, importable or not.
bool memorySwitchAvailable()
{
    guint major = 0;
    guint minor = 0;
    guint micro = 0;
    guint nano = 0;
    gst_version(&major, &minor, &micro, &nano);
    return ((major > 1) || ((major == 1) && (minor >= 24))) && GStreamer::hasElementFactory("switchbin");
}

// The switchbin of the DMABuf-or-glupload variant, floating; null when its elements are missing. A caps event picks
// the path, so the memory a decoder negotiates does: DMABuf goes straight on, and the sink offers only layouts its
// importer takes; GLMemory and system memory go through glupload ! glcolorconvert.
GstElement* makeMemorySwitch()
{
    GstElement* memorySwitch = gst_element_factory_make("switchbin", NULL);
    GstElement* upload =
        gst_parse_bin_from_description_full("glupload ! glcolorconvert", TRUE, NULL, GST_PARSE_FLAG_FATAL_ERRORS, NULL);
    if (!memorySwitch || !upload) {
        gst_clear_object(&memorySwitch);
        gst_clear_object(&upload);
        return NULL;
    }
    const GStreamer::GstCapsPtr direct = GStreamer::adoptCaps(gst_caps_from_string("video/x-raw(memory:DMABuf)"));
    const GStreamer::GstCapsPtr uploaded =
        GStreamer::adoptCaps(gst_caps_from_string("video/x-raw(memory:GLMemory); video/x-raw"));
    g_object_set(memorySwitch, "num-paths", (guint) 2, NULL);
    // The path takes ownership of the upload chain.
    gst_child_proxy_set(GST_CHILD_PROXY(memorySwitch), "path0::caps", direct.get(), "path1::caps", uploaded.get(),
                        "path1::element", upload, NULL);
    return memorySwitch;
}

// Has the sink answer a CONTEXT query that reaches the switchbin. Until a stream's first caps event the switchbin has
// no path, so the query of an element that asks downstream for Qt's device as it starts (a decoder with GL output)
// would end there, and the element would make a device of its own.
GstPadProbeReturn answerContextQueryFromSink(GstPad*, GstPadProbeInfo* info, gpointer sinkPad)
{
    GstQuery* const query = GST_PAD_PROBE_INFO_QUERY(info);
    if ((GST_QUERY_TYPE(query) == GST_QUERY_CONTEXT) && gst_pad_query(GST_PAD(sinkPad), query)) {
        return GST_PAD_PROBE_HANDLED;
    }
    return GST_PAD_PROBE_OK;
}

// The GPU variant for the render target (GstQgc::selectGpuBin). Any other renderer gets the passthrough variant, whose
// sink offers only memory that renderer imports (a glupload bin under Vulkan would download every frame and upload it
// again).
GstQgc::GpuBinMode selectBinVariant(GstElement* bin)
{
    const GstQgc::GpuBinPreference preference = GstQgc::gpuBinPreference();
    const GstQgc::RenderTarget target = GstQgc::currentRenderTarget();
    const GstQgc::GpuBinMode mode = GstQgc::selectGpuBin(preference, target);
    if ((mode == GstQgc::GpuBinMode::DmaBufOrGlUpload) && !memorySwitchAvailable()) {
        GST_INFO_OBJECT(bin,
                        "no switchbin of GStreamer 1.24 or later: using the glupload GPU bin, where gst-gl "
                        "imports DMABuf");
        return GstQgc::GpuBinMode::GlUpload;
    }
    if ((mode == GstQgc::GpuBinMode::D3D12Convert) &&
        (!GStreamer::hasElementFactory("d3d12upload") || !GStreamer::hasElementFactory("d3d12convert"))) {
        GST_WARNING_OBJECT(bin,
                           "Qt renders with Direct3D12 but d3d12upload/d3d12convert are missing: using the "
                           "passthrough GPU bin, which takes system memory");
        return GstQgc::GpuBinMode::Passthrough;
    }
    if (mode != preference.mode) {
        GST_INFO_OBJECT(bin, "Qt renders with %s (EGL %d): using the %s GPU bin", GstQgc::toString(target.backend),
                        target.egl, GstQgc::gpuBinName(mode));
    } else if (preference.explicitChoice && (GstQgc::selectGpuBin({.mode = mode}, target) != mode)) {
        GST_WARNING_OBJECT(bin, "QGC_GST_GPU_BIN forces a GPU bin Qt's renderer doesn't suit (%s, EGL %d)",
                           GstQgc::toString(target.backend), target.egl);
    }
    return mode;
}

// Builds [converter ! (glupload ! glcolorconvert | switchbin | d3d12upload ! d3d12convert !) qgcqvideosink] in @p bin
// and ghosts its head as the bin's "sink" pad. False, with the reason logged, when that fails.
bool buildChain(GstElement* bin, const GstQgc::SinkBinConfig& config)
{
    // gpu-zerocopy is construct-only — pass it to construction; g_object_set after the fact is rejected.
    GstElement* const videosink =
        addToBin(bin, gst_element_factory_make_full("qgcqvideosink", "name", kVideoSinkName, "gpu-zerocopy",
                                                    (gboolean) config.gpuZeroCopy, NULL));
    if (!videosink) {
        GST_ERROR_OBJECT(bin, "Failed to create qgcqvideosink element");
        return false;
    }
    // No QoS events upstream, which a GstVideoSink sends by default. Its "processing-deadline" stays the video sink's
    // own (15 ms; GstBaseSink's is 20 ms): a live pipeline that syncs to the clock adds the deadline to its latency.
    g_object_set(videosink, "active", (gboolean) TRUE, "sync", (gboolean) FALSE, "qos", (gboolean) FALSE, NULL);
    const GstQgc::GpuBinMode gpuBin = config.gpuZeroCopy ? selectBinVariant(bin) : GstQgc::GpuBinMode::Passthrough;
    GstQgc::configureSinkCaps(videosink, gpuBin, !config.disablePar);

    GstElement* head = NULL;
    GstElement* tail = NULL;
    if (!addConverter(bin, config.conversionElement, head, tail)) {
        GST_ERROR_OBJECT(bin, "Failed to create the video conversion element");
        return false;
    }
    if (gpuBin == GstQgc::GpuBinMode::GlUpload) {
        GstElement* const glupload = addToBin(bin, gst_element_factory_make("glupload", NULL));
        // glcolorconvert honours the sink's texture-target=2D; a passthrough when upstream is already 2D.
        GstElement* const glcolorconvert = addToBin(bin, gst_element_factory_make("glcolorconvert", NULL));
        if (!glupload || !glcolorconvert) {
            GST_ERROR_OBJECT(bin, "Failed to create glupload/glcolorconvert");
            return false;
        }
        tail = linkChain({tail, glupload, glcolorconvert}) ? glcolorconvert : NULL;
    } else if (gpuBin == GstQgc::GpuBinMode::DmaBufOrGlUpload) {
        GstElement* const memorySwitch = addToBin(bin, makeMemorySwitch());
        if (!memorySwitch) {
            GST_ERROR_OBJECT(bin, "Failed to create switchbin with glupload ! glcolorconvert");
            return false;
        }
        const GStreamer::GstPadPtr switchPad = GStreamer::adoptPad(gst_element_get_static_pad(memorySwitch, "sink"));
        GstPad* sinkPad = gst_element_get_static_pad(videosink, "sink");
        if (!switchPad || !sinkPad) {
            gst_clear_object(&sinkPad);
            GST_ERROR_OBJECT(bin, "switchbin or qgcqvideosink has no sink pad");
            return false;
        }
        // The probe owns the sink pad's reference.
        gst_pad_add_probe(switchPad.get(), GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, answerContextQueryFromSink, sinkPad,
                          gst_object_unref);
        tail = linkChain({tail, memorySwitch}) ? memorySwitch : NULL;
    } else if (gpuBin == GstQgc::GpuBinMode::D3D12Convert) {
        // Decoder output passes the converter as D3D12Memory; d3d12upload uploads software-decoded frames.
        GstElement* const upload = addToBin(bin, gst_element_factory_make("d3d12upload", NULL));
        GstElement* const convert = addToBin(bin, gst_element_factory_make("d3d12convert", NULL));
        if (!upload || !convert) {
            GST_ERROR_OBJECT(bin, "Failed to create d3d12upload/d3d12convert");
            return false;
        }
        tail = linkChain({tail, upload, convert}) ? convert : NULL;
    }
    const GStreamer::GstPadPtr headPad = GStreamer::adoptPad(gst_element_get_static_pad(head, "sink"));
    GstPad* const ghost =
        (tail && linkChain({tail, videosink}) && headPad) ? gst_ghost_pad_new("sink", headPad.get()) : NULL;
    // The bin takes the floating pad over, and drops it when it refuses it.
    if (!ghost || !gst_element_add_pad(bin, ghost)) {
        GST_ERROR_OBJECT(bin, "Failed to link/ghost the qgcqvideosink chain");
        return false;
    }
    GST_INFO_OBJECT(bin, "qgcqvideosink %s bin%s", config.gpuZeroCopy ? GstQgc::gpuBinName(gpuBin) : "CPU",
                    config.disablePar ? " (non-square pixels allowed)" : "");
    return true;
}

}  // namespace

namespace GstQgc {

GstElement* makeSinkBin(const SinkBinConfig& config)
{
    // The log category, which the plugin's registration sets up: a bin asked for before that logs why it has no sink.
    qgc_element_init(NULL);
    // Named for the logs, and apart from the other sink bins of a pipeline.
    static std::atomic<unsigned> s_serial{0};
    const GStreamer::GCharPtr name = GStreamer::adoptGChar(g_strdup_printf("qgcvideosinkbin%u", s_serial++));
    GstElement* const bin = gst_bin_new(name.get());
    if (!buildChain(bin, config)) {
        // The floating reference, and with it what the bin holds by now.
        gst_object_unref(bin);
        return NULL;
    }
    return bin;
}

GstElement* sinkBinVideoSink(GstElement* sinkBin)
{
    GstElement* const videosink =
        (sinkBin && GST_IS_BIN(sinkBin)) ? gst_bin_get_by_name(GST_BIN(sinkBin), kVideoSinkName) : NULL;
    if (videosink && !GST_IS_QGC_Q_VIDEO_SINK(videosink)) {
        gst_object_unref(videosink);
        return NULL;
    }
    return videosink;
}

}  // namespace GstQgc
