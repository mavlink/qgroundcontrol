#include "HwBuffers.h"

#include <array>
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

void dispatchBusMessage(GstMessage* msg) noexcept
{
    if (!msg || GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR)
        return;

    // Only a real device loss should drop every GPU cache; ordinary stream errors must not. There is no structured
    // device-lost event, so classify by the D3D/DXGI device-removed/reset text surfaced in the error/debug strings.
    GError* err = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(msg, &err, &debug);

    const auto mentionsDeviceLoss = [](const gchar* text) {
        return text && (g_strstr_len(text, -1, "DEVICE_REMOVED") || g_strstr_len(text, -1, "DEVICE_RESET") ||
                        g_strstr_len(text, -1, "device removed") || g_strstr_len(text, -1, "device reset"));
    };
    const bool deviceLost = mentionsDeviceLoss(err ? err->message : nullptr) || mentionsDeviceLoss(debug);

    if (err)
        g_error_free(err);
    g_free(debug);

    if (deviceLost)
        resetCachedGpuResources();
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
    // Latch-only: shared bridge objects and import caches stay, because another stream may be using them. Full resets
    // are reserved for scene-graph teardown, window swaps and device loss (resetCachedGpuResources).
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

const char* sinkFramesField(HwVideoBufferPath path) noexcept
{
    static const std::array<QByteArray, GstHw::kPathCount> s_fields = [] {
        std::array<QByteArray, GstHw::kPathCount> fields;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            fields[i] = QByteArray("frames-") + QByteArray(GstHw::toString(HwVideoBufferPath(i))).toLower();
        }
        return fields;
    }();
    const auto index = static_cast<std::size_t>(path);
    return (index < s_fields.size()) ? s_fields[index].constData() : nullptr;
}

PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since) noexcept
{
    PathStats out;
    if (!stats) {
        return out;
    }
    const auto countOf = [stats, since](const char* field) -> quint64 {
        guint64 now = 0;
        guint64 before = 0;
        (void) gst_structure_get_uint64(stats, field, &now);
        if (since) {
            (void) gst_structure_get_uint64(since, field, &before);
        }
        return (now > before) ? (now - before) : 0;
    };
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        if (const quint64 frames = countOf(sinkFramesField(path)); frames > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(GstHw::toString(path))).arg(frames);
            if (path != HwVideoBufferPath::None) {
                out.totalDelivered += frames;
            }
        }
    }
    for (const char* const field : {"gpu-fallbacks", "stream-demotions"}) {
        if (const quint64 count = countOf(field); count > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(field)).arg(count);
        }
    }
    return out;
}

QString takeExtraPathStats() noexcept
{
    QString out;
#if defined(QGC_HAS_ANY_GPU_PATH)
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
        const quint64 mapFailures = GstHwPathTelemetry::takeMapFailureCount(path);
        if (mapFailures > 0) {
            out += QStringLiteral(" %1-failures:%2").arg(pathLabel).arg(mapFailures);
        }
        const quint64 ewmaUs = GstHwPathTelemetry::peekMapDurationUsEwma(path);
        if (ewmaUs > 0) {
            out += QStringLiteral(" %1-map-us:%2").arg(pathLabel).arg(ewmaUs);
        }
        const quint64 reuse = GstHwPathTelemetry::takeTextureReuseHits(path);
        if (reuse > 0) {
            out += QStringLiteral(" %1-reuse:%2").arg(pathLabel).arg(reuse);
        }
        quint64 gpuWaits = 0;
        const quint64 cpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(path, gpuWaits);
        if ((gpuWaits > 0) || (cpuWaits > 0)) {
            out += QStringLiteral(" %1-wait[gpu/cpu]:%2/%3").arg(pathLabel).arg(gpuWaits).arg(cpuWaits);
        }
        const quint64 fenceTimeouts = GstHwPathTelemetry::takeProducerFenceTimeouts(path);
        if (fenceTimeouts > 0) {
            out += QStringLiteral(" %1-fence-timeouts:%2").arg(pathLabel).arg(fenceTimeouts);
        }
        const quint64 cacheHits = GstHwPathTelemetry::takeImageCacheHits(path);
        const quint64 cacheMisses = GstHwPathTelemetry::takeImageCacheMisses(path);
        if ((cacheHits > 0) || (cacheMisses > 0)) {
            out += QStringLiteral(" %1-image-cache[hit/miss]:%2/%3").arg(pathLabel).arg(cacheHits).arg(cacheMisses);
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
