#include "GStreamerHelpers.h"

#include <algorithm>
#include <gst/rtsp/gstrtspurl.h>
#include <gst/video/video-event.h>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QLatin1String>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GStreamer.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#ifdef Q_OS_WIN
#include "gstqgc/GstQgcRenderTarget.h"
#endif

QGC_LOGGING_CATEGORY(GStreamerHelpersLog, "Video.GStreamer.GStreamerHelpers")

namespace GStreamer {

bool isValidRtspUri(const gchar* uri_str)
{
    if (!uri_str) {
        return false;
    }

    if (!gst_uri_is_valid(uri_str)) {
        return false;
    }

    GstRTSPUrl* url = nullptr;
    const GstRTSPResult res = gst_rtsp_url_parse(uri_str, &url);
    if ((res != GST_RTSP_OK) || !url) {
        if (url) {
            gst_rtsp_url_free(url);
        }
        return false;
    }

    const bool hasHost = (url->host && url->host[0] != '\0');
    gst_rtsp_url_free(url);
    return hasHost;
}

namespace {

void removeAndStop(GstElement* element, bool flush)
{
    const GstObjectPtr parent = element ? adoptObject(gst_object_get_parent(GST_OBJECT(element))) : nullptr;
    if (!parent) {
        return;
    }

    // gst_bin_remove drops the bin's ref, which may be the last one, before the element reaches NULL.
    const GstElementPtr keepAlive = adoptElement(GST_ELEMENT(gst_object_ref(element)));
    (void) gst_bin_remove(GST_BIN(parent.get()), element);
    if (flush) {
        (void) gst_element_foreach_sink_pad(
            element,
            [](GstElement* /*element*/, GstPad* pad, gpointer /*userData*/) -> gboolean {
                (void) gst_pad_send_event(pad, gst_event_new_flush_start());
                return TRUE;
            },
            nullptr);
    }
    // Synchronous: gst_element_change_state() commits an ASYNC return at once when the target is READY or NULL, so
    // the streaming threads have stopped when this returns.
    (void) gst_element_set_state(element, GST_STATE_NULL);
}

}  // namespace

void disposeChild(GstElement* element)
{
    removeAndStop(element, false);
}

void flushAndDisposeChild(GstElement* element)
{
    removeAndStop(element, true);
}

GstElement* addToBin(GstElement* bin, GstElementPtr element, const char* what)
{
    if (!element || g_object_is_floating(element.get())) {
        qCCritical(GStreamerHelpersLog) << "Missing or floating element for" << what;
        return nullptr;
    }
    GstElement* const added = element.release();
    const bool ok = gst_bin_add(GST_BIN(bin), added);
    // The bin took its own reference; drop ours.
    gst_object_unref(added);
    if (!ok) {
        qCWarning(GStreamerHelpersLog) << "gst_bin_add() failed for" << what;
        return nullptr;
    }
    return added;
}

TeeBranch addTeeBranch(GstElement* pipeline, GstElement* tee, const char* what)
{
    GstElementPtr queue = makeElement("queue");
    GstElementPtr valve = makeElement("valve");
    if (!queue || !valve) {
        qCWarning(GStreamerHelpersLog) << "Failed to create the queue and valve of the" << what << "branch";
        return {};
    }
    g_object_set(valve.get(), "drop", TRUE, nullptr);

    const TeeBranch branch{.queue = addToBin(pipeline, std::move(queue), "queue"),
                           .valve = addToBin(pipeline, std::move(valve), "valve")};
    if (!branch.queue || !branch.valve || !gst_element_link_many(tee, branch.queue, branch.valve, nullptr)) {
        qCWarning(GStreamerHelpersLog) << "Unable to link the" << what << "branch";
        return {};
    }
    return branch;
}

void requestKeyframe(GstPad* sinkPad)
{
    if (!sinkPad) {
        return;
    }
    (void) gst_pad_push_event(sinkPad, gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, 0));
}

void sendEventAsync(GstPad* pad, GstEvent* event)
{
    struct PendingEvent
    {
        GstPadPtr pad;
        GstEventPtr event;
    };

    GstElementPtr parent = pad ? adoptElement(gst_pad_get_parent_element(pad)) : nullptr;
    if (!parent) {
        gst_event_unref(event);
        return;
    }
    gst_element_call_async(
        parent.get(),
        [](GstElement* /*element*/, gpointer data) {
            auto* const pending = static_cast<PendingEvent*>(data);
            (void) gst_pad_send_event(pending->pad.get(), pending->event.release());
        },
        new PendingEvent{.pad = adoptPad(GST_PAD(gst_object_ref(pad))), .event = adoptEvent(event)},
        [](gpointer data) { delete static_cast<PendingEvent*>(data); });
}

GstPadPtr firstSrcPad(GstElement* element)
{
    GstPad* first = nullptr;
    // Handles iterator resync itself, unlike a bare gst_iterator_next loop.
    (void) gst_element_foreach_src_pad(
        element,
        [](GstElement* /*element*/, GstPad* pad, gpointer userData) -> gboolean {
            *static_cast<GstPad**>(userData) = GST_PAD(gst_object_ref(pad));
            return FALSE;
        },
        &first);
    return adoptPad(first);
}

std::function<void()> holdPadUntilLinked(GstPad* pad, std::function<void(GstPad* pad)> link)
{
    // Returning OK from a blocking probe keeps the pad blocked until the probe is removed.
    const gulong blockId = gst_pad_add_probe(
        pad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
        [](GstPad* /*pad*/, GstPadProbeInfo* /*info*/, gpointer /*data*/) { return GST_PAD_PROBE_OK; }, nullptr,
        nullptr);
    return [blockId, heldPad = shareObject(pad), link = std::move(link)]() {
        link(heldPad.get());
        gst_pad_remove_probe(heldPad.get(), blockId);
    };
}

void forEachPlugin(GstRegistry* registry, const std::function<void(GstPlugin*)>& visitor)
{
    if (!registry || !visitor)
        return;
    GList* plugins = gst_registry_get_plugin_list(registry);
    for (GList* node = plugins; node != nullptr; node = node->next) {
        GstPlugin* plugin = static_cast<GstPlugin*>(node->data);
        if (plugin)
            visitor(plugin);
    }
    gst_plugin_list_free(plugins);
}

void forEachVideoDecoderFactory(GstRank minRank, const std::function<void(GstElementFactory*)>& visitor)
{
    GList* factories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        minRank);
    for (GList* node = factories; node != nullptr; node = node->next) {
        visitor(GST_ELEMENT_FACTORY(node->data));
    }
    gst_plugin_feature_list_free(factories);
}

namespace {

struct DecoderFamilyPrefix
{
    const char* prefix;
    VideoDecoderOptions family;  // ForceVideoDecoderDefault: not a user-selectable family
    bool isHardware;
};

// Single name-based decoder classification for the hardware/software split, the ForceVideoDecoder*
// rank overrides and availableDecoderFamilies(). Prefix (not substring) matching, so e.g. avdec_wnv1
// isn't taken for an NVIDIA decoder. The first match wins: specific prefixes precede general ones.
constexpr DecoderFamilyPrefix kDecoderFamilyPrefixes[] = {
    // Android MediaCodec: Google/Codec2 software wrappers; every other amcviddec is a hardware codec.
    {"amcviddec-omxgoogle", ForceVideoDecoderDefault, false},
    {"amcviddec-c2android", ForceVideoDecoderDefault, false},
    {"amcviddec-", ForceVideoDecoderDefault, true},
    // Legacy gstreamer-vaapi is demoted on Linux and never bumped by ForceVideoDecoderVAAPI (modern va* only).
    {"vaapi", ForceVideoDecoderDefault, true},
    {"va", ForceVideoDecoderVAAPI, true},
    {"nv", ForceVideoDecoderNVIDIA, true},
    {"d3d11", ForceVideoDecoderDirectX3D, true},
    {"d3d12", ForceVideoDecoderDirectX3D, true},
    {"dxva", ForceVideoDecoderDirectX3D, true},
    {"vtdec", ForceVideoDecoderVideoToolbox, true},
    {"qsv", ForceVideoDecoderIntel, true},
    {"msdk", ForceVideoDecoderIntel, true},
    {"vulkan", ForceVideoDecoderVulkan, true},
};

const DecoderFamilyPrefix* findDecoderFamilyPrefix(const char* factoryName)
{
    const GCharPtr lowerName = adoptGChar(g_ascii_strdown(factoryName, -1));
    for (const DecoderFamilyPrefix& entry : kDecoderFamilyPrefixes) {
        if (g_str_has_prefix(lowerName.get(), entry.prefix)) {
            return &entry;
        }
    }
    return nullptr;
}

}  // namespace

VideoDecoderOptions decoderFamilyForFactory(const char* factoryName)
{
    if (!factoryName) {
        return ForceVideoDecoderDefault;
    }

    const DecoderFamilyPrefix* entry = findDecoderFamilyPrefix(factoryName);
    return entry ? entry->family : ForceVideoDecoderDefault;
}

bool isHardwareDecoderFactory(GstElementFactory* factory)
{
    if (!factory) {
        return false;
    }

    const gchar* factoryName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
    if (!factoryName) {
        return false;
    }

    if (const DecoderFamilyPrefix* entry = findDecoderFamilyPrefix(factoryName)) {
        return entry->isHardware;
    }

    const auto containsHardware = [](const gchar* value) {
        if (!value) {
            return false;
        }
        const GCharPtr lower = adoptGChar(g_ascii_strdown(value, -1));
        return g_strrstr(lower.get(), "hardware") != nullptr;
    };

    return containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS)) ||
           containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_DESCRIPTION));
}

QString describeDecoderFactory(GstElementFactory* factory)
{
    GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
    const GstObjectPtr plugin = adoptObject(GST_OBJECT(gst_plugin_feature_get_plugin(feature)));
    return QStringLiteral("[%1] %2/%3 rank=%4 (%5)")
        .arg(isHardwareDecoderFactory(factory) ? QStringLiteral("HW") : QStringLiteral("SW"),
             plugin ? QString::fromUtf8(gst_plugin_get_name(GST_PLUGIN(plugin.get()))) : QStringLiteral("?"),
             QString::fromUtf8(gst_plugin_feature_get_name(feature)))
        .arg(gst_plugin_feature_get_rank(feature))
        .arg(QString::fromUtf8(gst_element_factory_get_klass(factory)));
}

namespace {

// gst_init() applies GST_PLUGIN_FEATURE_RANK ("name:rank,..."), and nothing re-applies it after QGC's overrides.
bool rankedByUser(const gchar* featureName)
{
    const QList<QByteArray> userRanks = qgetenv("GST_PLUGIN_FEATURE_RANK").split(',');
    return std::ranges::any_of(userRanks, [featureName](const QByteArray& entry) {
        return entry.left(entry.indexOf(':')).trimmed() == featureName;
    });
}

// The rank each feature had before QGC first changed it, which setCodecPriorities() restores before applying an option.
QHash<QByteArray, guint>& originalRanks()
{
    static QHash<QByteArray, guint> ranks;
    return ranks;
}

// Every QGC rank change goes through here, so a feature the user ranked keeps its rank.
void setFeatureRank(GstPluginFeature* feature, uint16_t rank)
{
    const gchar* const name = gst_plugin_feature_get_name(feature);
    if (rankedByUser(name)) {
        qCDebug(GStreamerHelpersLog) << "  Keeping the GST_PLUGIN_FEATURE_RANK rank of" << name;
        return;
    }
    const QByteArray key(name);
    if (!originalRanks().contains(key)) {
        originalRanks().insert(key, gst_plugin_feature_get_rank(feature));
    }
    qCDebug(GStreamerHelpersLog) << "  Changing feature (" << name << ") to use rank:" << rank;
    gst_plugin_feature_set_rank(feature, rank);
}

void restoreOriginalRanks(GstRegistry* registry)
{
    for (auto it = originalRanks().cbegin(); it != originalRanks().cend(); ++it) {
        const GstFeaturePtr feature = adoptFeature(gst_registry_lookup_feature(registry, it.key().constData()));
        if (feature) {
            gst_plugin_feature_set_rank(feature.get(), it.value());
        }
    }
}

}  // namespace

bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank)
{
    if (!registry || !featureName) {
        return false;
    }

    const GstFeaturePtr feature = adoptFeature(gst_registry_lookup_feature(registry, featureName));
    if (!feature) {
        return false;
    }

    setFeatureRank(feature.get(), rank);
    return true;
}

namespace {

// Ranks every installed video decoder that decoderFamilyForFactory() assigns to @p family.
void applyFamilyRank(VideoDecoderOptions family, uint16_t rank)
{
    forEachVideoDecoderFactory(GST_RANK_NONE, [family, rank](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        if (decoderFamilyForFactory(gst_plugin_feature_get_name(feature)) == family) {
            setFeatureRank(feature, rank);
        }
    });
}

// Ranks @p family's video decoders, at least @p minimumRank, above every other video decoder: GStreamer already ranks
// some PRIMARY + 1 (the d3d11 and va ones), which a tie would leave winning. The others stay as fallbacks for the
// codecs the family can't decode.
void prioritizeFamily(VideoDecoderOptions family, uint16_t minimumRank)
{
    guint highestOtherRank = GST_RANK_NONE;
    forEachVideoDecoderFactory(GST_RANK_NONE, [family, &highestOtherRank](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        if (decoderFamilyForFactory(gst_plugin_feature_get_name(feature)) != family) {
            highestOtherRank = std::max(highestOtherRank, gst_plugin_feature_get_rank(feature));
        }
    });
    const guint rank = std::max<guint>(minimumRank, std::min<guint>(highestOtherRank, G_MAXUINT16 - 1) + 1);
    applyFamilyRank(family, static_cast<uint16_t>(rank));
}

void lowerDecoderRanksByClass(bool lowerHardware)
{
    forEachVideoDecoderFactory(GST_RANK_MARGINAL, [lowerHardware](GstElementFactory* factory) {
        if (isHardwareDecoderFactory(factory) != lowerHardware) {
            return;
        }
        setFeatureRank(GST_PLUGIN_FEATURE(factory), GST_RANK_NONE);
    });
}

void prioritizeByHardwareClass(uint16_t prioritizedRank, bool requireHardware)
{
    qCDebug(GStreamerHelpersLog) << "Prioritizing" << (requireHardware ? "hardware" : "software")
                                 << "video decoders with rank:" << prioritizedRank;
    int decoderFactories = 0;
    int matchedFactories = 0;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&](GstElementFactory* factory) {
        ++decoderFactories;
        if (isHardwareDecoderFactory(factory) == requireHardware) {
            setFeatureRank(GST_PLUGIN_FEATURE(factory), prioritizedRank);
            ++matchedFactories;
        }
    });

    if (decoderFactories == 0) {
        qCDebug(GStreamerHelpersLog) << "No decoder factories available while prioritizing"
                                     << (requireHardware ? "hardware" : "software") << "decoders";
        return;
    }

    if (matchedFactories == 0) {
        qCWarning(GStreamerHelpersLog) << "No" << (requireHardware ? "hardware" : "software")
                                       << "video decoder factories found to reprioritize.";
    }

    qCDebug(GStreamerHelpersLog) << "Lowering" << (requireHardware ? "software" : "hardware") << "decoder ranks.";
    lowerDecoderRanksByClass(!requireHardware);
}

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
// Above the PRIMARY + 1 the ForceVideoDecoder* options give, so the GPU-memory decoders win autoplug.
constexpr uint16_t kZeroCopyRank = GST_RANK_PRIMARY + 2;

// Ranks every installed video decoder whose factory name starts with @p prefix.
void applyPrefixRank(const char* prefix, uint16_t rank)
{
    forEachVideoDecoderFactory(GST_RANK_NONE, [prefix, rank](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        if (g_str_has_prefix(gst_plugin_feature_get_name(feature), prefix)) {
            setFeatureRank(feature, rank);
        }
    });
}
#endif

#ifdef Q_OS_WIN
// A decoder whose D3D family mismatches the active QRhi backend can't be sampled zero-copy and drops to a CPU copy.
void alignD3DDecoderRanksToRhi(bool promoteMatchedFamily)
{
    // Ranks usually apply before the scene graph resolves its backend, while the render target is still Unknown: fall
    // back to QSG_RHI_BACKEND (Qt's D3D11 default) then.
    bool wantD3D12 = false;
    switch (GstQgc::currentRenderTarget().backend) {
        case GstQgc::RenderBackend::Direct3D12:
            wantD3D12 = true;
            break;
        case GstQgc::RenderBackend::Unknown:
            wantD3D12 =
                qEnvironmentVariable("QSG_RHI_BACKEND").compare(QLatin1String("d3d12"), Qt::CaseInsensitive) == 0;
            break;
        default:
            break;
    }
    const char* const matched = wantD3D12 ? "d3d12" : "d3d11";
    const char* const mismatched = wantD3D12 ? "d3d11" : "d3d12";
    qCDebug(GStreamerHelpersLog) << "Aligning D3D decoder ranks to" << (wantD3D12 ? "D3D12" : "D3D11")
                                 << "RHI - demoting the mismatched decoder family";
    if (promoteMatchedFamily) {
        applyPrefixRank(matched, kZeroCopyRank);
    }
    applyPrefixRank(mismatched, GST_RANK_NONE);
}
#endif  // Q_OS_WIN

#ifdef Q_OS_LINUX
// Steer autoplug toward zero-copy GPU-memory decoders (DMABuf/DMA_DRM) over system-memory ones. Rank-only and
// additive: absent factories no-op, va/v4l2 self-demote at runtime without a device, software fallback untouched.
void preferZeroCopyDecoders()
{
    // Legacy gstreamer-vaapi negotiates GstVaapiMemory the qgcqvideosink/HwBuffers paths can't import
    // zero-copy; demote it so the modern va plugin (clean DMABuf/DMA_DRM output) wins when both exist.
    applyPrefixRank("vaapi", GST_RANK_NONE);

    // Modern va decoders (gst-va, supersedes gstreamer-vaapi) emit DMABuf/DMA_DRM, and V4L2 stateless
    // decoders expose DMA_DRM caps on 1.26+; bump both above software so the zero-copy allocator is picked.
    applyFamilyRank(ForceVideoDecoderVAAPI, kZeroCopyRank);
    applyPrefixRank("v4l2sl", kZeroCopyRank);
}
#endif  // Q_OS_LINUX

}  // anonymous namespace

void setCodecPriorities(int rawOption)
{
    // Validate the raw setting here so the enum switch below stays default-free (-Wswitch).
    if ((rawOption < ForceVideoDecoderDefault) || (rawOption > ForceVideoDecoderHardware)) {
        qCWarning(GStreamerHelpersLog) << "Ignoring invalid decode option:" << rawOption;
        return;
    }
    setCodecPriorities(static_cast<VideoDecoderOptions>(rawOption));
}

void setCodecPriorities(VideoDecoderOptions option)
{
    if (!gst_is_initialized()) {
        return;
    }
    GstRegistry* const registry = gst_registry_get();
    if (!registry) {
        qCCritical(GStreamerHelpersLog) << "Failed to get gstreamer registry.";
        return;
    }
    restoreOriginalRanks(registry);

    static constexpr uint16_t PrioritizedRank = GST_RANK_PRIMARY + 1;

    switch (option) {
        case ForceVideoDecoderDefault:
            // HW-decoder GPU caps (GLMemory/DMABuf/VAMemory) auto-plug to system memory via gldownload/vapostproc.
#ifdef Q_OS_LINUX
            preferZeroCopyDecoders();
#endif
            break;
        case ForceVideoDecoderSoftware:
            prioritizeByHardwareClass(PrioritizedRank, false);
            break;
        case ForceVideoDecoderHardware:
            prioritizeByHardwareClass(PrioritizedRank, true);
            break;
        case ForceVideoDecoderVulkan:
            // Vulkan zero-copy import is dormant: gst-vulkan's own VkDevice fails QGC's device-match guard → CPU copy.
            qCWarning(GStreamerHelpersLog) << "Forcing Vulkan video decoders: zero-copy import is dormant "
                                              "(foreign VkDevice → CPU fallback), so decode will not be zero-copy.";
            [[fallthrough]];
        case ForceVideoDecoderVAAPI:
        case ForceVideoDecoderNVIDIA:
        case ForceVideoDecoderDirectX3D:
        case ForceVideoDecoderVideoToolbox:
        case ForceVideoDecoderIntel:
            prioritizeFamily(option, PrioritizedRank);
            break;
    }

#ifdef Q_OS_WIN
    // After any option-driven rank changes, force the D3D decoder API to match the active QRhi
    // backend so a D3D12 decoder never wins over D3D11 on Qt's default Windows RHI (and vice-versa).
    // Forcing the D3D family already ranks it above every other decoder, so only the mismatched API is demoted.
    const bool promoteMatchingD3D = (option == ForceVideoDecoderDefault) || (option == ForceVideoDecoderHardware);
    alignD3DDecoderRanksToRhi(promoteMatchingD3D);
#endif
}

}  // namespace GStreamer
