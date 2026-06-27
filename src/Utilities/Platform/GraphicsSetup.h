#pragma once

class QQuickWindow;

/// Low-level RHI / scene-graph configuration for the main QQuickWindow.
///
/// All of this MUST be applied before the window's scene graph initializes for the first time
/// (before first expose). Call configureMainWindow() synchronously right after the root window
/// object is created and before the event loop resumes — at that point the QQuickWindow exists but
/// the SG is not yet live. The diagnostic HDR probe is deferred to a render job because it needs a
/// valid QRhi, which only exists after sceneGraphInitialized.
///
/// Everything here is OFF by default and gated behind environment variables (except the low-risk
/// pipeline cache, which is ON unless QGC_RHI_PIPELINE_CACHE=0):
///   QGC_RHI_DEBUG           - enable RHI debug layer, debug markers and GPU timestamps
///   QGC_RHI_PIPELINE_CACHE  - "0" disables the on-disk pipeline cache (default on)
///   QGC_HDR_OUTPUT          - request HDR display output (diagnostic; see note in .cc)
///   QGC_FORCE_VIDEO_GPU     - "low:high" LUID hex pair to force a specific D3D adapter (Windows only)
namespace GraphicsSetup {

/// Apply pre-scene-graph-init configuration (graphics config + optional forced device) and schedule
/// the deferred HDR diagnostic probe. No-op with a warning if the SG is already initialized.
void configureMainWindow(QQuickWindow* window);

}  // namespace GraphicsSetup
