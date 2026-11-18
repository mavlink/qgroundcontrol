#include "HwBuffers.h"

#include <algorithm>
#include <atomic>
#include <optional>
#include <utility>
#include <vector>

#include <QtCore/QCoreApplication>
#include <QtQuick/QQuickWindow>

#include "QGCLoggingCategory.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstContextBridgeBase.h"
#include "GstHwPathRegistry.h"
#include "QGCRhiCapture.h"
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include <rhi/qrhi.h>

#include "GstDmaBufVideoBuffer.h"
#include "GstDmaBufVulkanImport.h"
#endif
#if defined(QGC_UNITTEST_BUILD) && (defined(QGC_HAS_GST_DMABUF_GPU_PATH) || \
                                    (defined(QGC_HAS_GST_D3D11_GPU_PATH) && defined(QGC_HAS_GST_D3D12_GPU_PATH)))
#include "gstqgc/GstQgcCaps.h"
#endif

namespace HwBuffers {

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

/// Resets every context bridge, then the paths' caches. Returns the pipelines the bridges had given their device.
std::vector<GStreamer::GstElementPtr> resetBridgesAndCaches()
{
    std::vector<GStreamer::GstElementPtr> pipelines;
    const auto paths = GstHwPathRegistry::paths();
    for (const HwPathDescriptor* path : paths) {
        if (!path->bridge) {
            continue;
        }
        for (GStreamer::GstElementPtr& pipeline : path->bridge().reset()) {
            const bool known = std::ranges::any_of(pipelines, [&pipeline](const GStreamer::GstElementPtr& other) {
                return other.get() == pipeline.get();
            });
            if (!known) {
                pipelines.push_back(std::move(pipeline));
            }
        }
    }
    for (const HwPathDescriptor* path : paths) {
        if (path->resetCaches) {
            path->resetCaches();
        }
    }
    return pipelines;
}

}  // namespace
#endif

namespace {

std::atomic<bool> s_softwareRendering{false};

}  // namespace

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

void resetCachedGpuResources()
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    (void) resetBridgesAndCaches();
#endif
}

void onGraphicsDeviceRemoved()
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    // Every pipeline a bridge served until it dropped the device, a streaming thread's request meanwhile included.
    for (const GStreamer::GstElementPtr& pipeline : resetBridgesAndCaches()) {
        (void) gst_element_post_message(
            pipeline.get(),
            gst_message_new_element(GST_OBJECT(pipeline.get()), gst_structure_new_empty(kGraphicsDeviceLostMessage)));
    }
#endif
}

bool graphicsDeviceLost() noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->bridge && path->bridge().deviceLost()) {
            return true;
        }
    }
#endif
    return false;
}

void configureQtGraphics(bool softwareRendering)
{
    s_softwareRendering.store(softwareRendering, std::memory_order_release);
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID) && \
    (defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH))
    // GStreamer's zero-copy imports need Qt on EGL, but Qt's xcb integration picks GLX first (the AppImage runs xcb
    // even in Wayland sessions). Qt falls back to GLX when EGL fails to initialize; the user's value wins.
    if (!qEnvironmentVariableIsSet("QT_XCB_GL_INTEGRATION")) {
        (void) qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");
    }
#endif
    // The GL and DMABuf paths both need QOpenGLContext::globalShareContext(), which this attribute enables.
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
#endif
    // Read only when Qt creates a Vulkan device, as it does with QSG_RHI_BACKEND=vulkan.
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
    GstDmaBufVulkan::requestDeviceExtensions();
#endif
}

bool softwareRenderingRequested() noexcept
{
    return s_softwareRendering.load(std::memory_order_acquire);
}

void connectMainWindow(QQuickWindow* window)
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

void* dmaBufImportDisplay() noexcept
{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    if (GstDmaBufVideoBuffer::importDisabled()) {
        return nullptr;
    }
#ifdef QGC_UNITTEST_BUILD
    if (const std::optional<GstQgc::SinkEnvironment> environment = GstQgc::sinkEnvironmentForTest()) {
        return environment->dmaBufDisplay;
    }
#endif
    // Qt can have an EGLDisplay under Vulkan too (its global share context), which imports nothing for a Vulkan QRhi.
    if (QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire) == static_cast<int>(QRhi::Vulkan)) {
#if defined(QGC_HAS_GST_DMABUF_VULKAN_IMPORT)
        return GstDmaBufVulkan::importDevice();
#else
        return nullptr;
#endif
    }
    return QGCRhiCapture::qtEglDisplay();
#else
    return nullptr;
#endif
}

qint64 graphicsAdapterLuid() noexcept
{
#if defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
    return QGCRhiCapture::deviceSnapshot().adapterLuid.load(std::memory_order_acquire);
#else
    return 0;
#endif
}

void setGraphicsDevicePublishedHandler([[maybe_unused]] void (*handler)()) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    QGCRhiCapture::setPublishedHandler(handler);
#endif
}

bool d3d11OpensD3D12() noexcept
{
#if defined(QGC_HAS_GST_D3D11_GPU_PATH) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
#ifdef QGC_UNITTEST_BUILD
    if (const std::optional<GstQgc::SinkEnvironment> environment = GstQgc::sinkEnvironmentForTest()) {
        return environment->d3d11OpensD3D12;
    }
#endif
    return QGCRhiCapture::deviceSnapshot().d3d11OpensD3D12.load(std::memory_order_acquire);
#else
    return false;
#endif
}

bool answerSinkBinContextQuery(GstQuery* query, [[maybe_unused]] GstElement* sink) noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const HwPathDescriptor* path : GstHwPathRegistry::paths()) {
        if (path->bridge && path->bridge().answerContextQuery(query, sink)) {
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

QString extraPathStatsSince(const GstHwPathTelemetry::Snapshot& baseline)
{
    QString out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    using GstHwPathTelemetry::Counter;
    const GstHwPathTelemetry::Snapshot now = GstHwPathTelemetry::snapshot();
    const auto delta = [&now, &baseline](HwVideoBufferPath path, auto key) {
        return now.count(path, key) - baseline.count(path, key);
    };

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
        {GstHwPathTelemetry::HwFallbackReason::EglBadMatch, "egl-bad-match"},
        {GstHwPathTelemetry::HwFallbackReason::ValidateFailed, "validate"},
        {GstHwPathTelemetry::HwFallbackReason::UnknownMemType, "unknown-mem"},
        {GstHwPathTelemetry::HwFallbackReason::MapFailed, "map-failed"},
        {GstHwPathTelemetry::HwFallbackReason::VulkanNoSync, "vulkan-no-sync"},
        {GstHwPathTelemetry::HwFallbackReason::ImportUnsupported, "import-unsupported"},
        {GstHwPathTelemetry::HwFallbackReason::FdDupFailed, "fd-dup-failed"},
    };
    forEachCompiledPath([&out, &delta](HwVideoBufferPath path) {
        const QLatin1String pathLabel(GstHw::toString(path));
        for (const CounterField& field : kCounterFields) {
            const quint64 first = delta(path, field.first);
            bool nonzero = (first > 0);
            QString counts = QString::number(first);
            if (field.second) {
                const quint64 second = delta(path, *field.second);
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
            const quint64 n = delta(path, reason);
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
    const quint64 unknownMem = delta(HwVideoBufferPath::None, GstHwPathTelemetry::HwFallbackReason::UnknownMemType);
    if (unknownMem > 0) {
        out += QStringLiteral(" None-fallback[unknown-mem=%1]").arg(unknownMem);
    }
#else
    Q_UNUSED(baseline);
#endif
    return out;
}

}  // namespace HwBuffers
