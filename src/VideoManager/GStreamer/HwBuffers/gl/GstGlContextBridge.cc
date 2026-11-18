#include "GstGlContextBridge.h"

#if defined(QGC_HAS_GST_GLMEMORY_GPU_PATH)

#include <gst/gl/gl.h>
#include <utility>

#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>
#include <QtGui/QGuiApplication>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtGui/qguiapplication_platform.h>

#include "QGCLoggingCategory.h"
// EGL on Linux and Android; GLX/Wayland/X11 wrapping below only where their headers exist.
#include <gst/gl/egl/gstgldisplay_egl.h>

#include <EGL/egl.h>
// Qt 6 on xcb uses GLX by default — fall back to GLX context wrapping when EGL isn't available.
#if __has_include(<gst/gl/x11/gstgldisplay_x11.h>)
#include <gst/gl/x11/gstgldisplay_x11.h>

#include <QtGui/qopenglcontext_platform.h>

#include <X11/Xlib.h>
#define QGC_GST_BRIDGE_HAS_GLX 1
#endif
// Wayland: downstream elements probe GST_IS_GL_DISPLAY_WAYLAND; wl_display must be tagged on GstGLDisplay or zero-copy
// paths are silently missed. Only from 1.26, where the EGL display gst derives from it can be marked foreign.
#if __has_include(<gst/gl/wayland/gstgldisplay_wayland.h>) && GST_CHECK_VERSION(1, 26, 0)
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#define QGC_GST_BRIDGE_HAS_WAYLAND 1
#endif

QGC_LOGGING_CATEGORY(GstGlBridgeLog, "Video.GStreamer.HwBuffers.GstGlBridge")

namespace {

// The API of Qt's context: what the wrap is declared as, and all primeLocked() lets gst-gl create contexts in.
GstGLAPI wrappedGlApi(const QOpenGLContext& qtCtx)
{
    if (qtCtx.isOpenGLES()) {
        return GST_GL_API_GLES2;
    }
    return (qtCtx.format().profile() == QSurfaceFormat::CoreProfile) ? GST_GL_API_OPENGL3 : GST_GL_API_OPENGL;
}

constexpr char kGlAppContextType[] = "gst.gl.app_context";
// A pipeline is given these in this order: a gst-gl element creates its GL context as soon as it is handed another
// display, sharing with the app context it holds by then.
constexpr const char* kContextTypes[] = {kGlAppContextType, GST_GL_DISPLAY_CONTEXT_TYPE};

class GlContextBridge final : public GstContextBridgeBase
{
public:
    constexpr GlContextBridge() noexcept
        : GstContextBridgeBase("GL", kContextTypes, &GstGlBridgeLog)
    {}

    bool shares(GstGLContext* context);

private:
    bool primeLocked() override;
    GstObject* refObject(const char* contextType) override;
    GstContext* buildContext(const char* contextType, GstObject* object) override;
    void clearLocked() override;

    GstGLDisplay* _display = nullptr;
    GstGLContext* _context = nullptr;
    // The wrap the last reset() dropped. The contexts gst-gl created from it still share with Qt's context, which
    // outlives every scene graph, so a running stream's textures stay importable.
    GstGLContext* _retiredContext = nullptr;
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

    // Try EGL first (Wayland, eglfs, xcb_egl, Android); fall back to GLX (Linux xcb default on Qt 6).
    const auto* const egl = qtCtx->nativeInterface<QNativeInterface::QEGLContext>();
    const EGLContext eglCtx = egl ? egl->nativeContext() : EGL_NO_CONTEXT;
    // QEGLContext::display() is the only handle that guarantees EGLImage import/sample compatibility; fall back to
    // EGL_DEFAULT_DISPLAY when unavailable.
    EGLDisplay eglDisp = (eglCtx != EGL_NO_CONTEXT) ? egl->display() : EGL_NO_DISPLAY;
    if ((eglCtx != EGL_NO_CONTEXT) && (eglDisp == EGL_NO_DISPLAY)) {
        eglDisp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    }
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
                                              wrappedGlApi(*qtCtx));
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
        // What Qt's context is can't change while the application runs, so these stay off rather than retry (and
        // warn) on every context request.
        if (!glx) {
            qCWarning(GstGlBridgeLog) << "Qt GL context exposes neither EGL nor GLX; GL bridge disabled";
            return false;
        }
        Display* xdisp = nullptr;
        if (auto* x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
            xdisp = x11->display();
        }
        if (!xdisp) {
            qCWarning(GstGlBridgeLog) << "X11 Display unresolvable; GL bridge disabled";
            return false;
        }
        GstGLDisplayX11* displayX11 = gst_gl_display_x11_new_with_display(xdisp);
        if (!displayX11) {
            qCWarning(GstGlBridgeLog) << "gst_gl_display_x11_new_with_display failed";
            return retryLater();
        }
        _display = GST_GL_DISPLAY(displayX11);
        _context = gst_gl_context_new_wrapped(_display, reinterpret_cast<guintptr>(glx->nativeContext()),
                                              GST_GL_PLATFORM_GLX, wrappedGlApi(*qtCtx));
        if (!_context) {
            qCWarning(GstGlBridgeLog) << "gst_gl_context_new_wrapped (GLX) failed";
            gst_clear_object(&_display);
            return retryLater();
        }
        qCInfo(GstGlBridgeLog) << "GL bridge primed (GLX)";
#else
        // Permanent for the application: Qt's context won't turn into an EGL one.
        qCWarning(GstGlBridgeLog) << "Qt EGLContext unresolvable and GLX bridge not built; GL bridge disabled";
        return false;
#endif
    }

    // gst-gl takes the API of a context it creates from the display, not from the context it shares with, and would
    // pick OpenGL ES or desktop OpenGL by its own order of preference. EGL defines sharing within one client API only.
    gst_gl_display_filter_gl_api(_display, gst_gl_context_get_gl_api(_context));

    // No gst_gl_context_fill_info(): it reads the context current on the calling thread, and Qt's global share context
    // is never current on any; nothing fills a wrap later either. gst-gl only shares with the wrap (handle, platform
    // and API, all set above) and runs no GL through it. Unfilled, it carries no EGLConfig for gst-gl to copy, so
    // gst-gl chooses the config of its own contexts, one that fits the surface it creates for them.
    qCDebug(GstGlBridgeLog) << "GL bridge primed: display=" << _display << "context=" << _context;
    return true;
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
    if (_context) {
        gst_clear_object(&_retiredContext);
        _retiredContext = std::exchange(_context, nullptr);
    }
    gst_clear_object(&_display);
}

bool GlContextBridge::shares(GstGLContext* context)
{
    const QMutexLocker lock(&_mutex);
    const auto sharesWith = [context](GstGLContext* wrapped) {
        return wrapped && ((wrapped == context) || gst_gl_context_can_share(wrapped, context));
    };
    return context && (sharesWith(_context) || sharesWith(_retiredContext));
}

}  // namespace

GstContextBridgeBase& GstGlContextBridge::instance()
{
    return s_bridge;
}

bool GstGlContextBridge::sharesQtContext(GstGLContext* context)
{
    return s_bridge.shares(context);
}

#endif  // QGC_HAS_GST_GLMEMORY_GPU_PATH
