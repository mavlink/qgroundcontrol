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

/// Head of qgcvideosinkbin's GPU branch. Linux builds with both gst-gl and DMABuf compile GlUpload and DirectDmaBuf
/// and pick one at runtime; every other build compiles exactly one.
enum class GpuBinMode
{
    Passthrough,   ///< Every compiled native memory straight into qgcqvideosink (Windows, Apple, ...).
    GlUpload,      ///< glupload → glcolorconvert → GLMemory 2D; gst-gl imports LINEAR and vendor-tiled DMABuf.
    DirectDmaBuf,  ///< memory:DMABuf and DMA_DRM into QGC's own importer (LINEAR only), skipping gst-gl's copy.
};

/// Resolves QGC_GST_GPU_BIN ("glupload" or "dmabuf", case-insensitive) against the compiled bins. Unset keeps the
/// default, GlUpload; an unknown value warns and keeps it too. With one compiled bin the variable is ignored.
GpuBinMode readGpuBinMode() noexcept;

/// readGpuBinMode() of the first call, logged once with the reason for the choice.
GpuBinMode gpuBinMode() noexcept;

#ifdef QGC_UNITTEST_BUILD
/// gpuBinMode() returns @p mode while alive, then what it returned before.
class ScopedGpuBinModeForTest
{
public:
    explicit ScopedGpuBinModeForTest(GpuBinMode mode);
    ~ScopedGpuBinModeForTest();
    ScopedGpuBinModeForTest(const ScopedGpuBinModeForTest&) = delete;
    ScopedGpuBinModeForTest& operator=(const ScopedGpuBinModeForTest&) = delete;

private:
    int _previous;
};
#endif

/// For a receiver's bus sync handler: the context bridges answer NEED_CONTEXT. GST_BUS_DROP means @p msg was consumed
/// (and unreffed); always GST_BUS_PASS when no GPU path is compiled.
GstBusSyncReply onBusSyncMessage(GstMessage* msg) noexcept;

/// For a receiver's pipeline ERROR (@p error and @p debug as gst_message_parse_error() returns them): drops cached GPU
/// devices when it reports a device loss. No-op when no GPU paths compiled.
void onPipelineError(const GError* error, const gchar* debug) noexcept;

/// Pipeline-restart hook; clears exhausted prime-retry latches (GL, Vulkan) so a restart can prime on the next
/// NEED_CONTEXT. Never drops shared GPU objects: other streams may still be using them.
void onPipelineRestart() noexcept;

/// Drop process-wide native GPU handles tied to the current Qt scene-graph device/context. Affects every running
/// stream, so only for scene-graph teardown, window swaps and device loss.
void resetCachedGpuResources() noexcept;

/// Wire the main QQuickWindow into the RHI-capture path so snapshots follow its QRhi, and resolve the EGLDisplay Qt
/// renders with for the DMABuf path. GUI thread; no-op without GPU.
void connectMainWindow(QQuickWindow* window) noexcept;

/// Populate the per-pipeline HwVideoBufferContext (EGL displays, gpu-enabled flag) the factory needs. GPU stays
/// disabled in builds without a GPU path. Safe off the GUI thread.
HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept;

/// Synchronously answer GST_QUERY_CONTEXT through the GPU paths' context bridges; false -> let bus NEED_CONTEXT run.
bool answerSinkBinContextQuery(GstQuery* query) noexcept;

/// Add a buffer pool on Qt's D3D11/D3D12 device to an ALLOCATION query for caps in that memory, so the decoder
/// allocates on the device the importer samples from. False for any other memory, or before the device bridge is
/// primed.
bool proposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size,
                            guint minBuffers) noexcept;

/// Process-wide render-time counters no sink can attribute (map failures, texture reuse, producer waits, fallback
/// reasons, ...) accumulated since @p baseline, each printed only when nonzero. Reading never resets them, so every
/// receiver can report its own window.
QString extraPathStatsSince(const GstHwPathTelemetry::Snapshot& baseline) noexcept;

/// extraPathStatsSince() the calling thread's previous call (process start on its first), then moves that thread's
/// baseline to now. Each receiver tears down on its own worker thread, so receivers never take each other's counts.
QString takeExtraPathStats() noexcept;

}  // namespace HwBuffers
