#pragma once

#include <QtCore/qglobal.h>

#if defined(QGC_HAS_GST_DMABUF_GPU_PATH) || defined(QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH)

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

class QOpenGLContext;
class QRhi;

/// Shared EGL helpers for EGL-backed zero-copy paths (DMABuf, AHB): display resolution + extension cache.
namespace GstEglHelpers {

/// EGLDisplay bound to @p qtCtx (else current-thread display, else EGL_NO_DISPLAY); does NOT initialize it.
EGLDisplay resolveEglDisplay(QOpenGLContext* qtCtx) noexcept;

/// Makes @p rhi's GL context current on the calling render thread (EGL/GL calls otherwise no-op into a foreign or null
/// context) and returns it; nullptr when the QRhi exposes none.
QOpenGLContext* bindRhiContext(QRhi& rhi);

/// EGLDisplay Qt renders @p qtCtx on, else @p fallback. EGLImages from one display can't be sampled by a context on
/// another, so prefer this over a display captured elsewhere.
EGLDisplay renderDisplay(QOpenGLContext* qtCtx, EGLDisplay fallback) noexcept;

/// True iff @p extension is in @p display's EGL_EXTENSIONS; does NOT eglInitialize, cached per (display, extension),
/// thread-safe.
bool displaySupportsExtension(EGLDisplay display, const char* extension);

/// Clears the per-display extension cache; call from sceneGraph/context teardown.
void resetExtensionCache();

/// eglGetProcAddress() lookups, done once; null when the EGL/GL implementation lacks the function.
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC imageTargetTexture2D() noexcept;
PFNEGLDESTROYIMAGEKHRPROC destroyImageKHR() noexcept;

}  // namespace GstEglHelpers

#endif  // QGC_HAS_GST_DMABUF_GPU_PATH || QGC_HAS_GST_AHARDWAREBUFFER_GPU_PATH
