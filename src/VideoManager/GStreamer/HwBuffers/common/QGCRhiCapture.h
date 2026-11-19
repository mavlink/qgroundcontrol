#pragma once

#include <array>
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
    /// The Vulkan physical device Qt renders on, as its VkPhysicalDeviceProperties name it: the VkPhysicalDevice handle
    /// belongs to Qt's VkInstance and matches nothing in another one, such as GStreamer's. Zero unless the backend is
    /// Vulkan and the gst-vulkan path is compiled.
    std::atomic<quint32> vkVendorId{0};                         ///< vendorID
    std::atomic<quint32> vkDeviceId{0};                         ///< deviceID
    std::array<std::atomic<quint64>, 2> vkPipelineCacheUuid{};  ///< pipelineCacheUUID, 8 bytes each in memory order
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

/// Has @p handler called after each publication of a device in deviceSnapshot(), on the thread that publishes it
/// (the render thread); null for none.
void setPublishedHandler(void (*handler)()) noexcept;

/// The EGLDisplay (as void*) Qt's OpenGL renders with; null when Qt's OpenGL doesn't run on EGL (GLX, WGL, CGL, no
/// OpenGL) or that isn't known yet. Taken from Qt's global share context, else from the platform integration (asked on
/// the GUI thread only), else from the scene graph's OpenGL context once it runs. Any thread.
void* qtEglDisplay();

}  // namespace QGCRhiCapture
