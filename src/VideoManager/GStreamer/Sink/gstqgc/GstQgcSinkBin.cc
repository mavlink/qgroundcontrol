#include "GstQgcSinkBin.h"

#include <atomic>
#include <initializer_list>
#include <utility>

#include <QtCore/QLoggingCategory>

#include "GstQgcSinkPolicy.h"
#include "GstScoped.h"
#include "gstqgcqvideosink.h"

Q_DECLARE_LOGGING_CATEGORY(GstQgcGpuBinLog)

namespace {

// A single-element converter, floating: the one of @p factoryName where it suits @p target, else a SoC converter, else
// videoconvertscale (GStreamer 1.22+). Null when none is available.
GstElement* makeConversionElement(const QByteArray& factoryName, const GstQgc::RenderTarget& target)
{
    if (!factoryName.isEmpty()) {
        const char* const name = factoryName.constData();
        const GStreamer::GstFactoryPtr factory = GStreamer::adoptFactory(gst_element_factory_find(name));
        if (factory && !GstQgc::converterSuitsRenderTarget(
                           gst_plugin_feature_get_plugin_name(GST_PLUGIN_FEATURE(factory.get())), name, target)) {
            qCWarning(GstQgcGpuBinLog) << "conversion element" << name << "would draw on the device Qt renders with"
                                       << GstQgc::toString(target.backend) << "- falling through to defaults";
        } else if (GstElement* e = factory ? gst_element_factory_create(factory.get(), NULL) : NULL) {
            qCDebug(GstQgcGpuBinLog) << "Using conversion element override" << name;
            return e;
        } else {
            qCWarning(GstQgcGpuBinLog) << "conversion element" << name
                                       << "factory missing - falling through to defaults";
        }
    }
    // These SoCs' decoders output proprietary layouts under plain video/x-raw formats, which only their converter
    // reads.
    static const char* kSoCFactories[] = {"imxvideoconvert_g2d", "nvvidconv", NULL};
    for (int i = 0; kSoCFactories[i] != NULL; ++i) {
        if (GstElement* e = gst_element_factory_make(kSoCFactories[i], NULL)) {
            qCDebug(GstQgcGpuBinLog) << "Using SoC conversion element" << kSoCFactories[i];
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
bool addConverter(GstElement* bin, const QByteArray& factoryName, const GstQgc::RenderTarget& target, GstElement*& head,
                  GstElement*& tail)
{
    if (GstElement* const converter = makeConversionElement(factoryName, target)) {
        head = tail = addToBin(bin, converter);
        return head != NULL;
    }
    head = addToBin(bin, gst_element_factory_make("videoconvert", NULL));
    tail = addToBin(bin, gst_element_factory_make("videoscale", NULL));
    return linkChain({head, tail});
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

// The GPU variant for the render target of @p environment (GstQgc::selectGpuBin). Any other renderer gets the
// passthrough variant, whose sink offers only memory that renderer imports (a glupload bin under Vulkan would download
// every frame and upload it again).
GstQgc::GpuBinMode selectBinVariant(const GstQgc::SinkEnvironment& environment)
{
    const GstQgc::GpuBinPreference preference = environment.gpuBin;
    const GstQgc::RenderTarget target = environment.target;
    const GstQgc::GpuBinMode mode = GstQgc::selectGpuBin(preference, target);
    if ((mode == GstQgc::GpuBinMode::DmaBufOrGlUpload) && !GStreamer::hasElementFactory("switchbin")) {
        qCDebug(GstQgcGpuBinLog) << "no switchbin: using the glupload GPU bin, where gst-gl imports DMABuf";
        return GstQgc::GpuBinMode::GlUpload;
    }
    if ((mode == GstQgc::GpuBinMode::D3D12Convert) &&
        (!GStreamer::hasElementFactory("d3d12upload") || !GStreamer::hasElementFactory("d3d12convert"))) {
        qCWarning(GstQgcGpuBinLog) << "Qt renders with Direct3D12 but d3d12upload/d3d12convert are missing: using the "
                                      "passthrough GPU bin, which takes system memory";
        return GstQgc::GpuBinMode::Passthrough;
    }
    if (mode != preference.mode) {
        qCDebug(GstQgcGpuBinLog) << "Qt renders with" << GstQgc::toString(target.backend) << "EGL" << target.egl
                                 << "- using the" << GstQgc::gpuBinName(mode) << "GPU bin";
    } else if (preference.explicitChoice && (GstQgc::selectGpuBin({.mode = mode}, target) != mode)) {
        qCWarning(GstQgcGpuBinLog) << "QGC_GST_GPU_BIN forces a GPU bin Qt's renderer doesn't suit:"
                                   << GstQgc::toString(target.backend) << "EGL" << target.egl;
    }
    return mode;
}

// Builds [converter ! (glupload ! glcolorconvert | switchbin | d3d12upload ! d3d12convert !) qgcqvideosink] in @p bin
// and ghosts its head as the bin's "sink" pad. Returns the qgcqvideosink, which the bin owns; null, with the reason
// logged, when that fails.
GstElement* buildChain(GstElement* bin, const GstQgc::SinkBinConfig& config)
{
    GstElement* const videosink = addToBin(bin, GstQgc::makeVideoSink(config.gpuZeroCopy));
    if (!videosink) {
        qCWarning(GstQgcGpuBinLog) << "the bin refused qgcqvideosink";
        return NULL;
    }
    // No QoS events upstream, which a GstVideoSink sends by default. Its "processing-deadline" stays the video sink's
    // own (15 ms; GstBaseSink's is 20 ms): a live pipeline that syncs to the clock adds the deadline to its latency.
    g_object_set(videosink, "sync", (gboolean) FALSE, "qos", (gboolean) FALSE, NULL);
    if (config.environment) {
        GstQgc::setSinkEnvironment(videosink, *config.environment);
    }
    const GstQgc::SinkEnvironment environment =
        config.environment ? *config.environment : GstQgc::currentSinkEnvironment();
    const GstQgc::GpuBinMode gpuBin =
        config.gpuZeroCopy ? selectBinVariant(environment) : GstQgc::GpuBinMode::Passthrough;
    GstQgc::configureSinkCaps(videosink, gpuBin, !config.disablePar);

    GstElement* head = NULL;
    GstElement* tail = NULL;
    if (!addConverter(bin, config.conversionElement, environment.target, head, tail)) {
        qCWarning(GstQgcGpuBinLog) << "Failed to create the video conversion element";
        return NULL;
    }
    if (gpuBin == GstQgc::GpuBinMode::GlUpload) {
        GstElement* const glupload = addToBin(bin, gst_element_factory_make("glupload", NULL));
        // glcolorconvert honours the sink's texture-target=2D; a passthrough when upstream is already 2D.
        GstElement* const glcolorconvert = addToBin(bin, gst_element_factory_make("glcolorconvert", NULL));
        if (!glupload || !glcolorconvert) {
            qCWarning(GstQgcGpuBinLog) << "Failed to create glupload/glcolorconvert";
            return NULL;
        }
        tail = linkChain({tail, glupload, glcolorconvert}) ? glcolorconvert : NULL;
    } else if (gpuBin == GstQgc::GpuBinMode::DmaBufOrGlUpload) {
        GstElement* const memorySwitch = addToBin(bin, makeMemorySwitch());
        if (!memorySwitch) {
            qCWarning(GstQgcGpuBinLog) << "Failed to create switchbin with glupload ! glcolorconvert";
            return NULL;
        }
        tail = linkChain({tail, memorySwitch}) ? memorySwitch : NULL;
    } else if (gpuBin == GstQgc::GpuBinMode::D3D12Convert) {
        // Decoder output passes the converter as D3D12Memory; d3d12upload uploads software-decoded frames.
        GstElement* const upload = addToBin(bin, gst_element_factory_make("d3d12upload", NULL));
        GstElement* const convert = addToBin(bin, gst_element_factory_make("d3d12convert", NULL));
        if (!upload || !convert) {
            qCWarning(GstQgcGpuBinLog) << "Failed to create d3d12upload/d3d12convert";
            return NULL;
        }
        tail = linkChain({tail, upload, convert}) ? convert : NULL;
    }
    const GStreamer::GstPadPtr headPad = GStreamer::adoptPad(gst_element_get_static_pad(head, "sink"));
    GstPad* const ghost =
        (tail && linkChain({tail, videosink}) && headPad) ? gst_ghost_pad_new("sink", headPad.get()) : NULL;
    // The bin takes the floating pad over, and drops it when it refuses it.
    if (!ghost || !gst_element_add_pad(bin, ghost)) {
        qCWarning(GstQgcGpuBinLog) << "Failed to link/ghost the qgcqvideosink chain";
        return NULL;
    }
    qCDebug(GstQgcGpuBinLog) << "Built" << GST_OBJECT_NAME(bin) << "as the"
                             << (config.gpuZeroCopy ? GstQgc::gpuBinName(gpuBin) : "CPU") << "variant"
                             << (config.disablePar ? "(non-square pixels allowed)" : "");
    return videosink;
}

}  // namespace

namespace GstQgc {

SinkBin makeSinkBin(const SinkBinConfig& config)
{
    // Named for the logs, and apart from the other sink bins of a pipeline.
    static std::atomic<unsigned> s_serial{0};
    const GStreamer::GCharPtr name = GStreamer::adoptGChar(g_strdup_printf("qgcvideosinkbin%u", s_serial++));
    // Dropped on failure, and with it what the bin holds by then.
    GStreamer::GstElementPtr bin = GStreamer::adoptFloating(gst_bin_new(name.get()));
    GstElement* const qvideosink = buildChain(bin.get(), config);
    if (!qvideosink) {
        return {};
    }
    return {.bin = std::move(bin), .qvideosink = GStreamer::adoptElement(GST_ELEMENT(gst_object_ref(qvideosink)))};
}

}  // namespace GstQgc
