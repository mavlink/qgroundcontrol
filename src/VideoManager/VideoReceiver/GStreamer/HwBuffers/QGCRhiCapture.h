#pragma once

#include <QtCore/qglobal.h>
#include <QtCore/QObject>
#include <QtCore/QString>

#include <atomic>

class QRhi;
class QQuickWindow;

/// Resolves the live `QRhi*` driving QGC's main scene graph so platform-specific
/// GPU-handoff code (D3D11 device sharing, Metal device matching) can target the
/// same device QRhi is rendering on.
///
/// Qt does not expose a global RHI accessor — `QQuickWindow::rhi()` is the only
/// public API and requires a window with an initialized scene graph. This helper
/// walks `QGCApplication::mainRootWindow()` lazily on each call.
namespace QGCRhiCapture {

/// Returns the QRhi for QGC's main QQuickWindow once its scene graph is up.
/// Safe to call from the GUI thread or the render thread (QQuickWindow::rhi()
/// is documented safe on both). Returns nullptr if the window doesn't exist
/// yet, isn't initialized, or QGC is running headless.
QRhi *qrhi();

/// Thread-safe snapshot populated from sceneGraphInitialized and cleared on
/// sceneGraphInvalidated. Safe to read from any thread via acquire ordering.
QRhi *cachedRhi() noexcept;

/// Immutable, thread-safe snapshot of the QRhi-owned native device handles.
/// Populated on the render thread when sceneGraphInitialized fires (where
/// QRhi::nativeHandles() is documented safe), cleared on sceneGraphInvalidated.
/// All fields read via acquire-ordering atomic loads — safe from any thread,
/// including GstBus sync-message callbacks where touching QRhi* would be UB.
struct DeviceSnapshot {
    std::atomic<int> backend{-1};            ///< QRhi::Implementation cast to int (-1 = unset)
    std::atomic<void*> d3d11Device{nullptr}; ///< ID3D11Device*  (Windows only)
    std::atomic<void*> d3d12Device{nullptr}; ///< ID3D12Device*  (Windows only)
    std::atomic<qint64> adapterLuid{0};      ///< Composed (high<<32)|low LUID, 0 if unknown
};

/// Returns the global snapshot. Atomic fields make individual reads thread-safe.
DeviceSnapshot &deviceSnapshot() noexcept;

/// Call once from the GUI thread after the main QQuickWindow is available.
/// Connects sceneGraphInitialized / sceneGraphInvalidated to maintain cachedRhi()
/// and the device snapshot.
void connectWindow(QQuickWindow *window);

} // namespace QGCRhiCapture
