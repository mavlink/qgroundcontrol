#include "QGCRhiCapture.h"

#include "QGCApplication.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstContextBridgeRegistry.h"
#endif

#include <QtCore/QPointer>
#include <QtQuick/QQuickWindow>

#include <rhi/qrhi.h>
#if defined(Q_OS_WIN) && (defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH))
#include <rhi/qrhi_platform.h>
#endif

#include <array>
#include <atomic>

namespace QGCRhiCapture {

namespace {
std::atomic<QRhi*> s_cachedRhi{nullptr};
QPointer<QQuickWindow> s_connectedWindow;
DeviceSnapshot s_snapshot;
// Stored per connection so a window swap (e.g. popout video) can disconnect the prior window's
// lambdas before they fire and clobber the new window's cached RHI.
std::array<QMetaObject::Connection, 3> s_connections;

void clearSnapshot()
{
    s_snapshot.backend.store(-1, std::memory_order_release);
    s_snapshot.d3d11Device.store(nullptr, std::memory_order_release);
    s_snapshot.d3d12Device.store(nullptr, std::memory_order_release);
    s_snapshot.adapterLuid.store(0, std::memory_order_release);
}

void populateSnapshot(QRhi *rhi)
{
    if (!rhi) {
        clearSnapshot();
        return;
    }
    const int backend = static_cast<int>(rhi->backend());
    void *d3d11 = nullptr;
    void *d3d12 = nullptr;
    qint64 luid = 0;

#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D11_GPU_PATH)
    if (rhi->backend() == QRhi::D3D11) {
        if (auto *h = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles())) {
            d3d11 = h->dev;
            // Sign-extend qint32 HighPart so it matches LARGE_INTEGER::QuadPart bit-for-bit.
            luid = (static_cast<qint64>(h->adapterLuidHigh) << 32)
                 | (static_cast<qint64>(h->adapterLuidLow) & 0xFFFFFFFFLL);
        }
    }
#endif
#if defined(Q_OS_WIN) && defined(QGC_HAS_GST_D3D12_GPU_PATH)
    if (rhi->backend() == QRhi::D3D12) {
        if (auto *h = static_cast<const QRhiD3D12NativeHandles *>(rhi->nativeHandles())) {
            d3d12 = h->dev;
            luid = (static_cast<qint64>(h->adapterLuidHigh) << 32)
                 | (static_cast<qint64>(h->adapterLuidLow) & 0xFFFFFFFFLL);
        }
    }
#endif

    // Publish device pointers before the backend tag so consumers seeing backend != -1 are
    // guaranteed to see populated handles (acquire-load on backend pairs with these releases).
    s_snapshot.d3d11Device.store(d3d11, std::memory_order_release);
    s_snapshot.d3d12Device.store(d3d12, std::memory_order_release);
    s_snapshot.adapterLuid.store(luid, std::memory_order_release);
    s_snapshot.backend.store(backend, std::memory_order_release);
}
} // namespace

QRhi *qrhi()
{
    QGCApplication *app = qgcApp();
    if (!app) return nullptr;
    QQuickWindow *win = app->mainRootWindow();
    if (!win) return nullptr;
    return win->rhi();
}

QRhi *cachedRhi() noexcept
{
    return s_cachedRhi.load(std::memory_order_acquire);
}

DeviceSnapshot &deviceSnapshot() noexcept
{
    return s_snapshot;
}

void connectWindow(QQuickWindow *window)
{
    if (!window) return;
    if (s_connectedWindow == window) return;  // idempotent — avoid duplicate connections

    // Detach prior window's signals before binding the new one — otherwise the old window's
    // destroyed/invalidated lambdas would later wipe cachedRhi() and reset bridges that the
    // new window is actively using.
    for (auto &conn : s_connections) {
        QObject::disconnect(conn);
        conn = QMetaObject::Connection();
    }

    s_connectedWindow = window;
    // sceneGraphInitialized fires on the render thread where rhi() and rhi->nativeHandles() are
    // valid — snapshot the native device pointers here so bus-sync callbacks never need to
    // dereference QRhi* across threads.
    s_connections[0] = QObject::connect(window, &QQuickWindow::sceneGraphInitialized, window, [window]() {
        QRhi *rhi = window->rhi();
        s_cachedRhi.store(rhi, std::memory_order_release);
        populateSnapshot(rhi);
    }, Qt::DirectConnection);
    s_connections[1] = QObject::connect(window, &QQuickWindow::sceneGraphInvalidated, window, []() {
        s_cachedRhi.store(nullptr, std::memory_order_release);
        clearSnapshot();
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
        // Drop bridge-cached GPU devices that wrap the now-defunct QRhi-owned device.
        GstContextBridgeRegistry::resetAllBridges();
#endif
    }, Qt::DirectConnection);
    // Clear cache when window is destroyed so a stale QRhi* is never returned.
    s_connections[2] = QObject::connect(window, &QQuickWindow::destroyed, window, [](QObject *) {
        s_cachedRhi.store(nullptr, std::memory_order_release);
        clearSnapshot();
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
        GstContextBridgeRegistry::resetAllBridges();
#endif
    }, Qt::DirectConnection);
}

} // namespace QGCRhiCapture
