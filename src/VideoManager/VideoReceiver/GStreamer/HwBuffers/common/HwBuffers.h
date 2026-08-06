#pragma once

/// Umbrella header / public facade for the HwBuffers subsystem; per-platform bridges stay internal.

#include <QtCore/QString>
#include <QtCore/qglobal.h>
#include <gst/gst.h>

class QQuickWindow;

// HwVideoBufferContext/Path and telemetry are unconditional (CPU path uses them); GstHwVideoBuffer needs
// MultimediaPrivate so it's GPU-only.
#include "GstHwPathTelemetry.h"
#include "GstHwVideoBufferFactory.h"  // HwVideoBufferContext, HwVideoBufferPath, makeHwVideoBuffer
#if defined(QGC_HAS_ANY_GPU_PATH)
#include "GstHwVideoBuffer.h"
#endif

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

/// Pipeline-restart hook; re-arms one-shot priming latches so a restart can prime on the next NEED_CONTEXT.
void onPipelineRestart() noexcept;

/// Drop process-wide native GPU handles tied to the current Qt scene-graph device/context.
void resetCachedGpuResources() noexcept;

/// Wire the main QQuickWindow into the RHI-capture path so snapshots follow its QRhi; no-op without GPU.
void connectMainWindow(QQuickWindow* window) noexcept;

#if defined(QGC_HAS_ANY_GPU_PATH)
/// Populate the per-pipeline HwVideoBufferContext (EGL displays, gpu-enabled flag) the factory needs.
HwVideoBufferContext makeAdapterContext(bool gpuEnabled) noexcept;
#endif

/// Synchronously answer GST_QUERY_CONTEXT (gst.gl.GLDisplay/app_context); false -> let bus NEED_CONTEXT run.
bool answerSinkBinContextQuery(GstQuery* query) noexcept;

/// Formatted per-path counters + delivered total; reset=true reads-and-clears (teardown), false peeks.
struct PathStats
{
    QString line;
    quint64 totalDelivered = 0;
};

PathStats formatPathStats(bool reset) noexcept;

/// Path-specific extras after formatPathStats (GL reuse/sync waits); reads-and-clears, teardown only.
QString takeExtraPathStats() noexcept;

}  // namespace HwBuffers
