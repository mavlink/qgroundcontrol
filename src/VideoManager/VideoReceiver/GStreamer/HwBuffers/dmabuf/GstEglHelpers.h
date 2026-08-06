#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <EGL/egl.h>

class QOpenGLContext;

/// Shared EGL helpers for EGL-backed zero-copy paths (DMABuf, AHB): display resolution + extension cache.
namespace GstEglHelpers {

/// EGLDisplay bound to @p qtCtx (else current-thread display, else EGL_NO_DISPLAY); does NOT initialize it.
EGLDisplay resolveEglDisplay(QOpenGLContext* qtCtx) noexcept;

/// True iff @p extension is in @p display's EGL_EXTENSIONS; does NOT eglInitialize, cached per (display, extension),
/// thread-safe.
bool displaySupportsExtension(EGLDisplay display, const char* extension);

/// Clears the per-display extension cache; call from sceneGraph/context teardown.
void resetExtensionCache();

}  // namespace GstEglHelpers

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH || QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
