#pragma once

#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/qglobal.h>

class QRhi;
class QQuickWindow;

/// Publishes the native device of the `QRhi` driving QGC's main scene graph; Qt has no global RHI accessor.
namespace QGCRhiCapture {

/// Atomic snapshot of native device handles; populated on render thread (where nativeHandles() is safe), safe from any
/// thread.
struct DeviceSnapshot
{
    std::atomic<int> backend{-1};                  ///< QRhi::Implementation cast to int (-1 = unset)
    std::atomic<void*> d3d11Device{nullptr};       ///< ID3D11Device*  (Windows only)
    std::atomic<qint64> adapterLuid{0};            ///< Composed (high<<32)|low LUID, 0 if unknown
    /// The Direct3D 11 device Qt renders with opens a Direct3D 12 device's resources as textures, as the D3D12 path
    /// found on the render thread; false for any other backend.
    std::atomic<bool> d3d11OpensD3D12{false};
};

/// Returns the global snapshot. Atomic fields make individual reads thread-safe.
DeviceSnapshot& deviceSnapshot() noexcept;

#ifdef QGC_UNITTEST_BUILD
/// Publishes @p rhi's device in deviceSnapshot() as the scene graph's render thread does for its own, and withdraws it
/// when null. On the thread @p rhi lives on.
void publishDeviceSnapshotForTest(QRhi* rhi);
#endif

/// Call once from the GUI thread once the main QQuickWindow exists; wires sceneGraph signals to maintain the device
/// snapshot.
void connectWindow(QQuickWindow* window);

#ifdef QGC_UNITTEST_BUILD
/// connectWindow(@p window), its scene graph counting as Qt Quick's QRhi one whatever this process renders with (the
/// unit tests' is the software one).
void connectQRhiWindowForTest(QQuickWindow* window);
#endif

/// Has @p handler called each time the connected window's scene graph comes up, once its device, if any, is in
/// deviceSnapshot(), on the render thread; null for none.
void setPublishedHandler(void (*handler)()) noexcept;

/// HwBuffers::graphicsDeviceSettled(): no window is connected, its scene graph doesn't render through a QRhi, or it is
/// up. Any thread.
bool deviceSettled() noexcept;

/// The EGLDisplay (as void*) Qt's OpenGL renders with; null when Qt's OpenGL doesn't run on EGL (GLX, WGL, CGL, no
/// OpenGL) or that isn't known yet. Taken from Qt's global share context, else from the scene graph's OpenGL context
/// once it runs. Any thread.
void* qtEglDisplay();

}  // namespace QGCRhiCapture
