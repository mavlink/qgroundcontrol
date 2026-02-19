#include "GStreamerHelpers.h"

#include <gst/rtsp/gstrtspurl.h>
#include <QtCore/QLatin1String>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

namespace GStreamer
{

gboolean
isValidRtspUri(const gchar *uri_str)
{
    if (!uri_str) {
        return FALSE;
    }

    GstRTSPUrl *url = NULL;
    GstRTSPResult res;

    if (!gst_uri_is_valid(uri_str)) {
        return FALSE;
    }

    res = gst_rtsp_url_parse(uri_str, &url);
    if ((res != GST_RTSP_OK) || (url == NULL)) {
        if (url) {
            gst_rtsp_url_free(url);
        }
        return FALSE;
    }

    const gboolean hasHost = (url->host && url->host[0] != '\0');
    gst_rtsp_url_free(url);
    return hasHost;
}

bool isHardwareDecoderFactory(GstElementFactory *factory)
{
    if (!factory) {
        return false;
    }

    const gchar *factoryName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
    if (!factoryName) {
        return false;
    }

    const QString nameLower = QString::fromUtf8(factoryName).toLower();

    // Android MediaCodec: exclude software wrappers, accept remaining as hardware
    if (nameLower.startsWith("amcviddec-omxgoogle") || nameLower.startsWith("amcviddec-c2android")) {
        return false;
    }
    if (nameLower.startsWith("amcviddec-")) {
        return true;
    }

    const auto containsHardware = [](const gchar *value) {
        if (!value) return false;
        gchar *lower = g_ascii_strdown(value, -1);
        bool found = (g_strrstr(lower, "hardware") != nullptr);
        g_free(lower);
        return found;
    };

    if (containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS))) {
        return true;
    }

    if (containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_DESCRIPTION))) {
        return true;
    }

    static constexpr QLatin1String kHardwareTags[] = {
        QLatin1String("va"),
        QLatin1String("nv"),
        QLatin1String("qsv"),
        QLatin1String("msdk"),
        QLatin1String("vulkan"),
        QLatin1String("d3d"),
        QLatin1String("dxva"),
        QLatin1String("vtdec"),
        QLatin1String("metal"),
    };

    for (const auto &tag : kHardwareTags) {
        if (nameLower.contains(tag)) {
            return true;
        }
    }

    return false;
}

namespace {

void changeFeatureRank(GstRegistry *registry, const char *featureName, uint16_t rank)
{
    if (!registry || !featureName) {
        return;
    }

    GstPluginFeature *feature = gst_registry_lookup_feature(registry, featureName);
    if (!feature) {
        qCDebug(GStreamerLog) << "Failed to change ranking of feature. Feature does not exist:" << featureName;
        return;
    }

    qCDebug(GStreamerLog) << "  Changing feature (" << featureName << ") to use rank:" << rank;
    gst_plugin_feature_set_rank(feature, rank);
    gst_clear_object(&feature);
}

void lowerSoftwareDecoderRanks(GstRegistry *registry)
{
    static constexpr uint16_t NewRank = GST_RANK_NONE;
    if (!registry) {
        qCCritical(GStreamerLog) << "Invalid registry!";
        return;
    }

    GList *decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    for (GList *node = decoderFactories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) {
            continue;
        }

        if (GStreamer::isHardwareDecoderFactory(factory)) {
            continue;
        }

        const gchar *name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        if (!name) {
            continue;
        }

        qCDebug(GStreamerLog) << "Setting software decoder rank low:" << name << " rank:" << NewRank;
        gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory), NewRank);
    }

    gst_plugin_feature_list_free(decoderFactories);
}

void prioritizeByHardwareClass(GstRegistry *registry, uint16_t prioritizedRank, bool requireHardware)
{
    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get gstreamer registry.";
        return;
    }

    GList *decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!decoderFactories) {
        qCDebug(GStreamerLog) << "No decoder factories available while prioritizing"
                              << (requireHardware ? "hardware" : "software") << "decoders";
        return;
    }

    qCDebug(GStreamerLog) << "Prioritizing" << (requireHardware ? "hardware" : "software")
                           << "video decoders with rank:" << prioritizedRank;
    int matchedFactories = 0;
    for (GList *node = decoderFactories; node != nullptr; node = node->next) {
        GstElementFactory *factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) {
            continue;
        }

        if (GStreamer::isHardwareDecoderFactory(factory) != requireHardware) {
            continue;
        }

        const gchar *featureName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        if (!featureName) {
            continue;
        }

        changeFeatureRank(registry, featureName, prioritizedRank);
        ++matchedFactories;
    }

    if (matchedFactories == 0) {
        qCWarning(GStreamerLog) << "No" << (requireHardware ? "hardware" : "software")
                               << "video decoder factories found to reprioritize.";
    }

    // Lower software decoder rank when using hardware decoders
    if (requireHardware) {
        qCDebug(GStreamerLog) << "Lowering software decoder ranks for hardware priority.";
        lowerSoftwareDecoderRanks(registry);
    }

    gst_plugin_feature_list_free(decoderFactories);
}

} // anonymous namespace

void setCodecPriorities(VideoDecoderOptions option)
{
    GstRegistry *registry = gst_registry_get();

    if (!registry) {
        qCCritical(GStreamerLog) << "Failed to get gstreamer registry.";
        return;
    }

    static constexpr uint16_t PrioritizedRank = GST_RANK_PRIMARY + 1;

    switch (option) {
    case ForceVideoDecoderDefault:
        // Android/iOS hardware decoders (amcviddec, vtdec) already have higher
        // default ranks than software decoders, so decodebin tries them first.
        // However, AMC decoders require GL output (GLMemory caps) which the
        // current pipeline doesn't support — so they fail and decodebin falls
        // back to software. Setting software to RANK_NONE here would eliminate
        // that fallback entirely.  Leave ranks untouched until the pipeline
        // supports GL context sharing with Qt.
        break;
    case ForceVideoDecoderSoftware:
        prioritizeByHardwareClass(registry, PrioritizedRank, false);
        break;
    case ForceVideoDecoderHardware:
        prioritizeByHardwareClass(registry, PrioritizedRank, true);
        break;
    case ForceVideoDecoderVAAPI:
        for (const char *name : {"vaav1dec", "vah264dec", "vah265dec", "vajpegdec", "vampeg2dec", "vavp8dec", "vavp9dec"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    case ForceVideoDecoderNVIDIA:
        for (const char *name : {"nvav1dec", "nvh264dec", "nvh265dec", "nvjpegdec", "nvmpeg2videodec", "nvmpeg4videodec", "nvmpegvideodec", "nvvp8dec", "nvvp9dec"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    case ForceVideoDecoderDirectX3D:
        for (const char *name : {"d3d11av1dec", "d3d11h264dec", "d3d11h265dec", "d3d11mpeg2dec", "d3d11vp8dec", "d3d11vp9dec",
                                 "d3d12av1dec", "d3d12h264dec", "d3d12h265dec", "d3d12mpeg2dec", "d3d12vp8dec", "d3d12vp9dec",
                                 "dxvaav1decoder", "dxvah264decoder", "dxvah265decoder", "dxvampeg2decoder", "dxvavp8decoder", "dxvavp9decoder"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    case ForceVideoDecoderVideoToolbox:
        for (const char *name : {"vtdec_hw", "vtdec"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    case ForceVideoDecoderIntel:
        for (const char *name : {"qsvh264dec", "qsvh265dec", "qsvjpegdec", "qsvvp9dec", "msdkav1dec", "msdkh264dec", "msdkh265dec", "msdkmjpegdec", "msdkmpeg2dec", "msdkvc1dec", "msdkvp8dec", "msdkvp9dec"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    case ForceVideoDecoderVulkan:
        for (const char *name : {"vulkanh264dec", "vulkanh265dec"}) {
            changeFeatureRank(registry, name, PrioritizedRank);
        }
        break;
    default:
        qCWarning(GStreamerLog) << "Can't handle decode option:" << option;
        break;
    }
}

} // namespace GStreamer
