#include "GstQgcRenderTarget.h"

#include <atomic>
#ifdef QGC_UNITTEST_BUILD
#include <optional>
#endif

#include <QtCore/QThread>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <rhi/qrhi.h>

#include "HwBuffers/common/QGCRhiCapture.h"
#endif
#ifdef QGC_UNITTEST_BUILD
#include "GstQgcCaps.h"
#endif

namespace GstQgc {

namespace {

constinit std::atomic<int> s_requestedBackend{static_cast<int>(RenderBackend::Unknown)};

RenderBackend fromGraphicsApi(QSGRendererInterface::GraphicsApi api) noexcept
{
    switch (api) {
        case QSGRendererInterface::Unknown:
            return RenderBackend::Unknown;
        case QSGRendererInterface::OpenGL:
            return RenderBackend::OpenGL;
        case QSGRendererInterface::Direct3D11:
            return RenderBackend::Direct3D11;
        case QSGRendererInterface::Direct3D12:
            return RenderBackend::Direct3D12;
        case QSGRendererInterface::Metal:
            return RenderBackend::Metal;
        case QSGRendererInterface::Vulkan:
            return RenderBackend::Vulkan;
        default:
            return RenderBackend::Null;
    }
}

#if defined(QGC_HAS_ANY_GPU_PATH)
RenderBackend fromRhiImplementation(int implementation) noexcept
{
    switch (static_cast<QRhi::Implementation>(implementation)) {
        case QRhi::Null:
            return RenderBackend::Null;
        case QRhi::OpenGLES2:
            return RenderBackend::OpenGL;
        case QRhi::D3D11:
            return RenderBackend::Direct3D11;
        case QRhi::D3D12:
            return RenderBackend::Direct3D12;
        case QRhi::Metal:
            return RenderBackend::Metal;
        case QRhi::Vulkan:
            return RenderBackend::Vulkan;
    }
    return RenderBackend::Unknown;
}
#endif

}  // namespace

RenderTarget currentRenderTarget() noexcept
{
#ifdef QGC_UNITTEST_BUILD
    if (const std::optional<SinkEnvironment> environment = sinkEnvironmentForTest()) {
        return environment->target;
    }
#endif
    // QQuickWindow::graphicsApi() may apply Qt Quick's settings on first use, so only the GUI thread asks it.
    if (QThread::isMainThread()) {
        s_requestedBackend.store(static_cast<int>(fromGraphicsApi(QQuickWindow::graphicsApi())),
                                 std::memory_order_relaxed);
    }
    RenderTarget target{.backend = static_cast<RenderBackend>(s_requestedBackend.load(std::memory_order_relaxed))};
#if defined(QGC_HAS_ANY_GPU_PATH)
    if (const int live = QGCRhiCapture::deviceSnapshot().backend.load(std::memory_order_acquire); live >= 0) {
        target.backend = fromRhiImplementation(live);
    }
    target.egl = (target.backend == RenderBackend::OpenGL) && QGCRhiCapture::qtEglDisplay();
#endif
    return target;
}

bool RenderTarget::sharesGlContext() const noexcept
{
#if defined(Q_OS_LINUX)
    return backend == RenderBackend::OpenGL;
#else
    // WGL and CGL contexts aren't wrapped.
    return isOpenGlOnEgl();
#endif
}

const char* toString(RenderBackend backend) noexcept
{
    switch (backend) {
        case RenderBackend::Unknown:
            return "unknown";
        case RenderBackend::Null:
            return "null";
        case RenderBackend::OpenGL:
            return "OpenGL";
        case RenderBackend::Direct3D11:
            return "Direct3D11";
        case RenderBackend::Direct3D12:
            return "Direct3D12";
        case RenderBackend::Metal:
            return "Metal";
        case RenderBackend::Vulkan:
            return "Vulkan";
    }
    return "unknown";
}

const char* zeroCopyPathName(RenderBackend backend) noexcept
{
    switch (backend) {
        case RenderBackend::OpenGL:
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_DMABUF_GPU_PATH)
            return "GLMemory/DMABuf";
#else
            return "CPU";
#endif
        case RenderBackend::Direct3D11:
#if defined(QGC_HAS_GST_D3D11_GPU_PATH)
            return "D3D11";
#else
            return "CPU";
#endif
        case RenderBackend::Direct3D12:
#if defined(QGC_HAS_GST_D3D12_GPU_PATH)
            // A D3D12 device is a per-adapter singleton, so GStreamer's device on Qt's adapter is Qt's device.
            return "D3D12 (d3d12convert to RGBA)";
#else
            return "CPU";
#endif
        case RenderBackend::Metal:
#if defined(QGC_HAS_GST_IOSURFACE_GPU_PATH)
            return "IOSurface/VideoToolbox";
#else
            return "CPU";
#endif
        case RenderBackend::Vulkan:
            // Vulkan import is dormant (foreign-VkDevice guard → CPU copy), so it never delivers zero-copy today.
            return "CPU (Vulkan import dormant)";
        default:
            return "CPU";
    }
}

}  // namespace GstQgc
