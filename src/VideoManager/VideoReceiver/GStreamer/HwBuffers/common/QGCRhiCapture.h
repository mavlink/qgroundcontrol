#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/qglobal.h>
#include <atomic>

class QRhi;
class QQuickWindow;

/// Resolves and caches the `QRhi*` driving QGC's main scene graph; Qt has no global RHI accessor.
namespace QGCRhiCapture {

/// Cached QRhi* maintained by sceneGraph signals; safe from any thread via acquire ordering.
QRhi* cachedRhi() noexcept;

/// Atomic snapshot of native device handles; populated on render thread (where nativeHandles() is safe), safe from any
/// thread.
struct DeviceSnapshot
{
    std::atomic<int> backend{-1};                  ///< QRhi::Implementation cast to int (-1 = unset)
    std::atomic<void*> d3d11Device{nullptr};       ///< ID3D11Device*  (Windows only)
    std::atomic<void*> d3d12Device{nullptr};       ///< ID3D12Device*  (Windows only)
    std::atomic<qint64> adapterLuid{0};            ///< Composed (high<<32)|low LUID, 0 if unknown
    std::atomic<void*> vkPhysicalDevice{nullptr};  ///< VkPhysicalDevice (Vulkan backend only)
    std::atomic<quint32> vkQueueFamilyIdx{0};      ///< QRhi's gfx queue family (Vulkan backend only)
    std::atomic<quint32> vkQueueIdx{0};            ///< QRhi's gfx queue index (Vulkan backend only)
    std::atomic<int> framesInFlight{0};            ///< QRhi::FramesInFlight resource limit (0 = unset)
};

/// Returns the global snapshot. Atomic fields make individual reads thread-safe.
DeviceSnapshot& deviceSnapshot() noexcept;

/// Call once from the GUI thread once the main QQuickWindow exists; wires sceneGraph signals to maintain cachedRhi()
/// and the device snapshot.
void connectWindow(QQuickWindow* window);

}  // namespace QGCRhiCapture
