#include "GStreamerHelpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLatin1String>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <array>
#include <gst/rtsp/gstrtspurl.h>
#include <span>

#ifdef Q_OS_WIN
#include <iterator>
#if defined(QGC_HAS_ANY_GPU_PATH)
#include <rhi/qrhi.h>

#include "HwBuffers/common/QGCRhiCapture.h"
#endif
#endif

#include "GStreamer.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"

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

QString dotFileTag(QStringView tag)
{
    // Fits a 255-byte file name with the "-yyyyMMdd-HHmmss-zzz.dot" suffix writePipelineDot() adds, even at 4 UTF-8
    // bytes a character.
    constexpr qsizetype kMaxTagLength = (255 - 24) / 4;

    QString safe;
    for (const QChar c : tag.left(kMaxTagLength)) {
        safe.append((c.isLetterOrNumber() || (c == u'-') || (c == u'_')) ? c : u'_');
    }
    return safe.isEmpty() ? QStringLiteral("graph") : safe;
}

QString writePipelineDot(GstElement* pipeline, QStringView tag, const QString& directory)
{
    // kMaxDotFiles: cap retained .dot pipeline graphs; below 5 makes retro-debugging hard.
    constexpr int kMaxDotFiles = 10;

    if (!pipeline || directory.isEmpty()) {
        return {};
    }
    QDir dir(directory);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        qCWarning(GStreamerHelpersLog) << "Failed to create" << dir.absolutePath();
        return {};
    }

    // Rotate: remove oldest .dot files until under cap. Sort by mtime ascending.
    QFileInfoList existing = dir.entryInfoList(QStringList{QStringLiteral("*.dot")},
                                               QDir::Files, QDir::Time | QDir::Reversed);
    while (existing.size() >= kMaxDotFiles) {
        QFile::remove(existing.takeFirst().absoluteFilePath());
    }

    const GCharPtr data = adoptGChar(gst_debug_bin_to_dot_data(GST_BIN(pipeline), kDiagnosticDotGraphDetails));
    if (!data) {
        return {};
    }
    const QString fileName =
        QStringLiteral("%1-%2.dot")
            .arg(dotFileTag(tag), QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")));
    const QString fullPath = dir.absoluteFilePath(fileName);
    const QByteArray dotData = QByteArray::fromRawData(data.get(), static_cast<qsizetype>(qstrlen(data.get())));
    bool wrote = false;
    QFile out(fullPath);
    if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        wrote = (out.write(dotData) == static_cast<qint64>(dotData.size()));
        if (!out.flush()) {
            wrote = false;
        }
        out.close();
        if (out.error() != QFileDevice::NoError) {
            wrote = false;
        }
    }
    return wrote ? fullPath : QString{};
}

void disposeChild(GstBin* bin, GstElement* element)
{
    if (!bin || !element || !gst_object_has_as_parent(GST_OBJECT(element), GST_OBJECT(bin))) {
        return;
    }

    // gst_bin_remove drops the bin's ref, which may be the last one, before the element reaches NULL.
    const GstElementPtr keepAlive = adoptElement(GST_ELEMENT(gst_object_ref(element)));
    (void) gst_bin_remove(bin, element);
    // Synchronous: gst_element_change_state() commits an ASYNC return at once when the target is READY or NULL, so
    // the streaming threads have stopped when this returns.
    (void) gst_element_set_state(element, GST_STATE_NULL);
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
        if (!value)
            return false;
        gchar* lower = g_ascii_strdown(value, -1);
        bool found = (g_strrstr(lower, "hardware") != nullptr);
        g_free(lower);
        return found;
    };

    return containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_KLASS)) ||
           containsHardware(gst_element_factory_get_metadata(factory, GST_ELEMENT_METADATA_DESCRIPTION));
}

bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank)
{
    if (!registry || !featureName) {
        return false;
    }

    const GstFeaturePtr feature = adoptFeature(gst_registry_lookup_feature(registry, featureName));
    if (!feature) {
        return false;
    }

    qCDebug(GStreamerHelpersLog) << "  Changing feature (" << featureName << ") to use rank:" << rank;
    gst_plugin_feature_set_rank(feature.get(), rank);
    return true;
}

namespace {

// Ranks every installed video decoder that decoderFamilyForFactory() assigns to @p family.
void applyFamilyRank(GstRegistry* registry, VideoDecoderOptions family, uint16_t rank)
{
    GList* decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);
    for (GList* node = decoderFactories; node != nullptr; node = node->next) {
        const gchar* name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(node->data));
        if (decoderFamilyForFactory(name) == family) {
            changeFeatureRank(registry, name, rank);
        }
    }
    gst_plugin_feature_list_free(decoderFactories);
}

void lowerDecoderRanksByClass(GstRegistry* registry, bool lowerHardware)
{
    static constexpr uint16_t NewRank = GST_RANK_NONE;
    if (!registry) {
        qCCritical(GStreamerHelpersLog) << "Invalid registry!";
        return;
    }

    GList* decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_MARGINAL);

    for (GList* node = decoderFactories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) {
            continue;
        }

        if (GStreamer::isHardwareDecoderFactory(factory) != lowerHardware) {
            continue;
        }

        const gchar* name = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        if (!name) {
            continue;
        }

        qCDebug(GStreamerHelpersLog) << "Lowering" << (lowerHardware ? "hardware" : "software")
                                     << "decoder rank:" << name;
        gst_plugin_feature_set_rank(GST_PLUGIN_FEATURE(factory), NewRank);
    }

    gst_plugin_feature_list_free(decoderFactories);
}

void prioritizeByHardwareClass(GstRegistry* registry, uint16_t prioritizedRank, bool requireHardware)
{
    if (!registry) {
        qCCritical(GStreamerHelpersLog) << "Failed to get gstreamer registry.";
        return;
    }

    GList* decoderFactories = gst_element_factory_list_get_elements(
        static_cast<GstElementFactoryListType>(GST_ELEMENT_FACTORY_TYPE_DECODER | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO),
        GST_RANK_NONE);

    if (!decoderFactories) {
        qCDebug(GStreamerHelpersLog) << "No decoder factories available while prioritizing"
                                     << (requireHardware ? "hardware" : "software") << "decoders";
        return;
    }

    qCDebug(GStreamerHelpersLog) << "Prioritizing" << (requireHardware ? "hardware" : "software")
                                 << "video decoders with rank:" << prioritizedRank;
    int matchedFactories = 0;
    for (GList* node = decoderFactories; node != nullptr; node = node->next) {
        GstElementFactory* factory = GST_ELEMENT_FACTORY(node->data);
        if (!factory) {
            continue;
        }

        if (GStreamer::isHardwareDecoderFactory(factory) != requireHardware) {
            continue;
        }

        const gchar* featureName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
        if (!featureName) {
            continue;
        }

        changeFeatureRank(registry, featureName, prioritizedRank);
        ++matchedFactories;
    }

    if (matchedFactories == 0) {
        qCWarning(GStreamerHelpersLog) << "No" << (requireHardware ? "hardware" : "software")
                                       << "video decoder factories found to reprioritize.";
    }

    qCDebug(GStreamerHelpersLog) << "Lowering" << (requireHardware ? "software" : "hardware") << "decoder ranks.";
    lowerDecoderRanksByClass(registry, !requireHardware);

    gst_plugin_feature_list_free(decoderFactories);
}

#ifdef Q_OS_WIN
// A decoder whose D3D family mismatches the active QRhi backend can't be sampled zero-copy and drops to a CPU copy.
void alignD3DDecoderRanksToRhi(GstRegistry* registry, bool promoteMatchedFamily)
{
    // Prefer the resolved QRhi backend; ranks usually apply before the scene graph resolves, so cachedRhi() is often
    // null here and graphicsApi() Unknown — fall back to QSG_RHI_BACKEND (Qt's D3D11 default) in that Windows case.
    // QGCRhiCapture/qrhi.h link only when a GPU path is compiled (GuiPrivate), so the resolved-backend lookup is gated;
    // a no-GPU Windows build relies on graphicsApi()/QSG_RHI_BACKEND alone.
    bool wantD3D12 = false;
    bool resolved = false;
#if defined(QGC_HAS_ANY_GPU_PATH)
    if (QRhi* rhi = QGCRhiCapture::cachedRhi()) {
        wantD3D12 = (rhi->backend() == QRhi::D3D12);
        resolved = true;
    }
#endif
    if (!resolved) {
        switch (QQuickWindow::graphicsApi()) {
            case QSGRendererInterface::Direct3D12:
                wantD3D12 = true;
                break;
            case QSGRendererInterface::Direct3D11:
                wantD3D12 = false;
                break;
            default:
                wantD3D12 =
                    qEnvironmentVariable("QSG_RHI_BACKEND").compare(QLatin1String("d3d12"), Qt::CaseInsensitive) == 0;
                break;
        }
    }
    static constexpr const char* kD3D11Decoders[] = {"d3d11av1dec",   "d3d11h264dec", "d3d11h265dec",
                                                     "d3d11mpeg2dec", "d3d11vp8dec",  "d3d11vp9dec"};
    static constexpr const char* kD3D12Decoders[] = {"d3d12av1dec",   "d3d12h264dec", "d3d12h265dec",
                                                     "d3d12mpeg2dec", "d3d12vp8dec",  "d3d12vp9dec"};
    const char* const* matched = wantD3D12 ? kD3D12Decoders : kD3D11Decoders;
    const char* const* mismatched = wantD3D12 ? kD3D11Decoders : kD3D12Decoders;
    qCDebug(GStreamerHelpersLog) << "Aligning D3D decoder ranks to" << (wantD3D12 ? "D3D12" : "D3D11")
                                 << "RHI - demoting the mismatched decoder family";
    static_assert(std::size(kD3D11Decoders) == std::size(kD3D12Decoders));
    if (promoteMatchedFamily) {
        static constexpr uint16_t ZeroCopyRank = GST_RANK_PRIMARY + 2;
        for (size_t i = 0; i < std::size(kD3D11Decoders); ++i) {
            changeFeatureRank(registry, matched[i], ZeroCopyRank);
        }
    }
    for (size_t i = 0; i < std::size(kD3D11Decoders); ++i) {
        changeFeatureRank(registry, mismatched[i], GST_RANK_NONE);
    }
}
#endif  // Q_OS_WIN

#ifdef Q_OS_LINUX
void applyRanks(GstRegistry* registry, std::span<const char* const> features, uint16_t rank)
{
    for (const char* name : features) {
        changeFeatureRank(registry, name, rank);
    }
}

constexpr std::array<const char* const, 8> kLegacyVaapiDecoders = {"vaapiav1dec",  "vaapih264dec",  "vaapih265dec",
                                                                   "vaapijpegdec", "vaapimpeg2dec", "vaapivp8dec",
                                                                   "vaapivp9dec",  "vaapidecodebin"};
constexpr std::array<const char* const, 6> kV4l2StatelessDecoders = {
    "v4l2slh264dec", "v4l2slh265dec", "v4l2slvp8dec", "v4l2slvp9dec", "v4l2slav1dec", "v4l2slmpeg2dec"};

// Steer autoplug toward zero-copy GPU-memory decoders (DMABuf/DMA_DRM) over system-memory ones. Rank-only and
// additive: absent factories no-op, va/v4l2 self-demote at runtime without a device, software fallback untouched.
void preferZeroCopyDecoders(GstRegistry* registry)
{
    static constexpr uint16_t ZeroCopyRank = GST_RANK_PRIMARY + 2;

    // Legacy gstreamer-vaapi negotiates GstVaapiMemory the qgcqvideosink/HwBuffers paths can't import
    // zero-copy; demote it so the modern va plugin (clean DMABuf/DMA_DRM output) wins when both exist.
    applyRanks(registry, kLegacyVaapiDecoders, GST_RANK_NONE);

    // Modern va decoders (gst-va, supersedes gstreamer-vaapi) emit DMABuf/DMA_DRM, and V4L2 stateless
    // decoders expose DMA_DRM caps on 1.26+; bump both above software so the zero-copy allocator is picked.
    applyFamilyRank(registry, ForceVideoDecoderVAAPI, ZeroCopyRank);
    applyRanks(registry, kV4l2StatelessDecoders, ZeroCopyRank);
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
    GstRegistry* registry = gst_registry_get();

    if (!registry) {
        qCCritical(GStreamerHelpersLog) << "Failed to get gstreamer registry.";
        return;
    }

    static constexpr uint16_t PrioritizedRank = GST_RANK_PRIMARY + 1;

    switch (option) {
        case ForceVideoDecoderDefault:
            // HW-decoder GPU caps (GLMemory/DMABuf/VAMemory) auto-plug to system memory via gldownload/vapostproc.
#ifdef Q_OS_LINUX
            preferZeroCopyDecoders(registry);
#endif
            break;
        case ForceVideoDecoderSoftware:
            prioritizeByHardwareClass(registry, PrioritizedRank, false);
            break;
        case ForceVideoDecoderHardware:
            prioritizeByHardwareClass(registry, PrioritizedRank, true);
            break;
        case ForceVideoDecoderVAAPI:
            applyFamilyRank(registry, ForceVideoDecoderVAAPI, PrioritizedRank);
            break;
        case ForceVideoDecoderNVIDIA:
            applyFamilyRank(registry, ForceVideoDecoderNVIDIA, PrioritizedRank);
            break;
        case ForceVideoDecoderDirectX3D:
            applyFamilyRank(registry, ForceVideoDecoderDirectX3D, PrioritizedRank);
            break;
        case ForceVideoDecoderVideoToolbox:
            applyFamilyRank(registry, ForceVideoDecoderVideoToolbox, PrioritizedRank);
            break;
        case ForceVideoDecoderIntel:
            applyFamilyRank(registry, ForceVideoDecoderIntel, PrioritizedRank);
            break;
        case ForceVideoDecoderVulkan:
            // Vulkan zero-copy import is dormant: gst-vulkan's own VkDevice fails QGC's device-match guard → CPU copy.
            qCWarning(GStreamerHelpersLog) << "Forcing Vulkan video decoders: zero-copy import is dormant "
                                              "(foreign VkDevice → CPU fallback), so decode will not be zero-copy.";
            applyFamilyRank(registry, ForceVideoDecoderVulkan, PrioritizedRank);
            break;
    }

#ifdef Q_OS_WIN
    // After any option-driven rank changes, force the D3D decoder API to match the active QRhi
    // backend so a D3D12 decoder never wins over D3D11 on Qt's default Windows RHI (and vice-versa).
    const bool promoteMatchingD3D = (option == ForceVideoDecoderDefault) || (option == ForceVideoDecoderHardware) ||
                                    (option == ForceVideoDecoderDirectX3D);
    alignD3DDecoderRanksToRhi(registry, promoteMatchingD3D);
#endif
}

}  // namespace GStreamer
