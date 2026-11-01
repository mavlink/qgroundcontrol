#include "GStreamerDecoderRanks.h"

#include <algorithm>

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include "GstScoped.h"
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

QList<DecoderInfo> installedDecoders()
{
    QList<DecoderInfo> decoders;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&decoders](GstElementFactory* factory) {
        GstPluginFeature* const feature = GST_PLUGIN_FEATURE(factory);
        const gchar* const name = gst_plugin_feature_get_name(feature);
        decoders.append({.name = QByteArray(name),
                         .rank = gst_plugin_feature_get_rank(feature),
                         .hardware = isHardwareDecoderFactory(factory),
                         .family = decoderFamilyForFactory(name)});
    });
    return decoders;
}

}  // namespace

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
        default:
            break;
    }
    traits.rhiD3DApi = d3d12 ? D3DApi::D3D12 : D3DApi::D3D11;
#endif
    return traits;
}

DecoderRankApplier& DecoderRankApplier::instance()
{
    static DecoderRankApplier applier;
    return applier;
}

void DecoderRankApplier::apply(VideoDecoderOptions option)
{
    GstRegistry* const registry = _registry();
    if (!registry) {
        return;
    }
    _restore(registry);

    const RankTraits traits = currentRankTraits();
    const RankPlan plan = planRanks(option, installedDecoders(), traits);
    qCDebug(GStreamerDecoderRanksLog) << "Applying decoder option" << static_cast<int>(option) << "with"
                                      << plan.changes.size() << "rank changes";
    for (const QString& warning : plan.warnings) {
        qCWarning(GStreamerDecoderRanksLog).noquote() << warning;
    }
    for (const RankChange& change : plan.changes) {
        (void) setRank(registry, change.feature.constData(), change.rank);
    }
    _applied = option;
}

void DecoderRankApplier::restore()
{
    if (GstRegistry* const registry = _registry()) {
        _restore(registry);
    }
}

bool DecoderRankApplier::setRank(GstRegistry* registry, const char* featureName, uint16_t rank)
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
    if (!gst_is_initialized()) {
        return nullptr;
    }
    GstRegistry* const registry = gst_registry_get();
    if (!registry) {
        qCCritical(GStreamerDecoderRanksLog) << "Failed to get gstreamer registry.";
    }
    return registry;
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

ScopedDecoderOption::ScopedDecoderOption(VideoDecoderOptions option)
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

void setCodecPriorities(int rawOption)
{
    // Validate the raw setting here so the planner's enum switch stays default-free (-Wswitch).
    if ((rawOption < ForceVideoDecoderDefault) || (rawOption > ForceVideoDecoderHardware)) {
        qCWarning(GStreamerDecoderRanksLog) << "Ignoring invalid decode option:" << rawOption;
        return;
    }
    setCodecPriorities(static_cast<VideoDecoderOptions>(rawOption));
}

void setCodecPriorities(VideoDecoderOptions option)
{
    DecoderRankApplier::instance().apply(option);
}

QList<VideoDecoderOptions> availableDecoderFamilies()
{
    QList<VideoDecoderOptions> families;
    forEachVideoDecoderFactory(GST_RANK_NONE, [&families](GstElementFactory* factory) {
        const VideoDecoderOptions family =
            decoderFamilyForFactory(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
#ifndef QGC_HAS_GST_VULKAN_GPU_PATH
        // Without the opt-in Vulkan GPU path, forcing Vulkan decoders only adds a CPU copy of every frame.
        if (family == ForceVideoDecoderVulkan) {
            return;
        }
#endif
        if ((family != ForceVideoDecoderDefault) && !families.contains(family)) {
            families.append(family);
        }
    });
    return families;
}

void logDecoderRanks()
{
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
