#include "GstGlContextBridge.h"
#include "GstContextBridgeRegistry.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include "QGCLoggingCategory.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <qpa/qplatformnativeinterface.h>

#include <gst/gl/gl.h>
#if defined(__linux__)
#  include <gst/gl/egl/gstgldisplay_egl.h>
#  include <EGL/egl.h>
// Qt 6 on xcb uses GLX by default — fall back to GLX context wrapping when EGL isn't available.
#  if __has_include(<gst/gl/x11/gstgldisplay_x11.h>) && __has_include(<QtGui/qopenglcontext_platform.h>)
#    include <gst/gl/x11/gstgldisplay_x11.h>
#    include <QtGui/qopenglcontext_platform.h>
#    include <X11/Xlib.h>
#    define QGC_GST_BRIDGE_HAS_GLX 1
#  endif
// Wayland: downstream elements that probe GST_IS_GL_DISPLAY_WAYLAND need the wl_display tagged on the GstGLDisplay; without it pool/queue negotiation falls back to a generic EGL display and silently misses Wayland-specific zero-copy paths.
#  if __has_include(<gst/gl/wayland/gstgldisplay_wayland.h>)
#    include <gst/gl/wayland/gstgldisplay_wayland.h>
#    define QGC_GST_BRIDGE_HAS_WAYLAND 1
#  endif
#endif

QGC_LOGGING_CATEGORY(GstGlBridgeLog, "Video.GStreamer.HwBuffers.GstGlBridge")

namespace GstGlContextBridge {
namespace {

QMutex s_mutex;
GstGLDisplay *s_display = nullptr;
GstGLContext *s_context = nullptr;
bool s_primed = false;
bool s_primeAttempted = false;
// Bounds globalShareContext retry spam when Qt GL is never initialized.
int s_primeNullShareCount = 0;
constexpr int kMaxPrimeNullShareRetries = 16;

#if defined(__linux__)
EGLDisplay qtEglDisplay()
{
    if (auto *ni = QGuiApplication::platformNativeInterface()) {
        if (auto *d = ni->nativeResourceForIntegration("egldisplay")) {
            return static_cast<EGLDisplay>(d);
        }
    }
    return eglGetDisplay(EGL_DEFAULT_DISPLAY);
}

EGLContext qtEglContext(QOpenGLContext *qtCtx)
{
    if (!qtCtx) return EGL_NO_CONTEXT;
    if (auto *egl = qtCtx->nativeInterface<QNativeInterface::QEGLContext>()) {
        return egl->nativeContext();
    }
    return EGL_NO_CONTEXT;
}
#endif

bool primeLocked()
{
    if (s_primed) return true;
    if (s_primeAttempted) return false;
    s_primeAttempted = true;

    QOpenGLContext *qtCtx = QOpenGLContext::globalShareContext();
    if (!qtCtx) {
        ++s_primeNullShareCount;
        if (s_primeNullShareCount <= kMaxPrimeNullShareRetries) {
            qCInfo(GstGlBridgeLog) << "globalShareContext() is null — Qt GL not initialized yet"
                                   << "(attempt" << s_primeNullShareCount << "/" << kMaxPrimeNullShareRetries << ")";
            s_primeAttempted = false; // allow retry until limit is reached
        } else {
            if (s_primeNullShareCount == kMaxPrimeNullShareRetries + 1) {
                qCWarning(GstGlBridgeLog) << "globalShareContext() still null after"
                                          << kMaxPrimeNullShareRetries
                                          << "retries; GL bridge giving up";
            }
            // s_primeAttempted stays true — no more retries until reset()
        }
        return false;
    }

#if defined(__linux__)
    // Try EGL first (Wayland, eglfs, xcb_egl); fall back to GLX (xcb default on Qt 6 desktop).
    EGLContext eglCtx = qtEglContext(qtCtx);
    EGLDisplay eglDisp = (eglCtx != EGL_NO_CONTEXT) ? qtEglDisplay() : EGL_NO_DISPLAY;
    // Allow retry on every "real" failure below — these are likely permanent (misconfigured
     // gst build, missing native interface) but if a window-recreate fixes things the next
     // NEED_CONTEXT should get another chance. reset() also re-enables retry by clearing
     // s_primeAttempted directly.
    auto bail = [](const char *) -> bool { s_primeAttempted = false; return false; };
    if (eglCtx != EGL_NO_CONTEXT && eglDisp != EGL_NO_DISPLAY) {
#  if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        // On Wayland, primary display must be GstGLDisplayWayland (carries wl_display); the EGL
        // display is then derived and marked foreign so unref doesn't tear down the EGL handle Qt owns.
        const QString platformName = QGuiApplication::platformName();
        if (platformName == QLatin1String("wayland") || platformName == QLatin1String("wayland-egl")) {
            struct wl_display *wlDisp = nullptr;
            if (auto *ni = QGuiApplication::platformNativeInterface()) {
                wlDisp = static_cast<struct wl_display *>(ni->nativeResourceForIntegration("wl_display"));
            }
            if (wlDisp) {
                GstGLDisplayWayland *displayWl = gst_gl_display_wayland_new_with_display(wlDisp);
                if (displayWl) {
                    s_display = GST_GL_DISPLAY(displayWl);
#    if GST_CHECK_VERSION(1, 26, 0)
                    // Pre-derive the EGL view so downstream NEED_CONTEXT for gst.gl.display.egl is satisfied
                    // by a display that maps back to the same wl_display. set_foreign(TRUE) is mandatory:
                    // Qt owns the EGLDisplay; without it gst would call eglTerminate on Qt's display.
                    if (GstGLDisplayEGL *derived = gst_gl_display_egl_from_gl_display(s_display)) {
                        gst_gl_display_egl_set_foreign(derived, TRUE);
                        gst_object_unref(derived);
                    }
#    endif
                }
            }
        }
#  endif
        if (!s_display) {
            GstGLDisplayEGL *displayEgl = gst_gl_display_egl_new_with_egl_display(eglDisp);
            if (!displayEgl) {
                qCWarning(GstGlBridgeLog) << "gst_gl_display_egl_new_with_egl_display failed";
                return bail("displayEgl");
            }
            s_display = GST_GL_DISPLAY(displayEgl);
        }

        s_context = gst_gl_context_new_wrapped(s_display,
                                               reinterpret_cast<guintptr>(eglCtx),
                                               GST_GL_PLATFORM_EGL,
                                               static_cast<GstGLAPI>(GST_GL_API_GLES2 | GST_GL_API_OPENGL));
        if (!s_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (EGL) failed";
            gst_clear_object(&s_display);
            return bail("ctxEgl");
        }
#  if defined(QGC_GST_BRIDGE_HAS_WAYLAND)
        const bool isWayland = GST_IS_GL_DISPLAY_WAYLAND(s_display);
        qCInfo(GstGlBridgeLog) << (isWayland ? "GL bridge primed (Wayland+EGL)" : "GL bridge primed (EGL)");
#  else
        qCInfo(GstGlBridgeLog) << "GL bridge primed (EGL)";
#  endif
    } else {
#  if defined(QGC_GST_BRIDGE_HAS_GLX)
        // Qt's QGLXContext exposes the X11 Display* + GLXContext we need to wrap.
        auto *glx = qtCtx->nativeInterface<QNativeInterface::QGLXContext>();
        if (!glx) {
            qCWarning(GstGlBridgeLog) << "Qt GL context exposes neither EGL nor GLX; GL bridge disabled";
            return bail("noGlx");
        }
        Display *xdisp = nullptr;
        if (auto *ni = QGuiApplication::platformNativeInterface()) {
            xdisp = static_cast<Display *>(ni->nativeResourceForIntegration("display"));
        }
        if (!xdisp) {
            qCWarning(GstGlBridgeLog) << "X11 Display unresolvable; GL bridge disabled";
            return bail("xdisp");
        }
        GstGLDisplayX11 *displayX11 = gst_gl_display_x11_new_with_display(xdisp);
        if (!displayX11) {
            qCWarning(GstGlBridgeLog) << "gst_gl_display_x11_new_with_display failed";
            return bail("displayX11");
        }
        s_display = GST_GL_DISPLAY(displayX11);
        s_context = gst_gl_context_new_wrapped(s_display,
                                               reinterpret_cast<guintptr>(glx->nativeContext()),
                                               GST_GL_PLATFORM_GLX,
                                               static_cast<GstGLAPI>(GST_GL_API_OPENGL));
        if (!s_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (GLX) failed";
            gst_clear_object(&s_display);
            return bail("ctxGlx");
        }
        qCInfo(GstGlBridgeLog) << "GL bridge primed (GLX)";
#  else
        qCWarning(GstGlBridgeLog) << "Qt EGLContext unresolvable and GLX bridge not built; GL bridge disabled";
        return bail("noEglNoGlx");
#  endif
    }

    s_primed = true;
    // gst_gl_context_fill_info() must run on the context's own thread, but a freshly wrapped context has no active thread until first activation — calling thread_add now trips an assertion. GStreamer fills info lazily on first use, so don't pre-fill.
    qCDebug(GstGlBridgeLog) << "GL bridge primed: display=" << s_display
                            << "context=" << s_context;
    return true;
#else
    // EGL-only by design. Windows uses GstD3D11ContextBridge; macOS/iOS use the
    // CVPixelBuffer path. WGL/CGL wrappers add no value over those.
    qCInfo(GstGlBridgeLog) << "GL bridge inactive on this platform (non-EGL)";
    return false;
#endif
}

} // namespace

bool prime()
{
    QMutexLocker lock(&s_mutex);
    return primeLocked();
}

GstBusSyncReply handleSyncMessage(GstMessage *message)
{
    if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_NEED_CONTEXT) {
        return GST_BUS_PASS;
    }

    const gchar *contextType = nullptr;
    if (!gst_message_parse_context_type(message, &contextType) || !contextType) {
        return GST_BUS_PASS;
    }
    const bool isGlDisplay = (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0);
    const bool isGlApp     = (g_strcmp0(contextType, "gst.gl.app_context") == 0);
    if (!isGlDisplay && !isGlApp) {
        return GST_BUS_PASS;
    }

    QMutexLocker lock(&s_mutex);
    if (!primeLocked()) {
        return GST_BUS_PASS;
    }

    GstElement *element = GST_ELEMENT(GST_MESSAGE_SRC(message));
    if (!element) {
        return GST_BUS_PASS;
    }

    if (isGlDisplay && s_display) {
        GstContext *gctx = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        gst_context_set_gl_display(gctx, s_display);
        gst_element_set_context(element, gctx);
        gst_context_unref(gctx);
        qCDebug(GstGlBridgeLog) << "Provided GL display context to" << GST_ELEMENT_NAME(element);
    } else if (isGlApp && s_context) {
        GstContext *gctx = gst_context_new("gst.gl.app_context", TRUE);
        GstStructure *s = gst_context_writable_structure(gctx);
        gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, s_context, NULL);
        gst_element_set_context(element, gctx);
        gst_context_unref(gctx);
        qCDebug(GstGlBridgeLog) << "Provided GL app context to" << GST_ELEMENT_NAME(element);
    } else {
        return GST_BUS_PASS;
    }

    gst_message_unref(message);
    return GST_BUS_DROP;
}

bool answerContextQuery(GstQuery *query)
{
    if (!query || GST_QUERY_TYPE(query) != GST_QUERY_CONTEXT) {
        return false;
    }
    const gchar *contextType = nullptr;
    if (!gst_query_parse_context_type(query, &contextType) || !contextType) {
        return false;
    }
    const bool isGlDisplay = (g_strcmp0(contextType, GST_GL_DISPLAY_CONTEXT_TYPE) == 0);
    const bool isGlApp     = (g_strcmp0(contextType, "gst.gl.app_context") == 0);
    if (!isGlDisplay && !isGlApp) {
        return false;
    }

    QMutexLocker lock(&s_mutex);
    if (!primeLocked()) {
        return false;
    }

    if (isGlDisplay && s_display) {
        GstContext *gctx = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
        gst_context_set_gl_display(gctx, s_display);
        gst_query_set_context(query, gctx);
        gst_context_unref(gctx);
        return true;
    }
    if (isGlApp && s_context) {
        GstContext *gctx = gst_context_new("gst.gl.app_context", TRUE);
        GstStructure *s = gst_context_writable_structure(gctx);
        gst_structure_set(s, "context", GST_TYPE_GL_CONTEXT, s_context, NULL);
        gst_query_set_context(query, gctx);
        gst_context_unref(gctx);
        return true;
    }
    return false;
}

void reset()
{
    QMutexLocker lock(&s_mutex);
    gst_clear_object(&s_context);
    gst_clear_object(&s_display);
    s_primed = false;
    s_primeAttempted = false;
    s_primeNullShareCount = 0;
    qCDebug(GstGlBridgeLog) << "GL bridge reset";
}

void rearm()
{
    QMutexLocker lock(&s_mutex);
    if (s_primed) return;
    if (s_primeAttempted && s_primeNullShareCount > kMaxPrimeNullShareRetries) {
        s_primeAttempted = false;
        s_primeNullShareCount = 0;
        qCInfo(GstGlBridgeLog) << "GL bridge rearm: clearing exhausted retry latch";
    }
}


namespace {
struct GlBridgeRegistrar {
    GlBridgeRegistrar() {
        GstContextBridgeRegistry::registerBridgeHandler(&GstGlContextBridge::handleSyncMessage);
        GstContextBridgeRegistry::registerResetCallback(&GstGlContextBridge::reset);
    }
};
static GlBridgeRegistrar s_glBridgeRegistrar;
} // anonymous namespace

} // namespace GstGlContextBridge

#endif // QGC_HAS_GST_GLMEMORY_GPU_PATH
