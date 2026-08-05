#include "HwBuffers.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstContextBridgeRegistry.h"
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include "GstAHardwareBufferVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
#include "GstDmaBufVideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
#include "GstD3D11ContextBridge.h"
#include "GstD3D11VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstD3D12ContextBridge.h"
#include "GstD3D12VideoBuffer.h"
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
#include "GstGlContextBridge.h"
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
#include "GstVulkanContextBridge.h"
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
#include "GstIOSurfaceVideoBuffer.h"
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCRhiCapture.h"
#endif

#if defined(QGC_HAS_ANY_GPU_PATH)
#include "QGCLoggingCategory.h"
QGC_LOGGING_CATEGORY(HwBuffersFacadeLog, "Video.GStreamer.HwBuffers.Facade")
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
#include <EGL/egl.h>
#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>

#include "GstEglHelpers.h"
#endif

#include <QtCore/QByteArray>
#include <array>
#include <utility>

#include "QGCLoggingCategory.h"

namespace HwBuffers {

QGC_LOGGING_CATEGORY(HwBuffersConfigLog, "Video.GStreamer.HwBuffers.Config")

#if defined(QGC_HAS_ANY_GPU_PATH)
namespace {
// Compile-time table of enabled GPU paths; iterated by resetCachedGpuResources/formatPathStats to avoid repeating the
// ifdef ladder.
struct GpuPathEntry
{
    HwVideoBufferPath path;
    const char* label;
};

constexpr std::array<GpuPathEntry, 0
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
                                       + 1
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
                                       + 1
#endif
                     >
    kEnabledPaths = {{
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
        {HwVideoBufferPath::DmaBuf, "DMABuf"},
#endif
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
        {HwVideoBufferPath::GlMemory, "GL"},
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
        {HwVideoBufferPath::D3D11, "D3D11"},
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
        {HwVideoBufferPath::D3D12, "D3D12"},
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
        {HwVideoBufferPath::IOSurface, "IOSurface"},
#endif
#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
        {HwVideoBufferPath::AHardwareBuffer, "AHWBuf"},
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
        {HwVideoBufferPath::Vulkan, "Vulkan"},
#endif
    }};
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
    // Bridges self-register at static-init; function kept as a stable call site for future lazy init.
}

const HwBufferEnvConfig& hwBufferEnvConfig() noexcept
{
    static const HwBufferEnvConfig cfg = []() noexcept {
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
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || \
    defined(QGC_HAS_GST_D3D12_GPU_PATH) || defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    return GstContextBridgeRegistry::dispatchBridges(msg);
#else
    Q_UNUSED(msg);
    return GST_BUS_PASS;
#endif
}

void onPipelineRestart() noexcept
{
    resetCachedGpuResources();
    // GL is the only path needing a pipeline-restart rearm (re-prime the shared GstGLContext).
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    GstGlContextBridge::rearm();
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    GstVulkanContextBridge::rearm();
#endif
}

void resetCachedGpuResources() noexcept
{
#if defined(QGC_HAS_ANY_GPU_PATH)
    GstContextBridgeRegistry::resetAllBridges();
    GstContextBridgeRegistry::resetAllCaches();
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    GstEglHelpers::resetExtensionCache();
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

#if defined(QGC_HAS_ANY_GPU_PATH)
HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept
{
    HwVideoBufferContext ctx;
    ctx.gpuEnabled = gpuEnabled;
    if (!gpuEnabled) {
        return ctx;
    }

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    // Construction-time hint only; render-time mapTextures() prefers eglGetCurrentDisplay() to avoid xcb_egl EGLDisplay
    // mismatches.
    EGLDisplay dpy = EGL_NO_DISPLAY;
    const QString platform = QGuiApplication::platformName();
    if (platform == QLatin1String("wayland") || platform == QLatin1String("wayland-egl")) {
        if (auto* ni = QGuiApplication::platformNativeInterface()) {
            dpy = static_cast<EGLDisplay>(ni->nativeResourceForIntegration("egldisplay"));
        }
    }
    if (dpy == EGL_NO_DISPLAY) {
        dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
    if (dpy == EGL_NO_DISPLAY) {
        qCWarning(HwBuffersFacadeLog) << "GPU zero-copy requested but EGLDisplay unavailable on platform" << platform
                                      << "— DMABuf path disabled";
    } else {
        qCInfo(HwBuffersFacadeLog) << "DMABuf zero-copy path available on" << platform
                                   << "— actual path chosen at caps negotiation";
    }
    ctx.dmaBufEglDisplay = dpy;
#endif

#if defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)
    ctx.ahbEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx.ahbEglDisplay == EGL_NO_DISPLAY) {
        qCWarning(HwBuffersFacadeLog) << "AHardwareBuffer path: EGLDisplay unavailable";
    } else {
        qCInfo(HwBuffersFacadeLog) << "AHardwareBuffer zero-copy path available"
                                   << "— actual path chosen at caps negotiation";
    }
#endif

#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    qCInfo(HwBuffersFacadeLog) << "D3D11 zero-copy path available — actual path chosen at caps negotiation";
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    qCInfo(HwBuffersFacadeLog) << "D3D12 zero-copy path available — actual path chosen at caps negotiation";
#endif
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
    qCInfo(HwBuffersFacadeLog) << "IOSurface zero-copy path available — actual path chosen at caps negotiation";
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    qCInfo(HwBuffersFacadeLog) << "Vulkan zero-copy path available (active only when QRhi uses the Vulkan backend)";
#endif

    return ctx;
}
#endif  // QGC_HAS_ANY_GPU_PATH

bool answerSinkBinContextQuery(GstQuery* query) noexcept
{
    bool handled = false;
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    handled = GstGlContextBridge::answerContextQuery(query);
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    if (!handled) {
        handled = GstVulkanContextBridge::answerContextQuery(query);
    }
#endif
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (!handled) {
        handled = GstD3D11ContextBridge::answerContextQuery(query);
    }
#endif
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (!handled) {
        handled = GstD3D12ContextBridge::answerContextQuery(query);
    }
#endif
    Q_UNUSED(query);
    return handled;
}

PathStats formatPathStats(bool reset) noexcept
{
    PathStats out;
#if defined(QGC_HAS_ANY_GPU_PATH)
    for (const auto& entry : kEnabledPaths) {
        const quint64 delivered = reset ? GstHwPathTelemetry::takeDeliveredCount(entry.path)
                                        : GstHwPathTelemetry::peekDeliveredCount(entry.path);
        const quint64 failures = reset ? GstHwPathTelemetry::takeMapFailureCount(entry.path)
                                       : GstHwPathTelemetry::peekMapFailureCount(entry.path);
        // Teardown (reset) uses expanded label so operators can copy the failures field into bug reports.
        if (reset) {
            out.line +=
                QStringLiteral(" %1:%2 %1-failures:%3").arg(QLatin1String(entry.label)).arg(delivered).arg(failures);
        } else {
            out.line += QStringLiteral(" %1:%2/%3").arg(QLatin1String(entry.label)).arg(delivered).arg(failures);
        }
        out.totalDelivered += delivered;
    }
#else
    Q_UNUSED(reset);
#endif
    return out;
}

QString takeExtraPathStats() noexcept
{
    QString out;
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)
    const quint64 glReuse = GstHwPathTelemetry::takeTextureReuseHits(HwVideoBufferPath::GlMemory);
    quint64 glGpuWaits = 0;
    const quint64 glCpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(HwVideoBufferPath::GlMemory, glGpuWaits);
    out += QStringLiteral(" GL-reuse:%1 GL-wait[gpu/cpu]:%2/%3").arg(glReuse).arg(glGpuWaits).arg(glCpuWaits);
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH)
    quint64 vkGpuWaits = 0;
    const quint64 vkCpuWaits = GstHwPathTelemetry::takeSyncWaitCounts(HwVideoBufferPath::Vulkan, vkGpuWaits);
    out += QStringLiteral(" Vulkan-wait[gpu/cpu]:%1/%2").arg(vkGpuWaits).arg(vkCpuWaits);
#endif
#if defined(QGC_HAS_GST_DMABUF_GPU_PATH)
    out += QStringLiteral(" DMABuf-fence-timeouts:%1 DMABuf-mmap-barriers:%2 DMABuf-explicit-fence-waits:%3")
               .arg(GstHwPathTelemetry::takeFenceTimeouts(HwVideoBufferPath::DmaBuf))
               .arg(GstHwPathTelemetry::takeMmapBarrierHits(HwVideoBufferPath::DmaBuf))
               .arg(GstHwPathTelemetry::takeExplicitFenceWaits(HwVideoBufferPath::DmaBuf));
#endif
#if defined(QGC_HAS_ANY_GPU_PATH)
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
    for (const auto& entry : kEnabledPaths) {
        const quint64 ewmaUs = GstHwPathTelemetry::peekMapDurationUsEwma(entry.path);
        if (ewmaUs > 0) {
            out += QStringLiteral(" %1-map-us:%2").arg(QLatin1String(entry.label)).arg(ewmaUs);
        }
        const quint64 demotions = GstHwPathTelemetry::takeStreamDemotions(entry.path);
        if (demotions > 0) {
            out += QStringLiteral(" %1-demotions:%2").arg(QLatin1String(entry.label)).arg(demotions);
        }
        QString reasonBreakdown;
        for (const auto& [reason, label] : kReasons) {
            const quint64 n = GstHwPathTelemetry::takeFallbackReason(entry.path, reason);
            if (n > 0) {
                reasonBreakdown += QStringLiteral("%1=%2,").arg(QLatin1String(label)).arg(n);
            }
        }
        if (!reasonBreakdown.isEmpty()) {
            reasonBreakdown.chop(1);
            out += QStringLiteral(" %1-fallback[%2]").arg(QLatin1String(entry.label)).arg(reasonBreakdown);
        }
    }
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
