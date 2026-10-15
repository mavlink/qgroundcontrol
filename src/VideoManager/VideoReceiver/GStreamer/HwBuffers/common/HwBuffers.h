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

/// One-time process init; single call site for future lazy bridge registration.
void initializeOnce() noexcept;

/// Process-wide QGC_GST_* runtime toggles, parsed once and logged at first access so operators can paste the
/// effective config into bug reports. Semantics match the historical per-call-site g_getenv reads.
struct HwBufferEnvConfig
{
    bool dmaBufCache = false;           // QGC_GST_DMABUF_CACHE        (default off)
    bool dmaBufSingleEglImage = true;   // QGC_GST_DMABUF_SINGLE_EGLIMAGE (default on)
    bool dmaBufNoMmapFence = false;     // QGC_GST_DMABUF_NO_MMAP_FENCE (default off)
    bool offerDmaDrmLinear = false;     // QGC_GST_OFFER_DMA_DRM_LINEAR (default off)
};

/// Lazily parses + logs the toggle config on first call; thread-safe via static-init guarantees.
const HwBufferEnvConfig& hwBufferEnvConfig() noexcept;

/// Bus sync handler (GstBusSyncHandler) installed on every pipeline; no-op when no GPU path compiled.
GstBusSyncReply onBusSyncMessage(GstBus* bus, GstMessage* msg, gpointer userData) noexcept;

/// Receiver-side bus hook; drops cached GPU devices on GST_MESSAGE_ERROR. No-op when no GPU paths compiled.
void dispatchBusMessage(GstMessage* msg) noexcept;

/// Pipeline-restart hook; clears exhausted prime-retry latches (GL, Vulkan) so a restart can prime on the next
/// NEED_CONTEXT. Never drops shared GPU objects: other streams may still be using them.
void onPipelineRestart() noexcept;

/// Drop process-wide native GPU handles tied to the current Qt scene-graph device/context. Affects every running
/// stream, so only for scene-graph teardown, window swaps and device loss.
void resetCachedGpuResources() noexcept;

/// Wire the main QQuickWindow into the RHI-capture path so snapshots follow its QRhi; no-op without GPU.
void connectMainWindow(QQuickWindow* window) noexcept;

/// Populate the per-pipeline HwVideoBufferContext (EGL displays, gpu-enabled flag) the factory needs. GPU stays
/// disabled in builds without a GPU path. Safe off the GUI thread.
HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept;

/// Synchronously answer GST_QUERY_CONTEXT (gst.gl.GLDisplay/app_context); false -> let bus NEED_CONTEXT run.
bool answerSinkBinContextQuery(GstQuery* query) noexcept;

/// Add a buffer pool on Qt's D3D11/D3D12 device to an ALLOCATION query for caps in that memory, so the decoder
/// allocates on the device the importer samples from. False for any other memory, or before the device bridge is
/// primed.
bool proposeDeviceBoundPool(GstQuery* query, GstCaps* caps, const GstVideoInfo* vinfo, gsize size,
                            guint minBuffers) noexcept;

/// Formatted per-path counters + delivered total; reset=true reads-and-clears (teardown), false peeks.
struct PathStats
{
    QString line;
    quint64 totalDelivered = 0;
};

PathStats formatPathStats(bool reset) noexcept;

/// formatPathStats() for a single qgcqvideosink, from its "stats" structure: the per-path counts it has delivered,
/// plus its GPU fallbacks and demotions. totalDelivered counts GPU-path frames, as in formatPathStats().
PathStats formatSinkStats(const GstStructure* stats) noexcept;

/// Path-specific extras after formatPathStats (GL reuse/sync waits); reads-and-clears, teardown only.
QString takeExtraPathStats() noexcept;

}  // namespace HwBuffers
