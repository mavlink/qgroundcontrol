#include "GStreamerDecoderRanks.h"

#include <algorithm>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GstScoped.h"
#include "HwBuffers/HwBuffers.h"
#include "QGCLoggingCategory.h"
#ifdef Q_OS_WIN
#include "gstqgc/GstQgcRenderTarget.h"
#endif

QGC_LOGGING_CATEGORY(GStreamerDecoderRanksLog, "Video.GStreamer.DecoderRanks")

namespace GStreamer {

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
    VideoDecoderOption family;  // VideoDecoderOption::Default: not a user-selectable family
    bool isHardware;
};

// Single name-based decoder classification for the hardware/software split, the forceVideoDecoder
// rank overrides and availableDecoderFamilies(). Prefix (not substring) matching, so e.g. avdec_wnv1
// isn't taken for an NVIDIA decoder. The first match wins: specific prefixes precede general ones.
constexpr DecoderFamilyPrefix kDecoderFamilyPrefixes[] = {
    // Android MediaCodec: Google/Codec2 software wrappers; every other amcviddec is a hardware codec.
    {"amcviddec-omxgoogle", VideoDecoderOption::Default, false},
    {"amcviddec-c2android", VideoDecoderOption::Default, false},
    {"amcviddec-", VideoDecoderOption::Default, true},
    // Legacy gstreamer-vaapi is demoted on Linux and never bumped by VideoDecoderOption::VAAPI (modern va* only).
    {"vaapi", VideoDecoderOption::Default, true},
    {"va", VideoDecoderOption::VAAPI, true},
    {"nv", VideoDecoderOption::NVIDIA, true},
    {"d3d11", VideoDecoderOption::DirectX3D, true},
    {"d3d12", VideoDecoderOption::DirectX3D, true},
    {"dxva", VideoDecoderOption::DirectX3D, true},
    // vtdec and vtdec_hw, and from GStreamer 1.30 one decoder per codec (vth264dec, vth264dec_hw, ...).
    {"vt", VideoDecoderOption::VideoToolbox, true},
    {"qsv", VideoDecoderOption::Intel, true},
    {"msdk", VideoDecoderOption::Intel, true},
    {"vulkan", VideoDecoderOption::Vulkan, true},
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

VideoDecoderOption decoderFamilyForFactory(const char* factoryName)
{
    if (!factoryName) {
        return VideoDecoderOption::Default;
    }

    const DecoderFamilyPrefix* entry = findDecoderFamilyPrefix(factoryName);
    return entry ? entry->family : VideoDecoderOption::Default;
}

bool isHardwareDecoderFactory(GstElementFactory* factory)
{
    if (!factory) {
        return false;
    }

    const gchar* factoryName = gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory));
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

#ifdef Q_OS_WIN
/// The adapter the Direct3D decoder @p factory decodes on, 0 when it doesn't tell. GStreamer registers one decoder
/// per adapter and codec and keeps the adapter in the element's class, which only an element reads ("adapter-luid"):
/// one is made for it, once per factory. It opens no device before it leaves the NULL state.
qint64 decoderAdapterLuid(GstElementFactory* factory)
{
    static QHash<QByteArray, qint64> s_luids;
    const QByteArray name(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
    if (const auto known = s_luids.constFind(name); known != s_luids.constEnd()) {
        return known.value();
    }
    gint64 luid = 0;
    if (const GstElementPtr decoder = makeElement(name.constData());
        decoder && g_object_class_find_property(G_OBJECT_GET_CLASS(decoder.get()), "adapter-luid")) {
        g_object_get(decoder.get(), "adapter-luid", &luid, nullptr);
    }
    s_luids.insert(name, luid);
    return luid;
}
#endif

/// With the applier's mutex held.
QList<DecoderInfo> installedDecoders()
{
    QList<DecoderInfo> decoders;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&decoders](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        const gchar* const name = gst_plugin_feature_get_name(feature);
        DecoderInfo decoder{.name = QByteArray(name),
                            .rank = gst_plugin_feature_get_rank(feature),
                            .hardware = isHardwareDecoderFactory(factory),
                            .family = decoderFamilyForFactory(name)};
#ifdef Q_OS_WIN
        if (decoder.family == VideoDecoderOption::DirectX3D) {
            decoder.adapterLuid = decoderAdapterLuid(factory);
        }
#endif
        decoders.append(decoder);
    });
    return decoders;
}

/// The RankTraits of the running platform: the zero-copy preferences on Linux, the RHI's D3D API on Windows.
RankTraits currentRankTraits()
{
    RankTraits traits;
#if defined(Q_OS_LINUX)
    traits.preferZeroCopyDecoders = true;
#elif defined(Q_OS_WIN)
    // Ranks usually apply before the scene graph resolves its backend, while the render target is still Unknown: fall
    // back to QSG_RHI_BACKEND (Qt's D3D11 default) then.
    bool d3d12 = false;
    switch (GstQgc::currentRenderTarget().backend) {
        case GstQgc::RenderBackend::Direct3D12:
            d3d12 = true;
            break;
        case GstQgc::RenderBackend::Unknown:
            d3d12 = qEnvironmentVariable("QSG_RHI_BACKEND").compare(QLatin1String("d3d12"), Qt::CaseInsensitive) == 0;
            break;
        case GstQgc::RenderBackend::Direct3D11:
        // No D3D API is sampled zero-copy under these: the D3D11 decoders stay the ones preferred.
        case GstQgc::RenderBackend::Null:
        case GstQgc::RenderBackend::OpenGL:
        case GstQgc::RenderBackend::Metal:
        case GstQgc::RenderBackend::Vulkan:
            break;
    }
    traits.rhiD3DApi = d3d12 ? D3DApi::D3D12 : D3DApi::D3D11;
    // Both known once the scene graph has published Qt's device, which has the ranks applied again (reapply()).
    traits.d3d11OpensD3D12 = !d3d12 && HwBuffers::d3d11OpensD3D12();
    traits.rhiAdapterLuid = HwBuffers::graphicsAdapterLuid();
#endif
    traits.softwareRendering = HwBuffers::softwareRenderingRequested();
    return traits;
}

}  // namespace

DecoderRankApplier& DecoderRankApplier::instance()
{
    static DecoderRankApplier applier;
    return applier;
}

void DecoderRankApplier::apply(VideoDecoderOption option)
{
    GstRegistry* const registry = _registry();
    if (!registry) {
        return;
    }
    const QMutexLocker locker(&_mutex);
    _restore(registry);

    const RankTraits traits = currentRankTraits();
    const RankPlan plan = planRanks(option, installedDecoders(), traits);
    qCDebug(GStreamerDecoderRanksLog) << "Applying decoder option" << static_cast<int>(option) << "with"
                                      << plan.changes.size() << "rank changes";
    for (const QString& warning : plan.warnings) {
        qCWarning(GStreamerDecoderRanksLog).noquote() << warning;
    }
    for (const RankChange& change : plan.changes) {
        (void) _setRank(registry, change.feature.constData(), change.rank);
    }
    _applied = option;
    _appliedTraits = traits;
}

void DecoderRankApplier::reapply()
{
    if (_applied && (currentRankTraits() != _appliedTraits)) {
        apply(*_applied);
    }
}

void DecoderRankApplier::restore()
{
    if (GstRegistry* const registry = _registry()) {
        const QMutexLocker locker(&_mutex);
        _restore(registry);
    }
}

bool DecoderRankApplier::setRank(GstRegistry* registry, const char* featureName, uint16_t rank)
{
    const QMutexLocker locker(&_mutex);
    return _setRank(registry, featureName, rank);
}

bool DecoderRankApplier::demoteFailedDecoder(const char* featureName, const GstCaps* caps)
{
    GstRegistry* const registry = _registry();
    if (!registry || !featureName || !caps || rankedByUser(featureName)) {
        return false;
    }
    // decodebin3's candidates for the stream: the autoplugged decoders whose sink template covers its caps.
    GList* const decoders = gst_element_factory_list_get_elements(GST_ELEMENT_FACTORY_TYPE_DECODER, GST_RANK_MARGINAL);
    GList* const candidates = gst_element_factory_list_filter(decoders, caps, GST_PAD_SINK, TRUE);
    gst_plugin_feature_list_free(decoders);
    bool otherDecoder = false;
    for (GList* node = candidates; node != nullptr; node = node->next) {
        otherDecoder =
            otherDecoder || (g_strcmp0(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(node->data)), featureName) != 0);
    }
    gst_plugin_feature_list_free(candidates);
    if (!otherDecoder) {
        return false;
    }
    const QMutexLocker locker(&_mutex);
    return _setRank(registry, featureName, GST_RANK_NONE);
}

bool DecoderRankApplier::_setRank(GstRegistry* registry, const char* featureName, uint16_t rank)
{
    if (!registry || !featureName) {
        return false;
    }
    const GstFeaturePtr feature = adoptFeature(gst_registry_lookup_feature(registry, featureName));
    if (!feature) {
        return false;
    }

    if (rankedByUser(featureName)) {
        qCDebug(GStreamerDecoderRanksLog) << "  Keeping the GST_PLUGIN_FEATURE_RANK rank of" << featureName;
        return true;
    }
    const QByteArray key(featureName);
    if (!_originalRanks.contains(key)) {
        _originalRanks.insert(key, gst_plugin_feature_get_rank(feature.get()));
    }
    qCDebug(GStreamerDecoderRanksLog) << "  Changing feature (" << featureName << ") to use rank:" << rank;
    gst_plugin_feature_set_rank(feature.get(), rank);
    return true;
}

GstRegistry* DecoderRankApplier::_registry()
{
    return gst_is_initialized() ? gst_registry_get() : nullptr;
}

void DecoderRankApplier::_restore(GstRegistry* registry)
{
    for (auto it = _originalRanks.cbegin(); it != _originalRanks.cend(); ++it) {
        const GstFeaturePtr feature = adoptFeature(gst_registry_lookup_feature(registry, it.key().constData()));
        if (feature) {
            gst_plugin_feature_set_rank(feature.get(), it.value());
        }
    }
    _applied.reset();
}

ScopedDecoderOption::ScopedDecoderOption(VideoDecoderOption option)
    : _previous(DecoderRankApplier::instance().applied())
{
    DecoderRankApplier::instance().apply(option);
}

ScopedDecoderOption::~ScopedDecoderOption()
{
    if (_previous) {
        DecoderRankApplier::instance().apply(*_previous);
    } else {
        DecoderRankApplier::instance().restore();
    }
}

bool changeFeatureRank(GstRegistry* registry, const char* featureName, uint16_t rank)
{
    return DecoderRankApplier::instance().setRank(registry, featureName, rank);
}

void setCodecPriorities(VideoDecoderOption option)
{
    DecoderRankApplier::instance().apply(option);
}

bool demoteFailedDecoder(const char* factoryName, const GstCaps* caps)
{
    return DecoderRankApplier::instance().demoteFailedDecoder(factoryName, caps);
}

QList<VideoDecoderOption> availableDecoderFamilies()
{
    QList<VideoDecoderOption> families;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&families](GstElementFactory* factory) {
        const VideoDecoderOption family =
            decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
#ifndef QGC_HAS_GST_VULKAN_GPU_PATH
        // Only a build with the opt-in Vulkan GPU path has a use for them: no sink takes their VulkanImage memory.
        if (family == VideoDecoderOption::Vulkan) {
            return;
        }
#endif
        if ((family != VideoDecoderOption::Default) && !families.contains(family)) {
            families.append(family);
        }
    });
    return families;
}

void logDecoderRanks()
{
    if (!GStreamerDecoderRanksLog().isDebugEnabled()) {
        return;
    }

    struct Decoder
    {
        guint rank;
        QByteArray name;
        QString description;
    };

    QList<Decoder> decoders;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&decoders](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        decoders.append({gst_plugin_feature_get_rank(feature), QByteArray(gst_plugin_feature_get_name(feature)),
                         describeDecoderFactory(factory)});
    });

    if (decoders.isEmpty()) {
        qCDebug(GStreamerDecoderRanksLog) << "No video decoder factories found";
        return;
    }

    std::ranges::sort(decoders, [](const Decoder& lhs, const Decoder& rhs) {
        return (lhs.rank != rhs.rank) ? (lhs.rank > rhs.rank) : (lhs.name < rhs.name);
    });

    qCDebug(GStreamerDecoderRanksLog) << "Video decoder ranks:";
    for (const Decoder& decoder : std::as_const(decoders)) {
        qCDebug(GStreamerDecoderRanksLog).noquote() << "  " + decoder.description;
    }
}

}  // namespace GStreamer
