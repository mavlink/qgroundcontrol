#include "GstQgcRenderTarget.h"

#include <atomic>

#include <QtCore/QThread>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <rhi/qrhi.h>

#include "HwBuffers/common/QGCRhiCapture.h"
#endif

namespace GstQgc {

namespace {

constinit std::atomic<int> s_requestedBackend{static_cast<int>(RenderBackend::Unknown)};

#ifdef QGC_UNITTEST_BUILD
constexpr int kNoTestTarget = -1;
constinit std::atomic<int> s_testTarget{kNoTestTarget};

constexpr int encodeTarget(RenderTarget target) noexcept
{
    return (static_cast<int>(target.backend) << 1) | (target.egl ? 1 : 0);
}

constexpr RenderTarget decodeTarget(int encoded) noexcept
{
    return {.backend = static_cast<RenderBackend>(encoded >> 1), .egl = (encoded & 1) != 0};
}
#endif

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
    if (const int testTarget = s_testTarget.load(std::memory_order_acquire); testTarget != kNoTestTarget) {
        return decodeTarget(testTarget);
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

#ifdef QGC_UNITTEST_BUILD
ScopedRenderTargetForTest::ScopedRenderTargetForTest(RenderTarget target)
    : _previous(s_testTarget.exchange(encodeTarget(target), std::memory_order_acq_rel))
{}

ScopedRenderTargetForTest::~ScopedRenderTargetForTest()
{
    s_testTarget.store(_previous, std::memory_order_release);
}
#endif

}  // namespace GstQgc
