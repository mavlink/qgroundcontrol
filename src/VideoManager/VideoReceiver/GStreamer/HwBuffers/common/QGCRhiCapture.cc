#include "QGCRhiCapture.h"

#if defined(QGC_HAS_ANY_GPU_PATH)
#include "HwBuffers.h"
#endif

#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>
#include <rhi/qrhi.h>
#if (defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))) || \
    (defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan))
#include <rhi/qrhi_platform.h>
#endif

#include <array>
#include <atomic>

namespace QGCRhiCapture {

namespace {
std::atomic<QRhi*> s_cachedRhi{nullptr};
// The connected window (or null after it is destroyed); read only by connectWindow's idempotency
// guard. Atomic because the destroyed lambda nulls it on the window's thread.
std::atomic<QQuickWindow*> s_connectedWindow{nullptr};
DeviceSnapshot s_snapshot;
// Per-connection so a window swap (e.g. popout video) can disconnect the prior window's lambdas before they clobber the
// new window's cached RHI.
std::array<QMetaObject::Connection, 3> s_connections;

void clearSnapshot()
{
    s_snapshot.backend.store(-1, std::memory_order_release);
    s_snapshot.d3d11Device.store(nullptr, std::memory_order_release);
    s_snapshot.d3d12Device.store(nullptr, std::memory_order_release);
    s_snapshot.adapterLuid.store(0, std::memory_order_release);
    s_snapshot.vkPhysicalDevice.store(nullptr, std::memory_order_release);
    s_snapshot.vkQueueFamilyIdx.store(0, std::memory_order_release);
    s_snapshot.vkQueueIdx.store(0, std::memory_order_release);
    s_snapshot.framesInFlight.store(0, std::memory_order_release);
}

void populateSnapshot(QRhi* rhi)
{
    if (!rhi) {
        clearSnapshot();
        return;
    }
    const int backend = static_cast<int>(rhi->backend());
    void* d3d11 = nullptr;
    void* d3d12 = nullptr;
    qint64 luid = 0;
    void* vkPhysDev = nullptr;
    quint32 vkQueueFamilyIdx = 0;
    quint32 vkQueueIdx = 0;

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (rhi->backend() == QRhi::D3D11) {
        if (auto* h = static_cast<const QRhiD3D11NativeHandles*>(rhi->nativeHandles())) {
            d3d11 = h->dev;
            // Sign-extend qint32 HighPart so it matches LARGE_INTEGER::QuadPart bit-for-bit.
            luid = (static_cast<qint64>(h->adapterLuidHigh) << 32) |
                   (static_cast<qint64>(h->adapterLuidLow) & 0xFFFFFFFFLL);
        }
    }
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (rhi->backend() == QRhi::D3D12) {
        if (auto* h = static_cast<const QRhiD3D12NativeHandles*>(rhi->nativeHandles())) {
            d3d12 = h->dev;
            luid = (static_cast<qint64>(h->adapterLuidHigh) << 32) |
                   (static_cast<qint64>(h->adapterLuidLow) & 0xFFFFFFFFLL);
        }
    }
#endif
#if defined(QGC_HAS_GST_VULKAN_GPU_PATH) && QT_CONFIG(vulkan)
    if (rhi->backend() == QRhi::Vulkan) {
        if (auto* h = static_cast<const QRhiVulkanNativeHandles*>(rhi->nativeHandles())) {
            vkPhysDev = static_cast<void*>(h->physDev);
            vkQueueFamilyIdx = h->gfxQueueFamilyIdx;
            vkQueueIdx = h->gfxQueueIdx;
        }
    }
#endif

    // Publish device pointers before the backend tag so a consumer's acquire-load on backend != -1 is guaranteed to see
    // populated handles.
    s_snapshot.d3d11Device.store(d3d11, std::memory_order_release);
    s_snapshot.d3d12Device.store(d3d12, std::memory_order_release);
    s_snapshot.adapterLuid.store(luid, std::memory_order_release);
    s_snapshot.vkPhysicalDevice.store(vkPhysDev, std::memory_order_release);
    s_snapshot.vkQueueFamilyIdx.store(vkQueueFamilyIdx, std::memory_order_release);
    s_snapshot.vkQueueIdx.store(vkQueueIdx, std::memory_order_release);
    s_snapshot.framesInFlight.store(rhi->resourceLimit(QRhi::FramesInFlight), std::memory_order_release);
    s_snapshot.backend.store(backend, std::memory_order_release);
}
}  // namespace

QRhi* cachedRhi() noexcept
{
    return s_cachedRhi.load(std::memory_order_acquire);
}

DeviceSnapshot& deviceSnapshot() noexcept
{
    return s_snapshot;
}

void connectWindow(QQuickWindow* window)
{
    if (!window)
        return;
    if (s_connectedWindow.load(std::memory_order_acquire) == window)
        return;  // idempotent — avoid duplicate connections

    // Window swap: the old window's QRhi can be torn down before the new window's SG initializes; drop the cached
    // QRhi/snapshot and native GPU handles now so bus-sync threads never dereference a dead device across the gap.
    if (s_connectedWindow.load(std::memory_order_acquire) != nullptr) {
        s_cachedRhi.store(nullptr, std::memory_order_release);
        clearSnapshot();
#if defined(QGC_HAS_ANY_GPU_PATH)
        HwBuffers::resetCachedGpuResources();
#endif
    }

    // Detach the prior window's signals first, else its destroyed/invalidated lambdas later wipe cachedRhi() and reset
    // bridges the new window is using.
    for (std::size_t i = 0; i < s_connections.size(); ++i) {
        auto& conn = s_connections[i];
        QObject::disconnect(conn);
        conn = QMetaObject::Connection();
    }

    s_connectedWindow.store(window, std::memory_order_release);
    // sceneGraphInitialized fires on the render thread where rhi() is valid — snapshot native device pointers here so
    // bus-sync callbacks never deref QRhi* cross-thread.
    s_connections[0] = QObject::connect(
        window, &QQuickWindow::sceneGraphInitialized, window,
        [window]() {
            QRhi* rhi = window->rhi();
            s_cachedRhi.store(rhi, std::memory_order_release);
            populateSnapshot(rhi);
        },
        Qt::DirectConnection);
    s_connections[1] = QObject::connect(
        window, &QQuickWindow::sceneGraphInvalidated, window,
        []() {
            s_cachedRhi.store(nullptr, std::memory_order_release);
            clearSnapshot();
#if defined(QGC_HAS_ANY_GPU_PATH)
            // Drop native GPU handles that wrap the now-defunct QRhi-owned device/context.
            HwBuffers::resetCachedGpuResources();
#endif
        },
        Qt::DirectConnection);
    // Clear cache when window is destroyed so a stale QRhi* is never returned.
    s_connections[2] = QObject::connect(
        window, &QQuickWindow::destroyed, window,
        [](QObject*) {
            s_connectedWindow.store(nullptr, std::memory_order_release);
            s_cachedRhi.store(nullptr, std::memory_order_release);
            clearSnapshot();
#if defined(QGC_HAS_ANY_GPU_PATH)
            HwBuffers::resetCachedGpuResources();
#endif
        },
        Qt::DirectConnection);

    // SG already initialized (video init runs after first render) — publish on the render thread, where rhi() is valid.
    if (window->isSceneGraphInitialized()) {
        window->scheduleRenderJob(QRunnable::create([window]() {
                                      QRhi* rhi = window->rhi();
                                      s_cachedRhi.store(rhi, std::memory_order_release);
                                      populateSnapshot(rhi);
                                  }),
                                  QQuickWindow::BeforeSynchronizingStage);
        window->update();
    }
}

}  // namespace QGCRhiCapture
