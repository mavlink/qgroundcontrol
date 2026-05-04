#include "QGCRhiCapture.h"

#include "QGCApplication.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
#include "GstContextBridgeRegistry.h"
#endif

#include <QtCore/QPointer>
#include <QtQuick/QQuickWindow>

#include <array>
#include <atomic>

namespace QGCRhiCapture {

namespace {
std::atomic<QRhi*> s_cachedRhi{nullptr};
QPointer<QQuickWindow> s_connectedWindow;
// Stored per connection so a window swap (e.g. popout video) can disconnect the prior window's
// lambdas before they fire and clobber the new window's cached RHI.
std::array<QMetaObject::Connection, 3> s_connections;
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
    s_connections[0] = QObject::connect(window, &QQuickWindow::sceneGraphInitialized, window, [window]() {
        s_cachedRhi.store(window->rhi(), std::memory_order_release);
    }, Qt::DirectConnection);
    s_connections[1] = QObject::connect(window, &QQuickWindow::sceneGraphInvalidated, window, []() {
        s_cachedRhi.store(nullptr, std::memory_order_release);
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
        // Drop bridge-cached GPU devices that wrap the now-defunct QRhi-owned device.
        GstContextBridgeRegistry::resetAllBridges();
#endif
    }, Qt::DirectConnection);
    // Clear cache when window is destroyed so a stale QRhi* is never returned.
    s_connections[2] = QObject::connect(window, &QQuickWindow::destroyed, window, [](QObject *) {
        s_cachedRhi.store(nullptr, std::memory_order_release);
#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH) || defined(QGC_HAS_GST_D3D11_GPU_PATH) || defined(QGC_HAS_GST_D3D12_GPU_PATH)
        GstContextBridgeRegistry::resetAllBridges();
#endif
    }, Qt::DirectConnection);
}

} // namespace QGCRhiCapture
