#include "HwBuffers.h"

#include <QtCore/QByteArray>
#include <utility>

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

// Every compiled GPU path in selection order: a frame takes the first one whose matches() accepts its memory. CUDA
// precedes DMABuf because it exports CUDA memory to the DMABuf importer.
constexpr const HwPathDescriptor* kCompiledPaths[] = {
#if defined(QGC_HAS_GST_CUDA_GPU_PATH)
    &GstHwPaths::kCuda,
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    &GstHwPaths::kDmaBuf,
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    &GstHwPaths::kGlMemory,
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    &GstHwPaths::kD3D11,
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    &GstHwPaths::kD3D12,
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    &GstHwPaths::kVulkan,
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    &GstHwPaths::kIOSurface,
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    &GstHwPaths::kAHardwareBuffer,
#endif
};

/// Calls @p fn for every registered GPU path in HwVideoBufferPath order (the stats order), once per path.
template <class Fn>
void forEachRegisteredPath(Fn&& fn)
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

void initializeOnce() noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    [[maybe_unused]] static const bool s_registered = [] {
        for (const HwPathDescriptor* path : kCompiledPaths) {
            GstHwPathRegistry::add(*path);
        }
        return true;
    }();
#endif
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

GstBusSyncReply onBusSyncMessage(GstBus* /*bus*/, GstMessage* msg, gpointer /*userData*/) noexcept
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
    forEachRegisteredPath(
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

PathStats formatPathStats(bool reset) noexcept
{
    PathStats out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    forEachRegisteredPath([reset, &out](HwVideoBufferPath path) {
        const QLatin1String label(GstHw::toString(path));
        const quint64 delivered =
            reset ? GstHwPathTelemetry::takeDeliveredCount(path) : GstHwPathTelemetry::peekDeliveredCount(path);
        const quint64 failures =
            reset ? GstHwPathTelemetry::takeMapFailureCount(path) : GstHwPathTelemetry::peekMapFailureCount(path);
        // Teardown (reset) uses expanded label so operators can copy the failures field into bug reports.
        if (reset) {
            out.line += QStringLiteral(" %1:%2 %1-failures:%3").arg(label).arg(delivered).arg(failures);
        } else {
            out.line += QStringLiteral(" %1:%2/%3").arg(label).arg(delivered).arg(failures);
        }
        out.totalDelivered += delivered;
    });
#else
    Q_UNUSED(reset);
#endif
    return out;
}

PathStats formatSinkStats(const GstStructure* stats) noexcept
{
    PathStats out;
    if (!stats) {
        return out;
    }
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        guint64 frames = 0;
        if (gst_structure_get_uint64(stats, GstHwPathTelemetry::sinkFramesField(path), &frames) && (frames > 0)) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(GstHw::toString(path))).arg(frames);
            if (path != HwVideoBufferPath::None) {
                out.totalDelivered += frames;
            }
        }
    }
    for (const char* const field : {"gpu-fallbacks", "stream-demotions"}) {
        guint64 count = 0;
        if (gst_structure_get_uint64(stats, field, &count) && (count > 0)) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(field)).arg(count);
        }
    }
    return out;
}

QString takeExtraPathStats() noexcept
{
    QString out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    if (GstHwPathRegistry::has(HwVideoBufferPath::GlMemory)) {
        const quint64 glReuse = GstHwPathTelemetry::takeTextureReuseHits(HwVideoBufferPath::GlMemory);
        quint64 glGpuWaits = 0;
        const quint64 glCpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(HwVideoBufferPath::GlMemory, glGpuWaits);
        out += QStringLiteral(" GL-reuse:%1 GL-wait[gpu/cpu]:%2/%3").arg(glReuse).arg(glGpuWaits).arg(glCpuWaits);
    }
    if (GstHwPathRegistry::has(HwVideoBufferPath::Vulkan)) {
        quint64 vkGpuWaits = 0;
        const quint64 vkCpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(HwVideoBufferPath::Vulkan, vkGpuWaits);
        out += QStringLiteral(" Vulkan-wait[gpu/cpu]:%1/%2").arg(vkGpuWaits).arg(vkCpuWaits);
    }
    if (GstHwPathRegistry::has(HwVideoBufferPath::DmaBuf)) {
        out += QStringLiteral(" DMABuf-fence-timeouts:%1 DMABuf-cpu-barriers:%2 DMABuf-explicit-fence-waits:%3")
                   .arg(GstHwPathTelemetry::takeProducerFenceTimeouts(HwVideoBufferPath::DmaBuf))
                   .arg(GstHwPathTelemetry::takeCpuBarrierWaits(HwVideoBufferPath::DmaBuf))
                   .arg(GstHwPathTelemetry::takeExplicitFenceWaits(HwVideoBufferPath::DmaBuf));
    }
    static constexpr std::pair<GstHwPathTelemetry::HwFallbackReason, const char*> kReasons[] = {
        {GstHwPathTelemetry::HwFallbackReason::NoExt, "no-ext"},
        {GstHwPathTelemetry::HwFallbackReason::ModifierRejected, "modifier"},
        {GstHwPathTelemetry::HwFallbackReason::EglBadMatch, "egl-bad-match"},
        {GstHwPathTelemetry::HwFallbackReason::FenceTimeout, "fence-timeout"},
        {GstHwPathTelemetry::HwFallbackReason::ValidateFailed, "validate"},
        {GstHwPathTelemetry::HwFallbackReason::UnknownMemType, "unknown-mem"},
        {GstHwPathTelemetry::HwFallbackReason::NullSample, "null-sample"},
        {GstHwPathTelemetry::HwFallbackReason::MapFailed, "map-failed"},
        {GstHwPathTelemetry::HwFallbackReason::VulkanNoSync, "vulkan-no-sync"},
        {GstHwPathTelemetry::HwFallbackReason::ImportUnsupported, "import-unsupported"},
    };
    forEachRegisteredPath([&out](HwVideoBufferPath path) {
        const QLatin1String pathLabel(GstHw::toString(path));
        const quint64 ewmaUs = GstHwPathTelemetry::peekMapDurationUsEwma(path);
        if (ewmaUs > 0) {
            out += QStringLiteral(" %1-map-us:%2").arg(pathLabel).arg(ewmaUs);
        }
        const quint64 demotions = GstHwPathTelemetry::takeStreamDemotions(path);
        if (demotions > 0) {
            out += QStringLiteral(" %1-demotions:%2").arg(pathLabel).arg(demotions);
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
    {
        const quint64 unknownMem =
            GstHwPathTelemetry::takeFallbackReason(HwVideoBufferPath::None,
                                                   GstHwPathTelemetry::HwFallbackReason::UnknownMemType);
        if (unknownMem > 0) {
            out += QStringLiteral(" None-fallback[unknown-mem=%1]").arg(unknownMem);
        }
        const quint64 cpuDemotions = GstHwPathTelemetry::takeStreamDemotions(HwVideoBufferPath::None);
        if (cpuDemotions > 0) {
            out += QStringLiteral(" None-demotions:%1").arg(cpuDemotions);
        }
    }
#endif
    return out;
}

}  // namespace HwBuffers
