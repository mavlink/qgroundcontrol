#include "GstGlContextBridge.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gl/gl.h>

#include <QtCore/QLoggingCategory>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/qguiapplication_platform.h>

#include "QGCLoggingCategory.h"
// Shared by Linux and Windows-ANGLE (ANGLE ships EGL/egl.h); on Windows it's a fallback behind D3D11/D3D12.
// GLX/Wayland/X11 wrapping stays Linux-only below.
#if defined(__linux__) || (defined(_WIN32) && __has_include(<EGL/egl.h>))
#include <EGL/egl.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#define QGC_GST_BRIDGE_HAS_EGL 1
#endif
#if defined(__linux__)
// Qt 6 on xcb uses GLX by default — fall back to GLX context wrapping when EGL isn't available.
#if __has_include(<gst/gl/x11/gstgldisplay_x11.h>) && __has_include(<QtGui/qopenglcontext_platform.h>)
#include <QtGui/qopenglcontext_platform.h>
#include <X11/Xlib.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
#define QGC_GST_BRIDGE_HAS_GLX 1
#endif
// Wayland: downstream elements probe GST_IS_GL_DISPLAY_WAYLAND; wl_display must be tagged on GstGLDisplay or zero-copy
// paths are silently missed. Only from 1.26, where the EGL display gst derives from it can be marked foreign.
#if __has_include(<gst/gl/wayland/gstgldisplay_wayland.h>) && GST_CHECK_VERSION(1, 26, 0)
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#define QGC_GST_BRIDGE_HAS_WAYLAND 1
#endif
#endif

QGC_LOGGING_CATEGORY(GstGlBridgeLog, "Video.GStreamer.HwBuffers.GstGlBridge")

namespace {

#if defined(QGC_GST_BRIDGE_HAS_EGL)
EGLDisplay qtEglDisplay(QOpenGLContext* qtCtx)
{
    // QEGLContext::display() is the only handle that guarantees EGLImage import/sample compatibility; fall back to
    // EGL_DEFAULT_DISPLAY when unavailable.
    if (qtCtx) {
        if (auto* egl = qtCtx->nativeInterface<QNativeInterface::QEGLContext>()) {
            EGLDisplay d = egl->display();
            if (d != EGL_NO_DISPLAY)
                return d;
        }
    }
    return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

EGLContext qtEglContext(QOpenGLContext* qtCtx)
{
    if (!qtCtx)
        return EGL_NO_CONTEXT;
    if (auto* egl = qtCtx->nativeInterface<QNativeInterface::QEGLContext>()) {
        return egl->nativeContext();
    }
    return EGL_NO_CONTEXT;
}
#endif

constexpr char kGlAppContextType[] = "gst.gl.app_context";
constexpr const char* kContextTypes[] = {GST_GL_DISPLAY_CONTEXT_TYPE, kGlAppContextType};

class GlContextBridge final : public GstContextBridgeBase
{
public:
    constexpr GlContextBridge() noexcept
        : GstContextBridgeBase("GL", kContextTypes, &GstGlBridgeLog)
    {}

private:
    bool primeLocked() override;
    GstObject* refObject(const char* contextType) override;
    GstContext* buildContext(const char* contextType, GstObject* object) override;
    void clearLocked() override;

    GstGLDisplay* _display = nullptr;
    GstGLContext* _context = nullptr;
};

constinit GlContextBridge s_bridge;

bool GlContextBridge::primeLocked()
{
    QOpenGLContext* qtCtx = QOpenGLContext::globalShareContext();
    if (!qtCtx) {
        if (retryWithinBudget()) {
            qCInfo(GstGlBridgeLog) << "globalShareContext() is null — Qt GL not initialized yet"
                                   << "(attempt" << failedPrimes() << "/" << kMaxPrimeRetries << ")";
        } else if (retriesJustExhausted()) {
            qCWarning(GstGlBridgeLog) << "globalShareContext() still null after" << kMaxPrimeRetries
                                      << "retries; GL bridge giving up";
        }
        return false;
    }

#if defined(QGC_GST_BRIDGE_HAS_EGL)
    // Try EGL first (Linux: Wayland/eglfs/xcb_egl; Windows: ANGLE-EGL); fall back to GLX (Linux xcb default on Qt 6).
    EGLContext eglCtx = qtEglContext(qtCtx);
    EGLDisplay eglDisp = (eglCtx != EGL_NO_CONTEXT) ? qtEglDisplay(qtCtx) : EGL_NO_DISPLAY;
    if (eglCtx != EGL_NO_CONTEXT && eglDisp != EGL_NO_DISPLAY) {
#if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        // On Wayland, primary display must be GstGLDisplayWayland; derived EGL display is marked foreign so unref
        // doesn't tear down Qt's EGLDisplay. Before 1.26 that derived display can't be marked foreign and its finalize
        // eglTerminate()s Qt's display, so older GStreamer takes the foreign EGL wrap below instead.
        const QString platformName = QGuiApplication::platformName();
        if (platformName == QLatin1String("wayland") || platformName == QLatin1String("wayland-egl")) {
            struct wl_display* wlDisp = nullptr;
            if (auto* wl = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
                wlDisp = wl->display();
            }
            if (wlDisp) {
                GstGLDisplayWayland* displayWl = gst_gl_display_wayland_new_with_display(wlDisp);
                if (displayWl) {
                    _display = GST_GL_DISPLAY(displayWl);
                    if (const std::unique_ptr<GstGLDisplayEGL, GStreamer::GstObjectDeleter> derived{
                            gst_gl_display_egl_from_gl_display(_display)}) {
                        gst_gl_display_egl_set_foreign(derived.get(), TRUE);
                    }
                }
            }
        }
#endif
        if (!_display) {
            // Marked foreign: finalize never eglTerminate()s Qt's EGLDisplay, and gst_gl_display_egl_from_gl_display()
            // returns this same object rather than deriving a non-foreign one.
            GstGLDisplayEGL* displayEgl = gst_gl_display_egl_new_with_egl_display(eglDisp);
            if (!displayEgl) {
                qCWarning(GstGlBridgeLog) << "gst_gl_display_egl_new_with_egl_display failed";
                return retryLater();
            }
            _display = GST_GL_DISPLAY(displayEgl);
        }

        _context = gst_gl_context_new_wrapped(_display, reinterpret_cast<guintptr>(eglCtx), GST_GL_PLATFORM_EGL,
                                              static_cast<GstGLAPI>(GST_GL_API_GLES2 | GST_GL_API_OPENGL));
        if (!_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (EGL) failed";
            gst_clear_object(&_display);
            return retryLater();
        }
#if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        const bool isWayland = GST_IS_GL_DISPLAY_WAYLAND(_display);
        qCInfo(GstGlBridgeLog) << (isWayland ? "GL bridge primed (Wayland+EGL)" : "GL bridge primed (EGL)");
#else
        qCInfo(GstGlBridgeLog) << "GL bridge primed (EGL)";
#endif
    } else {
#if defined(QGC_GST_BRIDGE_HAS_GLX)
        // Qt's QGLXContext exposes the X11 Display* + GLXContext we need to wrap.
        auto* glx = qtCtx->nativeInterface<QNativeInterface::QGLXContext>();
        if (!glx) {
            qCWarning(GstGlBridgeLog) << "Qt GL context exposes neither EGL nor GLX; GL bridge disabled";
            return retryLater();
        }
        Display* xdisp = nullptr;
        if (auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
            xdisp = x11->display();
        }
        if (!xdisp) {
            qCWarning(GstGlBridgeLog) << "X11 Display unresolvable; GL bridge disabled";
            return retryLater();
        }
        GstGLDisplayX11* displayX11 = gst_gl_display_x11_new_with_display(xdisp);
        if (!displayX11) {
            qCWarning(GstGlBridgeLog) << "gst_gl_display_x11_new_with_display failed";
            return retryLater();
        }
        _display = GST_GL_DISPLAY(displayX11);
        _context = gst_gl_context_new_wrapped(_display, reinterpret_cast<guintptr>(glx->nativeContext()),
                                              GST_GL_PLATFORM_GLX, static_cast<GstGLAPI>(GST_GL_API_OPENGL));
        if (!_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (GLX) failed";
            gst_clear_object(&_display);
            return retryLater();
        }
        qCInfo(GstGlBridgeLog) << "GL bridge primed (GLX)";
#else
        qCWarning(GstGlBridgeLog) << "Qt EGLContext unresolvable and GLX bridge not built; GL bridge disabled";
        return retryLater();
#endif
    }

    // Don't call gst_gl_context_fill_info(): a freshly wrapped context has no active thread yet, and GStreamer fills
    // info lazily on first activation.
    qCDebug(GstGlBridgeLog) << "GL bridge primed: display=" << _display << "context=" << _context;
    return true;
#else
    // EGL-only by design; Windows uses the D3D11/D3D12 bridge, macOS/iOS use CVPixelBuffer. No WGL/CGL wrap: forcing
    // QSG_RHI_BACKEND=opengl on Windows (rare; RHI defaults to D3D11) just falls back to the CPU path here.
    qCInfo(GstGlBridgeLog) << "GL bridge inactive on this platform (non-EGL)";
    return false;
#endif
}

GstObject* GlContextBridge::refObject(const char* contextType)
{
    if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
        return _display ? GST_OBJECT(gst_object_ref(_display)) : nullptr;
    }
    return _context ? GST_OBJECT(gst_object_ref(_context)) : nullptr;
}

GstContext* GlContextBridge::buildContext(const char* contextType, GstObject* object)
{
    if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
        GstContext* ctx = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        gst_context_set_gl_display(ctx, GST_GL_DISPLAY(object));
        return ctx;
    }
    GstContext* ctx = gst_context_new(kGlAppContextType, TRUE);
    GstStructure* s = gst_context_writable_structure(ctx);
    gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, GST_GL_CONTEXT(object), NULL);
    return ctx;
}

void GlContextBridge::clearLocked()
{
    gst_clear_object(&_context);
    gst_clear_object(&_display);
}

}  // namespace

GstContextBridgeBase& GstGlContextBridge::instance()
{
    return s_bridge;
}

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
