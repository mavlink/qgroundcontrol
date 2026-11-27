#pragma once

/// The HwBuffers facade: all that the other folders use of the GPU paths, the same in every build and free of EGL and
/// Qt-private headers. The folders below HwBuffers/ are include directories of the GStreamer library alone, so the
/// headers this one shares are included by their path from here.

#include <gst/gst.h>
#include <gst/video/video-info.h>
#include <memory>

#include <QtCore/QString>
#include <QtCore/qglobal.h>
#include <QtMultimedia/QVideoFrameFormat>

#include "common/GstHwFrameMapping.h"   // GstHw::FrameMapping, the CPU frame map
#include "common/GstHwPathTelemetry.h"  // the GPU path statistics
#include "common/GstHwTypes.h"          // HwVideoBufferPath, HwVideoBufferContext
#include "common/GstHwVideoInfo.h"      // GstHw::dmaDrmAwareVideoInfo()
#include "dmabuf/GstDmaDrmCaps.h"       // the DMA_DRM sink caps, with the DMABuf path only

class QHwVideoBuffer;
class QQuickWindow;

/// Per-stream GPU path cache, keyed to a caps epoch. Once a sample validated on a path, makeHwVideoBuffer() tries that
/// path's descriptors first; an allocator no path imports skips GPU selection altogether. Reset on set_caps.
struct HwResolvedPathCache
{
    HwVideoBufferPath path = HwVideoBufferPath::None;
    bool validated = false;                     // path resolved by a prior validated sample this epoch
    GType unmatchedAllocator = G_TYPE_INVALID;  // allocator no compiled path imports; its frames go straight to the CPU
    bool validationFailureLogged = false;       // one warning per epoch for buffers that fail plane-handle validation
};

/// Selects the zero-copy QHwVideoBuffer for @p buffer; returns nullptr when no GPU path matches (CPU fallback). The
/// GstSample the buffer holds is only created once a path matches. Defined only in builds with a GPU path.
std::unique_ptr<QHwVideoBuffer> makeHwVideoBuffer(GstBuffer* buffer, GstCaps* caps, const GstVideoInfo& info,
                                                  QVideoFrameFormat format, const HwVideoBufferContext& context,
                                                  HwVideoBufferPath& matchedPath, HwResolvedPathCache* cache = nullptr);

namespace HwBuffers {

/// Name of the element message onGraphicsDeviceRemoved() posts on the pipelines a context bridge gave its device,
/// once that device is reported removed (a driver reset or update): their GPU elements can't go on, and they hold
/// references that keep the device's successor from being created.
inline constexpr char kGraphicsDeviceLostMessage[] = "qgc-graphics-device-lost";

// noexcept marks what GStreamer's C callbacks call straight into (qgcqvideosink's vfuncs): no exception may unwind
// through those, so one thrown here, such as a failed allocation, ends the process on purpose.

/// For a receiver's pipeline, before its decoder and video sink join it: the context bridges set their contexts on
/// @p pipeline, which hands them to every element it is given, so the GPU elements work on the device Qt renders with.
/// A bridge that can't prime, as before graphicsDeviceSettled(), sets none. No-op when no GPU path is compiled.
void provideGraphicsContexts(GstElement* pipeline);

/// False while the device Qt renders the main window with is unknown: the window is connected (connectMainWindow()),
/// its scene graph renders through a QRhi, and isn't up, as before the window is first exposed or after its scene graph
/// was torn down (a device loss). A decoder plugged meanwhile would work on a device of its own for as long as it ran.
/// True otherwise: with no main window (unit tests), with a scene graph of another kind (the software one), and
/// without a GPU path. Each time it turns true, the handler of setGraphicsDevicePublishedHandler() runs. Safe off the
/// GUI thread.
bool graphicsDeviceSettled() noexcept;

/// Drop process-wide native GPU handles tied to the current Qt scene-graph device/context. Affects every running
/// stream, so only for scene-graph teardown, window swaps and device loss.
void resetCachedGpuResources();

/// For a context bridge whose device was reported removed: resetCachedGpuResources(), then kGraphicsDeviceLostMessage
/// on every pipeline a bridge had given its device. GUI thread.
void onGraphicsDeviceRemoved();

/// True from when a context bridge's device is reported removed until that bridge has a device again: a decoder
/// that fails meanwhile isn't at fault. Always false when no GPU path is compiled. Safe off the GUI thread.
bool graphicsDeviceLost() noexcept;

/// Qt graphics setup the GPU paths need, which Qt reads at startup: call before the QGuiApplication exists. With the
/// GL or DMABuf path it shares Qt's OpenGL contexts and, on desktop Linux, selects Qt's xcb EGL integration unless
/// QT_XCB_GL_INTEGRATION is set; with the DMABuf import under Vulkan it asks for the device extensions that takes,
/// should Qt Quick render with Vulkan.
void configureQtGraphics(bool softwareRendering);

/// What configureQtGraphics() was told: the user asked for software rendering (--swrast). Safe off the GUI thread.
bool softwareRenderingRequested() noexcept;

/// Wire the main QQuickWindow into the RHI-capture path so snapshots, and the EGLDisplay Qt renders with, follow its
/// QRhi. GUI thread; no-op without GPU.
void connectMainWindow(QQuickWindow* window);

/// The QRhi::Implementation the scene graph renders with, as its render thread published it; -1 until it did, and
/// always without a GPU path. Safe off the GUI thread.
int renderBackend() noexcept;

/// Qt's EGLDisplay once Qt's OpenGL is known to run on EGL, else null; always null without a GPU path. Safe off the
/// GUI thread.
void* qtEglDisplay() noexcept;

/// What the DMABuf path imports with now, as HwVideoBufferContext::dmaBufImporter stores it: Qt's EGLDisplay
/// (QGCRhiCapture::qtEglDisplay()), or, once Qt Quick renders with Vulkan, its VkPhysicalDevice when that device
/// imports dma-bufs. Null when Qt renders without either or that isn't known yet, or no DMABuf path is compiled.
/// Safe off the GUI thread.
void* dmaBufImportDisplay() noexcept;

/// True once an import failure has disabled the DMABuf path, until resetCachedGpuResources(): nothing imports with
/// its display meanwhile. Safe off the GUI thread.
bool dmaBufImportDisabled() noexcept;

/// The DXGI adapter of the Direct3D device Qt Quick renders with, as the decoders' "adapter-luid" names theirs; 0
/// until the scene graph publishes that device, and for any other renderer. Safe off the GUI thread.
qint64 graphicsAdapterLuid() noexcept;

/// Has @p handler called each time the main window's scene graph comes up, on its render thread, once it has published
/// the device Qt Quick renders with, if any: what depends on that device (the decoder ranks, the decoders) follows it
/// then. Null for none. No-op when no GPU path is compiled.
void setGraphicsDevicePublishedHandler(void (*handler)()) noexcept;

/// True when the Direct3D 11 device Qt Quick renders with is known to open a Direct3D 12 device's resources as
/// textures, which is how the D3D12 path shows a frame there. False until the scene graph publishes that device, and
/// in builds without both D3D paths. Safe off the GUI thread.
bool d3d11OpensD3D12() noexcept;

/// Add a buffer pool on Qt's D3D11 device to an ALLOCATION query for caps in that memory, so the decoder allocates on
/// the device the importer samples from. False for any other memory, or before the device bridge is primed.
bool proposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size,
                            guint minBuffers) noexcept;

/// Process-wide render-time counters no sink can attribute (map failures, producer waits, fallback
/// reasons, ...) accumulated since @p baseline, each printed only when nonzero. Reading never resets them, so every
/// receiver can report its own window.
QString extraPathStatsSince(const GstHwPathTelemetry::Snapshot& baseline);

}  // namespace HwBuffers
