#pragma once

/// Umbrella header / public facade for the HwBuffers subsystem; per-platform bridges stay internal.

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include <QtCore/QString>
#include <QtCore/qglobal.h>

class QQuickWindow;

// Same in every build and free of EGL/Qt-private headers; GstHwVideoBuffer.h (Qt-private) is included where needed.
#include "GstHwPathTelemetry.h"
#include "GstHwTypes.h"
#include "GstHwVideoBufferFactory.h"  // makeHwVideoBuffer, HwResolvedPathCache

namespace HwBuffers {

/// Name of the element message a context bridge posts on a pipeline it gives the device Qt renders with after an
/// element of the pipeline had asked for one in vain: what was built meanwhile works on another device, and keeps it.
inline constexpr char kGraphicsDeviceServedMessage[] = "qgc-graphics-device-served";

/// Name of the element message onGraphicsDeviceRemoved() posts on the pipelines a context bridge gave its device,
/// once that device is reported removed (a driver reset or update): their GPU elements can't go on, and they hold
/// references that keep the device's successor from being created.
inline constexpr char kGraphicsDeviceLostMessage[] = "qgc-graphics-device-lost";

// noexcept marks what GStreamer's C callbacks call straight into (a bus sync handler, qgcqvideosink's vfuncs): no
// exception may unwind through those, so one thrown here, such as a failed allocation, ends the process on purpose.

/// For a receiver's bus sync handler: the context bridges answer NEED_CONTEXT. GST_BUS_DROP means @p msg was consumed
/// (and unreffed); always GST_BUS_PASS when no GPU path is compiled.
GstBusSyncReply onBusSyncMessage(GstMessage* msg) noexcept;

/// Drop process-wide native GPU handles tied to the current Qt scene-graph device/context. Affects every running
/// stream, so only for scene-graph teardown, window swaps and device loss.
void resetCachedGpuResources();

/// For a context bridge whose device was reported removed: resetCachedGpuResources(), then kGraphicsDeviceLostMessage
/// on every pipeline a bridge had given its device. GUI thread.
void onGraphicsDeviceRemoved();

/// True from when a context bridge's device is reported removed until that bridge has a device again: a decoder
/// that fails meanwhile isn't at fault. Always false when no GPU path is compiled. Safe off the GUI thread.
bool graphicsDeviceLost() noexcept;

/// Qt graphics setup the GPU paths need, which Qt reads at startup: call before the QGuiApplication exists. On
/// desktop Linux it sets Qt's xcb platform to EGL; with the GL or DMABuf path it shares Qt's OpenGL contexts, and
/// with the DMABuf import under Vulkan it asks for the device extensions that takes, should Qt Quick render with
/// Vulkan. No-op otherwise.
void configureQtGraphics(bool softwareRendering);

/// What configureQtGraphics() was told: the user asked for software rendering (--swrast). Safe off the GUI thread.
bool softwareRenderingRequested() noexcept;

/// Wire the main QQuickWindow into the RHI-capture path so snapshots, and the EGLDisplay Qt renders with, follow its
/// QRhi. GUI thread; no-op without GPU.
void connectMainWindow(QQuickWindow* window);

/// Populate the per-pipeline HwVideoBufferContext (gpu-enabled flag, AHardwareBuffer EGL display) the factory needs;
/// the sink adds dmaBufImportDisplay() per frame. GPU stays disabled in builds without a GPU path. Safe off the GUI
/// thread.
HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept;

/// What the DMABuf path imports with now, as HwVideoBufferContext::dmaBufImporter stores it: Qt's EGLDisplay
/// (QGCRhiCapture::qtEglDisplay()), or, once Qt Quick renders with Vulkan, its VkPhysicalDevice when that device
/// imports dma-bufs. Null when Qt renders without either or that isn't known yet, an import failure disabled the path
/// (until resetCachedGpuResources()), or no DMABuf path is compiled. Safe off the GUI thread.
void* dmaBufImportDisplay() noexcept;

/// The DXGI adapter of the Direct3D device Qt Quick renders with, as the decoders' "adapter-luid" names theirs; 0
/// until the scene graph publishes that device, and for any other renderer. Safe off the GUI thread.
qint64 graphicsAdapterLuid() noexcept;

/// Has @p handler called each time the scene graph publishes the device Qt Quick renders with, on its render thread:
/// what depends on that device (the decoder ranks) follows it then. Null for none. No-op when no GPU path is compiled.
void setGraphicsDevicePublishedHandler(void (*handler)()) noexcept;

/// True when the Direct3D 11 device Qt Quick renders with is known to open a Direct3D 12 device's resources as
/// textures, which is how the D3D12 path shows a frame there. False until the scene graph publishes that device, and
/// in builds without both D3D paths. Safe off the GUI thread.
bool d3d11OpensD3D12() noexcept;

/// Synchronously answer GST_QUERY_CONTEXT through the GPU paths' context bridges; false -> let bus NEED_CONTEXT run.
/// @p sink, the element the query reached, names the pipeline that gets the device this way.
bool answerSinkBinContextQuery(GstQuery* query, GstElement* sink = nullptr) noexcept;

/// Add a buffer pool on Qt's D3D11/D3D12 device to an ALLOCATION query for caps in that memory, so the decoder
/// allocates on the device the importer samples from. False for any other memory, or before the device bridge is
/// primed.
bool proposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size,
                            guint minBuffers) noexcept;

/// Process-wide render-time counters no sink can attribute (map failures, texture reuse, producer waits, fallback
/// reasons, ...) accumulated since @p baseline, each printed only when nonzero. Reading never resets them, so every
/// receiver can report its own window.
QString extraPathStatsSince(const GstHwPathTelemetry::Snapshot& baseline);

}  // namespace HwBuffers
