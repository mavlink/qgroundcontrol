#pragma once

class QQuickWindow;

/// Low-level RHI / scene-graph configuration for the main QQuickWindow.
/// Call before the first scene graph initialization.
/// Environment variables:
///   QGC_RHI_DEBUG           - enable RHI debug layer, debug markers and GPU timestamps
///   QGC_RHI_PIPELINE_CACHE  - "0" disables the on-disk pipeline cache (default on)
///   QGC_HDR_OUTPUT          - request HDR display output (diagnostic; see note in .cc)
///   QGC_FORCE_VIDEO_GPU     - "low:high" LUID hex pair to force a specific D3D adapter (Windows only)
namespace GraphicsSetup {

void configureMainWindow(QQuickWindow* window);

}  // namespace GraphicsSetup
