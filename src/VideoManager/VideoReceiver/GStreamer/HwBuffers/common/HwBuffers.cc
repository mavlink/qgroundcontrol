#include "HwBuffers.h"

#include <atomic>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>

#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstContextBridgeBase.h"
#include "GstHwPathRegistry.h"
#include "QGCRhiCapture.h"
#endif

namespace HwBuffers {

QGC_LOGGING_CATEGORY(HwBuffersConfigLog, "Video.GStreamer.HwBuffers.Config")

#if defined(QGC_HAS_ANY_GPU_PATH)
QGC_LOGGING_CATEGORY(HwBuffersFacadeLog, "Video.GStreamer.HwBuffers.Facade")

namespace {

/// Calls @p fn for every compiled GPU path in HwVideoBufferPath order (the stats order), once per path.
template <class Fn>
void forEachCompiledPath(Fn&& fn)
{
    for (std::size_t i = 1; i < GstHw::kPathCount; ++i) {
        const auto path = static_cast<HwVideoBufferPath>(i);
        if (GstHwPathRegistry::has(path)) {
            fn(path);
        }
    }
}

}  // namespace
#endif

void onPipelineError(const GError* error, const gchar* debug) noexcept
{
    // Only a real device loss should drop every GPU cache; ordinary stream errors must not. There is no structured
    // device-lost event, so classify by the D3D/DXGI device-removed/reset text surfaced in the error/debug strings.
    const auto mentionsDeviceLoss = [](const gchar* text) {
        return text && (g_strstr_len(text, -1, "DEVICE_REMOVED") || g_strstr_len(text, -1, "DEVICE_RESET") ||
                        g_strstr_len(text, -1, "device removed") || g_strstr_len(text, -1, "device reset"));
    };
    if (mentionsDeviceLoss(error ? error->message : nullptr) || mentionsDeviceLoss(debug)) {
        resetCachedGpuResources();
    }
}

HwBufferEnvConfig readHwBufferEnvConfig() noexcept
{
    const auto truthy = [](const char* name, bool dflt) noexcept {
        const QByteArray v = qgetenv(name).trimmed().toLower();
        if (v.isEmpty()) {
            return dflt;
        }
        return v != "0" && v != "false" && v != "off" && v != "no";
    };
    HwBufferEnvConfig c;
    c.dmaBufCache = truthy("QGC_GST_DMABUF_CACHE", false);
    c.dmaBufSingleEglImage = truthy("QGC_GST_DMABUF_SINGLE_EGLIMAGE", true);
    c.dmaBufNoMmapFence = truthy("QGC_GST_DMABUF_NO_MMAP_FENCE", false);
    c.offerDmaDrmLinear = truthy("QGC_GST_OFFER_DMA_DRM_LINEAR", false);
    return c;
}

const HwBufferEnvConfig& hwBufferEnvConfig() noexcept
{
    static const HwBufferEnvConfig cfg = []() noexcept {
        const HwBufferEnvConfig c = readHwBufferEnvConfig();
        qCInfo(HwBuffersConfigLog).nospace()
            << "HwBuffer env config: QGC_GST_DMABUF_CACHE=" << c.dmaBufCache
            << " QGC_GST_DMABUF_SINGLE_EGLIMAGE=" << c.dmaBufSingleEglImage
            << " QGC_GST_DMABUF_NO_MMAP_FENCE=" << c.dmaBufNoMmapFence
            << " QGC_GST_OFFER_DMA_DRM_LINEAR=" << c.offerDmaDrmLinear;
        return c;
    }();
    return cfg;
}

namespace {

struct GpuBinChoice
{
    GpuBinMode mode;
    const char* reason;
};

const char* gpuBinName(GpuBinMode mode) noexcept
{
    switch (mode) {
        case GpuBinMode::GlUpload:
            return "glupload";
        case GpuBinMode::DirectDmaBuf:
            return "direct DMABuf";
        case GpuBinMode::Passthrough:
            break;
    }
    return "native memory passthrough";
}

GpuBinChoice chooseGpuBin() noexcept
{
    const QByteArray value = qgetenv("QGC_GST_GPU_BIN").trimmed().toLower();
#if defined(QGC_GST_BIN_HAS_GLUPLOAD) && defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    if (value == "dmabuf") {
        return {GpuBinMode::DirectDmaBuf, "from QGC_GST_GPU_BIN"};
    }
    if (value == "glupload") {
        return {GpuBinMode::GlUpload, "from QGC_GST_GPU_BIN"};
    }
    if (!value.isEmpty()) {
        qCWarning(HwBuffersConfigLog) << "Unknown QGC_GST_GPU_BIN value" << value
                                      << "(expected glupload or dmabuf); using the default glupload bin";
    }
    return {GpuBinMode::GlUpload, "default"};
#else
#if defined(QGC_GST_BIN_HAS_GLUPLOAD)
    constexpr GpuBinMode kOnlyMode = GpuBinMode::GlUpload;
#elif defined(QGC_GST_BIN_HAS_DIRECT_DMABUF)
    constexpr GpuBinMode kOnlyMode = GpuBinMode::DirectDmaBuf;
#else
    constexpr GpuBinMode kOnlyMode = GpuBinMode::Passthrough;
#endif
    return {kOnlyMode, value.isEmpty() ? "only variant compiled" : "only variant compiled; QGC_GST_GPU_BIN ignored"};
#endif
}

#ifdef QGC_UNITTEST_BUILD
constexpr int kNoTestGpuBinMode = -1;
constinit std::atomic<int> s_testGpuBinMode{kNoTestGpuBinMode};
#endif

}  // namespace

GpuBinMode readGpuBinMode() noexcept
{
    return chooseGpuBin().mode;
}

GpuBinMode gpuBinMode() noexcept
{
#ifdef QGC_UNITTEST_BUILD
    if (const int testMode = s_testGpuBinMode.load(std::memory_order_acquire); testMode != kNoTestGpuBinMode) {
        return static_cast<GpuBinMode>(testMode);
    }
#endif
    static const GpuBinMode mode = []() noexcept {
        const GpuBinChoice choice = chooseGpuBin();
        qCInfo(HwBuffersConfigLog).nospace() << "GPU bin: " << gpuBinName(choice.mode) << " (" << choice.reason << ")";
        return choice.mode;
    }();
    return mode;
}

#ifdef QGC_UNITTEST_BUILD
ScopedGpuBinModeForTest::ScopedGpuBinModeForTest(GpuBinMode mode)
    : _previous(s_testGpuBinMode.exchange(static_cast<int>(mode), std::memory_order_acq_rel))
{}

ScopedGpuBinModeForTest::~ScopedGpuBinModeForTest()
{
    s_testGpuBinMode.store(_previous, std::memory_order_release);
}
#endif

GstBusSyncReply onBusSyncMessage(GstMessage* msg) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    // First GST_BUS_DROP wins; bridges answer disjoint context types, so none shadows another.
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->bridge && (path->bridge().handleSyncMessage(msg) == GST_BUS_DROP)) {
            return GST_BUS_DROP;
        }
    }
#else
    Q_UNUSED(msg);
#endif
    return GST_BUS_PASS;
}

void onPipelineRestart() noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->bridge) {
            path->bridge().rearm();
        }
    }
#endif
}

void resetCachedGpuResources() noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    const auto paths = GstHwPathRegistry::paths();
    for (const HwPathDescriptor* path : paths) {
        if (path->bridge) {
            path->bridge().reset();
        }
    }
    for (const HwPathDescriptor* path : paths) {
        if (path->resetCaches) {
            path->resetCaches();
        }
    }
#endif
}

void connectMainWindow(QQuickWindow* window) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    QGCRhiCapture::connectWindow(window);
#else
    Q_UNUSED(window);
#endif
}

HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept
{
    HwVideoBufferContext ctx;
#if defined(QGC_HAS_ANY_GPU_PATH)
    ctx.gpuEnabled = gpuEnabled;
    if (!gpuEnabled) {
        return ctx;
    }
    QString available;
    forEachCompiledPath(
        [&available](HwVideoBufferPath path) { available += QLatin1Char(' ') + QLatin1String(GstHw::toString(path)); });
    qCDebug(HwBuffersFacadeLog).noquote()
        << "zero-copy paths compiled:" << available.trimmed() << "— actual path chosen at caps negotiation";
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->prepareContext) {
            path->prepareContext(ctx);
        }
    }
#else
    Q_UNUSED(gpuEnabled);
#endif
    return ctx;
}

bool answerSinkBinContextQuery(GstQuery* query) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->bridge && path->bridge().answerContextQuery(query)) {
            return true;
        }
    }
#else
    Q_UNUSED(query);
#endif
    return false;
}

bool proposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size,
                            guint minBuffers) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->proposePool && path->proposePool(query, caps, vinfo, size, minBuffers)) {
            return true;
        }
    }
#else
    Q_UNUSED(query);
    Q_UNUSED(caps);
    Q_UNUSED(vinfo);
    Q_UNUSED(size);
    Q_UNUSED(minBuffers);
#endif
    return false;
}

QString takeExtraPathStats() noexcept
{
    QString out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    using GstHwPathTelemetry::Counter;

    // "<path>-<label>:<count>", or "<count>/<count>" for a pair, when any count is nonzero.
    struct CounterField
    {
        const char* label;
        Counter first;
        std::optional<Counter> second = std::nullopt;
    };

    static constexpr CounterField kCounterFields[] = {
        {"failures", Counter::MapFailures},
        {"reuse", Counter::TextureReuse},
        {"wait[gpu/cpu]", Counter::GpuWaits, Counter::CpuWaits},
        {"fence-timeouts", Counter::ProducerFenceTimeouts},
        {"image-cache[hit/miss]", Counter::ImageCacheHits, Counter::ImageCacheMisses},
    };
    static constexpr std::pair<GstHwPathTelemetry::HwFallbackReason, const char*> kReasons[] = {
        {GstHwPathTelemetry::HwFallbackReason::NoExt, "no-ext"},
        {GstHwPathTelemetry::HwFallbackReason::ModifierRejected, "modifier"},
        {GstHwPathTelemetry::HwFallbackReason::EglBadMatch, "egl-bad-match"},
        {GstHwPathTelemetry::HwFallbackReason::ValidateFailed, "validate"},
        {GstHwPathTelemetry::HwFallbackReason::UnknownMemType, "unknown-mem"},
        {GstHwPathTelemetry::HwFallbackReason::MapFailed, "map-failed"},
        {GstHwPathTelemetry::HwFallbackReason::VulkanNoSync, "vulkan-no-sync"},
        {GstHwPathTelemetry::HwFallbackReason::ImportUnsupported, "import-unsupported"},
    };
    forEachCompiledPath([&out](HwVideoBufferPath path) {
        const QLatin1String pathLabel(GstHw::toString(path));
        for (const CounterField& field : kCounterFields) {
            const quint64 first = GstHwPathTelemetry::take(path, field.first);
            bool nonzero = (first > 0);
            QString counts = QString::number(first);
            if (field.second) {
                const quint64 second = GstHwPathTelemetry::take(path, *field.second);
                nonzero = nonzero || (second > 0);
                counts += u'/' + QString::number(second);
            }
            if (nonzero) {
                out += QStringLiteral(" %1-%2:%3").arg(pathLabel, QLatin1String(field.label), counts);
            }
        }
        const quint64 ewmaUs = GstHwPathTelemetry::peekMapDurationUsEwma(path);
        if (ewmaUs > 0) {
            out += QStringLiteral(" %1-map-us:%2").arg(pathLabel).arg(ewmaUs);
        }
        QString reasonBreakdown;
        for (const auto& [reason, label] : kReasons) {
            const quint64 n = GstHwPathTelemetry::takeFallbackReason(path, reason);
            if (n > 0) {
                reasonBreakdown += QStringLiteral("%1=%2,").arg(QLatin1String(label)).arg(n);
            }
        }
        if (!reasonBreakdown.isEmpty()) {
            reasonBreakdown.chop(1);
            out += QStringLiteral(" %1-fallback[%2]").arg(pathLabel).arg(reasonBreakdown);
        }
    });
    // None-path accumulates UnknownMemType (no allocator matched any compiled path).
    const quint64 unknownMem = GstHwPathTelemetry::takeFallbackReason(
        HwVideoBufferPath::None, GstHwPathTelemetry::HwFallbackReason::UnknownMemType);
    if (unknownMem > 0) {
        out += QStringLiteral(" None-fallback[unknown-mem=%1]").arg(unknownMem);
    }
#endif
    return out;
}

}  // namespace HwBuffers
