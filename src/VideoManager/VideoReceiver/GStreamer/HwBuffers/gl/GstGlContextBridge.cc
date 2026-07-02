#include "GstGlContextBridge.h"

#include "GstBridgePrimeRetry.h"
#include "GstContextBridgeCommon.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/qguiapplication_platform.h>
#include <gst/gl/gl.h>

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
// paths are silently missed.
#if __has_include(<gst/gl/wayland/gstgldisplay_wayland.h>)
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#define QGC_GST_BRIDGE_HAS_WAYLAND 1
#endif
#endif

QGC_LOGGING_CATEGORY(GstGlBridgeLog, "Video.GStreamer.HwBuffers.GstGlBridge")

namespace GstGlContextBridge {
namespace {

QMutex s_mutex;
GstGLDisplay* s_display = nullptr;
GstGLContext* s_context = nullptr;
// Bounds globalShareContext retry spam when Qt GL is never initialized.
GstBridgePrimeRetry::PrimeRetryState s_retry;

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

bool primeLocked()
{
    switch (GstBridgePrimeRetry::primeRetryGuard(s_retry)) {
        case GstBridgePrimeRetry::Decision::AlreadyPrimed:
            return true;
        case GstBridgePrimeRetry::Decision::GiveUp:
            return false;
        case GstBridgePrimeRetry::Decision::ShouldRetry:
            break;
    }

    QOpenGLContext* qtCtx = QOpenGLContext::globalShareContext();
    if (!qtCtx) {
        if (GstBridgePrimeRetry::rearmRetry(s_retry)) {
            qCInfo(GstGlBridgeLog) << "globalShareContext() is null — Qt GL not initialized yet"
                                   << "(attempt" << s_retry.nullCount << "/" << s_retry.maxRetries << ")";
        } else if (GstBridgePrimeRetry::justGaveUp(s_retry)) {
            qCWarning(GstGlBridgeLog) << "globalShareContext() still null after" << s_retry.maxRetries
                                      << "retries; GL bridge giving up";
        }
        return false;
    }

#if defined(QGC_GST_BRIDGE_HAS_EGL)
    // Try EGL first (Linux: Wayland/eglfs/xcb_egl; Windows: ANGLE-EGL); fall back to GLX (Linux xcb default on Qt 6).
    EGLContext eglCtx = qtEglContext(qtCtx);
    EGLDisplay eglDisp = (eglCtx != EGL_NO_CONTEXT) ? qtEglDisplay(qtCtx) : EGL_NO_DISPLAY;
    // Reset s_primeAttempted on each bail so a window-recreate or reset() gets a retry.
    auto bail = [](const char*) -> bool {
        s_retry.primeAttempted = false;
        return false;
    };
    if (eglCtx != EGL_NO_CONTEXT && eglDisp != EGL_NO_DISPLAY) {
#if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        // On Wayland, primary display must be GstGLDisplayWayland; derived EGL display is marked foreign so unref
        // doesn't tear down Qt's EGLDisplay.
        const QString platformName = QGuiApplication::platformName();
        if (platformName == QLatin1String("wayland") || platformName == QLatin1String("wayland-egl")) {
            struct wl_display* wlDisp = nullptr;
            if (auto* wl = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()) {
                wlDisp = wl->display();
            }
            if (wlDisp) {
                GstGLDisplayWayland* displayWl = gst_gl_display_wayland_new_with_display(wlDisp);
                if (displayWl) {
                    s_display = GST_GL_DISPLAY(displayWl);
#if GST_CHECK_VERSION(1, 26, 0)
                    // set_foreign(TRUE) is mandatory: Qt owns the EGLDisplay; without it gst calls eglTerminate on Qt's
                    // display.
                    if (GstGLDisplayEGL* derived = gst_gl_display_egl_from_gl_display(s_display)) {
                        gst_gl_display_egl_set_foreign(derived, TRUE);
                        gst_object_unref(derived);
                    }
#endif
                }
            }
        }
#endif
        if (!s_display) {
            GstGLDisplayEGL* displayEgl = gst_gl_display_egl_new_with_egl_display(eglDisp);
            if (!displayEgl) {
                qCWarning(GstGlBridgeLog) << "gst_gl_display_egl_new_with_egl_display failed";
                return bail("displayEgl");
            }
            s_display = GST_GL_DISPLAY(displayEgl);
        }

        s_context = gst_gl_context_new_wrapped(s_display, reinterpret_cast<guintptr>(eglCtx), GST_GL_PLATFORM_EGL,
                                               static_cast<GstGLAPI>(GST_GL_API_GLES2 | GST_GL_API_OPENGL));
        if (!s_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (EGL) failed";
            gst_clear_object(&s_display);
            return bail("ctxEgl");
        }
#if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        const bool isWayland = GST_IS_GL_DISPLAY_WAYLAND(s_display);
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
            return bail("noGlx");
        }
        Display* xdisp = nullptr;
        if (auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
            xdisp = x11->display();
        }
        if (!xdisp) {
            qCWarning(GstGlBridgeLog) << "X11 Display unresolvable; GL bridge disabled";
            return bail("xdisp");
        }
        GstGLDisplayX11* displayX11 = gst_gl_display_x11_new_with_display(xdisp);
        if (!displayX11) {
            qCWarning(GstGlBridgeLog) << "gst_gl_display_x11_new_with_display failed";
            return bail("displayX11");
        }
        s_display = GST_GL_DISPLAY(displayX11);
        s_context = gst_gl_context_new_wrapped(s_display, reinterpret_cast<guintptr>(glx->nativeContext()),
                                               GST_GL_PLATFORM_GLX, static_cast<GstGLAPI>(GST_GL_API_OPENGL));
        if (!s_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (GLX) failed";
            gst_clear_object(&s_display);
            return bail("ctxGlx");
        }
        qCInfo(GstGlBridgeLog) << "GL bridge primed (GLX)";
#else
        qCWarning(GstGlBridgeLog) << "Qt EGLContext unresolvable and GLX bridge not built; GL bridge disabled";
        return bail("noEglNoGlx");
#endif
    }

    s_retry.primed = true;
    // Don't call gst_gl_context_fill_info(): a freshly wrapped context has no active thread yet, and GStreamer fills
    // info lazily on first activation.
    qCDebug(GstGlBridgeLog) << "GL bridge primed: display=" << s_display << "context=" << s_context;
    return true;
#else
    // EGL-only by design; Windows uses the D3D11/D3D12 bridge, macOS/iOS use CVPixelBuffer. No WGL/CGL wrap: forcing
    // QSG_RHI_BACKEND=opengl on Windows (rare; RHI defaults to D3D11) just falls back to the CPU path here.
    qCInfo(GstGlBridgeLog) << "GL bridge inactive on this platform (non-EGL)";
    return false;
#endif
}

}  // namespace

namespace {

constexpr char kGlAppContextType[] = "gst.gl.app_context";
const char* const kContextTypes[] = {GST_GL_DISPLAY_CONTEXT_TYPE, kGlAppContextType};

const QLoggingCategory& vtCat(void*)
{
    return GstGlBridgeLog();
}

QMutex& vtMutex(void*)
{
    return s_mutex;
}

bool vtPrime(void*)
{
    return primeLocked();
}

GstObject* vtRefObject(void*, const char* contextType)
{
    if (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
        return s_display ? GST_OBJECT(gst_object_ref(s_display)) : nullptr;
    }
    return s_context ? GST_OBJECT(gst_object_ref(s_context)) : nullptr;
}

GstContext* vtBuildContext(void*, const char* contextType, GstObject* object)
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

const GstContextBridge::BridgeVTable s_vtable = {
    "GL", kContextTypes, 2, &vtCat, &vtMutex, &vtPrime, &vtRefObject, &vtBuildContext, nullptr,
};

}  // namespace

bool prime()
{
    QMutexLocker lock(&s_mutex);
    return primeLocked();
}

GstBusSyncReply handleSyncMessage(GstMessage* message)
{
    return GstContextBridge::handleSyncMessage(s_vtable, nullptr, message);
}

bool answerContextQuery(GstQuery* query)
{
    return GstContextBridge::answerContextQuery(s_vtable, nullptr, query);
}

void reset()
{
    QMutexLocker lock(&s_mutex);
    gst_clear_object(&s_context);
    gst_clear_object(&s_display);
    GstBridgePrimeRetry::resetRetry(s_retry);
    qCDebug(GstGlBridgeLog) << "GL bridge reset";
}

void rearm()
{
    QMutexLocker lock(&s_mutex);
    if (GstBridgePrimeRetry::rearmAfterExhaustion(s_retry)) {
        qCInfo(GstGlBridgeLog) << "GL bridge rearm: clearing exhausted retry latch";
    }
}

namespace {
struct GlBridgeRegistrar
{
    GlBridgeRegistrar()
    {
        GstContextBridge::registerBridge(GstGlBridgeLog(), "GL", &GstGlContextBridge::handleSyncMessage,
                                         &GstGlContextBridge::reset);
    }
};

static GlBridgeRegistrar s_glBridgeRegistrar;
}  // anonymous namespace

}  // namespace GstGlContextBridge

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
