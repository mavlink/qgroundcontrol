/*
 * GStreamer
 * Copyright (C) 2016 Freescale Semiconductor, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstqt6glutility.h"
#include <QtGui/QGuiApplication>
#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
#include <gst/gl/x11/gstgldisplay_x11.h>
//#include <QtPlatformHeaders/QGLXNativeContext>
#endif
#if GST_GL_HAVE_PLATFORM_EGL && (defined (HAVE_QT_WAYLAND) || defined (HAVE_QT_EGLFS) || defined (HAVE_QT_ANDROID))
#include <gst/gl/egl/gstegl.h>
#ifdef HAVE_QT_QPA_HEADER
#include QT_QPA_HEADER
#endif
//#include <QtPlatformHeaders/QEGLNativeContext>
#include <gst/gl/egl/gstgldisplay_egl.h>
#endif

#if GST_GL_HAVE_WINDOW_WAYLAND && defined (HAVE_QT_WAYLAND)
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif
#if 0
#if GST_GL_HAVE_WINDOW_VIV_FB
#include <gst/gl/viv-fb/gstgldisplay_viv_fb.h>
#endif

#if GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)
#include <windows.h>
#include <QtPlatformHeaders/QWGLNativeContext>
#endif
#endif
#include <gst/gl/gstglfuncs.h>

#define GST_CAT_DEFAULT qml6_gl_utils_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

G_LOCK_DEFINE_STATIC (display_lock);
static GWeakRef qt_display;
static gboolean sink_retrieved = FALSE;

GstGLDisplay *
gst_qml6_get_gl_display (gboolean sink)
{
  GstGLDisplay *display = NULL;
  QGuiApplication *app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
  static gsize _debug;

  g_assert (app != NULL);

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtglutility", 0,
        "Qt gl utility functions");
    g_once_init_leave (&_debug, 1);
  }

  G_LOCK (display_lock);
  /* XXX: this assumes that only one display will ever be created by Qt */
  display = static_cast<GstGLDisplay *>(g_weak_ref_get (&qt_display));
  if (display) {
    if (sink_retrieved) {
      GST_INFO ("returning previously created display");
      G_UNLOCK (display_lock);
      return display;
    }
    gst_clear_object (&display);
  }
  if (sink)
    sink_retrieved = sink;

  GST_INFO ("QGuiApplication::instance()->platformName() %s", app->platformName().toUtf8().data());
#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
  if (QString::fromUtf8 ("xcb") == app->platformName()) {
    auto x11_native = app->nativeInterface<QNativeInterface::QX11Application>();
    if (x11_native) {
      display = (GstGLDisplay *)
          gst_gl_display_x11_new_with_display (x11_native->display());
    }
  }
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && GST_GL_HAVE_PLATFORM_EGL && defined (HAVE_QT_WAYLAND)
  if (QString::fromUtf8 ("wayland") == app->platformName()
        || QString::fromUtf8 ("wayland-egl") == app->platformName()){
    struct wl_display * wayland_display;
    QPlatformNativeInterface *native =
        QGuiApplication::platformNativeInterface();
    wayland_display = (struct wl_display *)
        native->nativeResourceForWindow("display", NULL);
    display = (GstGLDisplay *)
        gst_gl_display_wayland_new_with_display (wayland_display);
  }
#endif
#if GST_GL_HAVE_PLATFORM_EGL && GST_GL_HAVE_WINDOW_ANDROID
  if (QString::fromUtf8 ("android") == app->platformName()) {
    EGLDisplay egl_display = (EGLDisplay) gst_gl_display_egl_get_from_native (GST_GL_DISPLAY_TYPE_ANY, 0);
    display = (GstGLDisplay *) gst_gl_display_egl_new_with_egl_display (egl_display);
  }
#elif GST_GL_HAVE_PLATFORM_EGL && defined (HAVE_QT_EGLFS)
  if (QString::fromUtf8("eglfs") == app->platformName()) {
#if GST_GL_HAVE_WINDOW_VIV_FB
    /* FIXME: Could get the display directly from Qt like this
     * QPlatformNativeInterface *native =
     *     QGuiApplication::platformNativeInterface();
     * EGLDisplay egl_display = (EGLDisplay)
     *     native->nativeResourceForWindow("egldisplay", NULL);
     *
     * However we seem to have no way for getting the EGLNativeDisplayType, aka
     * native_display, via public API. As such we have to assume that display 0
     * is always used. Only way around that is parsing the index the same way as
     * Qt does in QEGLDeviceIntegration::fbDeviceName(), so let's do that.
     */
    const gchar *fb_dev;
    gint disp_idx = 0;

    fb_dev = g_getenv ("QT_QPA_EGLFS_FB");
    if (fb_dev) {
      if (sscanf (fb_dev, "/dev/fb%d", &disp_idx) != 1)
        disp_idx = 0;
    }

    display = (GstGLDisplay *) gst_gl_display_viv_fb_new (disp_idx);
#elif defined(HAVE_QT_QPA_HEADER)
    QPlatformNativeInterface *native =
        QGuiApplication::platformNativeInterface();
    EGLDisplay egl_display = (EGLDisplay)
        native->nativeResourceForWindow("egldisplay", NULL);
    if (egl_display != EGL_NO_DISPLAY)
      display = (GstGLDisplay *) gst_gl_display_egl_new_with_egl_display (egl_display);
#else
    EGLDisplay egl_display = (EGLDisplay) gst_gl_display_egl_get_from_native (GST_GL_DISPLAY_TYPE_ANY, 0);
    display = (GstGLDisplay *) gst_gl_display_egl_new_with_egl_display (egl_display);
#endif
  }
#endif
#if GST_GL_HAVE_WINDOW_COCOA && GST_GL_HAVE_PLATFORM_CGL && defined (HAVE_QT_MAC)
  if (QString::fromUtf8 ("cocoa") == app->platformName())
    display = (GstGLDisplay *) gst_gl_display_new ();
#endif
#if GST_GL_HAVE_WINDOW_EAGL && GST_GL_HAVE_PLATFORM_EAGL && defined (HAVE_QT_IOS)
  if (QString::fromUtf8 ("ios") == app->platformName())
    display = gst_gl_display_new ();
#endif
#if GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)
  if (QString::fromUtf8 ("windows") == app->platformName())
    display = gst_gl_display_new ();
#endif

  if (!display)
    display = gst_gl_display_new ();

  g_weak_ref_set (&qt_display, display);
  G_UNLOCK (display_lock);

  return display;
}

gboolean
gst_qml6_get_gl_wrapcontext (GstGLDisplay * display,
    GstGLContext **wrap_glcontext, GstGLContext **context)
{
  GstGLPlatform G_GNUC_UNUSED platform = (GstGLPlatform) 0;
  GstGLAPI G_GNUC_UNUSED gl_api;
  guintptr G_GNUC_UNUSED gl_handle;
  GstGLContext *current;
  GError *error = NULL;

  g_return_val_if_fail (display != NULL && wrap_glcontext != NULL, FALSE);
#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
  if (GST_IS_GL_DISPLAY_X11 (display)) {
#if GST_GL_HAVE_PLATFORM_GLX
    platform = GST_GL_PLATFORM_GLX;
#elif GST_GL_HAVE_PLATFORM_EGL
    platform = GST_GL_PLATFORM_EGL;
#endif
  }
#endif
#if GST_GL_HAVE_WINDOW_WAYLAND && defined (HAVE_QT_WAYLAND)
  if (GST_IS_GL_DISPLAY_WAYLAND (display)) {
    platform = GST_GL_PLATFORM_EGL;
  }
#endif
#if GST_GL_HAVE_PLATFORM_EGL && defined (HAVE_QT_EGLFS)
#if GST_GL_HAVE_WINDOW_VIV_FB
  if (GST_IS_GL_DISPLAY_VIV_FB (display)) {
#else
  if (GST_IS_GL_DISPLAY_EGL (display)) {
#endif
    platform = GST_GL_PLATFORM_EGL;
  }
#endif
  if (platform == 0) {
#if GST_GL_HAVE_WINDOW_COCOA && GST_GL_HAVE_PLATFORM_CGL && defined (HAVE_QT_MAC)
    platform = GST_GL_PLATFORM_CGL;
#elif GST_GL_HAVE_WINDOW_EAGL && GST_GL_HAVE_PLATFORM_EAGL && defined (HAVE_QT_IOS)
    platform = GST_GL_PLATFORM_EAGL;
#elif GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)
    platform = GST_GL_PLATFORM_WGL;
#elif GST_GL_HAVE_WINDOW_ANDROID && GST_GL_HAVE_PLATFORM_EGL && defined (HAVE_QT_ANDROID)
    platform = GST_GL_PLATFORM_EGL;
#else
    GST_ERROR ("Unknown platform");
    return FALSE;
#endif
  }

  gl_api = gst_gl_context_get_current_gl_api (platform, NULL, NULL);
  gl_handle = gst_gl_context_get_current_gl_context (platform);

  /* see if we already have a current GL context in GStreamer for this thread */
  current = gst_gl_context_get_current ();
  if (current && current->display == display) {
    /* just use current context we found */
    *wrap_glcontext = static_cast<GstGLContext *> (gst_object_ref (current));
  }
  else {
    if (gl_handle)
      *wrap_glcontext =
          gst_gl_context_new_wrapped (display, gl_handle,
          platform, gl_api);

    if (!*wrap_glcontext) {
      GST_ERROR ("cannot wrap qt OpenGL context");
      return FALSE;
    }

    gst_gl_context_activate(*wrap_glcontext, TRUE);
    if (!gst_gl_context_fill_info (*wrap_glcontext, &error)) {
      GST_ERROR ("failed to retrieve qt context info: %s", error->message);
      gst_gl_context_activate(*wrap_glcontext, FALSE);
      gst_clear_object (wrap_glcontext);
      return FALSE;
    }

    gst_gl_display_filter_gl_api (display, gst_gl_context_get_gl_api (*wrap_glcontext));
    gst_gl_context_activate (*wrap_glcontext, FALSE);
  }
#if GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)
  g_return_val_if_fail (context != NULL, FALSE);

  G_STMT_START {
    /* If there's no wglCreateContextAttribsARB() support, then we would fallback to
     * wglShareLists() which will fail with ERROR_BUSY (0xaa) if either of the GL
     * contexts are current in any other thread.
     *
     * The workaround here is to temporarily disable Qt's GL context while we
     * set up our own.
     *
     * Sometimes wglCreateContextAttribsARB()
     * exists, but isn't functional (some Intel drivers), so it's easiest to do this
     * unconditionally.
     */

    /* retrieve Qt's GL device context as current device context */
    HDC device = wglGetCurrentDC ();

    *context = gst_gl_context_new (display);

    wglMakeCurrent (NULL, NULL);
    if (!gst_gl_context_create (*context, *wrap_glcontext, &error)) {
      GST_ERROR ("failed to create shared GL context: %s", error->message);
      gst_clear_object (wrap_glcontext);
      gst_clear_object (context);
    }
    wglMakeCurrent (device, (HGLRC) gl_handle);

    if (!*context)
      return FALSE;

  } G_STMT_END;
#endif
  return TRUE;
}

QOpenGLContext *
qt_opengl_native_context_from_gst_gl_context (GstGLContext * context)
{
  guintptr handle;
  GstGLPlatform platform;
  QOpenGLContext *ret = NULL;

  handle = gst_gl_context_get_gl_context (context);
  platform = gst_gl_context_get_gl_platform (context);

  /* this is required as Qt doesn't allow retrieving the relevant native
   * interface unless the underlying context has been created */
  QOpenGLContext *qt_gl_context = new QOpenGLContext();
  qt_gl_context->create();

#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
  if (!ret && platform == GST_GL_PLATFORM_GLX) {
    auto glx = qt_gl_context->nativeInterface<QNativeInterface::QGLXContext>();
    if (!glx) {
      GST_WARNING ("Retriving GLX context interface from Qt failed");
    } else {
      GstGLDisplay *display = gst_gl_context_get_display (context);
      GstGLWindow *window = gst_gl_context_get_window (context);
      gst_object_unref (window);
      gst_object_unref (display);
      ret = glx->fromNative((GLXContext) handle);
    }
  }
#endif
#if GST_GL_HAVE_PLATFORM_EGL && (defined (HAVE_QT_WAYLAND) || defined (HAVE_QT_EGLFS) || defined (HAVE_QT_ANDROID))
  if (!ret && platform == GST_GL_PLATFORM_EGL) {
    auto egl = qt_gl_context->nativeInterface<QNativeInterface::QEGLContext>();
    if (!egl) {
      GST_WARNING ("Retriving EGL context interface from Qt failed");
    } else {
      EGLDisplay egl_display = EGL_DEFAULT_DISPLAY;
      GstGLDisplay *display = gst_gl_context_get_display (context);
      GstGLDisplayEGL *display_egl = gst_gl_display_egl_from_gl_display (display);
#if GST_GL_HAVE_WINDOW_WAYLAND && defined (HAVE_QT_WAYLAND)
      if (gst_gl_display_get_handle_type (display) == GST_GL_DISPLAY_TYPE_WAYLAND) {
#if 0
        g_warning ("Qt does not support wrapping native OpenGL contexts "
            "on wayland. See https://bugreports.qt.io/browse/QTBUG-82528");
        gst_object_unref (display_egl);
        gst_object_unref (display);
        return NULL;
#else
        if (display_egl)
          egl_display = (EGLDisplay) gst_gl_display_get_handle ((GstGLDisplay *) display_egl);
#endif
      }
#endif
      gst_object_unref (display_egl);
      gst_object_unref (display);
      GST_ERROR ("creating native context from context %p and display %p", (void *) handle, egl_display);
      ret = egl->fromNative((EGLContext) handle, egl_display);
      GST_ERROR ("created native context %p", ret);
    }
  }
#endif
#if GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)
  if (!ret && platform == GST_GL_PLATFORM_WGL) {
    auto wgl = qt_gl_context->nativeInterface<QNativeInterface::QWGLContext>();
    if (!wgl) {
      GST_WARNING ("Retriving WGL context interface from Qt failed");
    } else {
      GstGLWindow *window = gst_gl_context_get_window (context);
      guintptr hwnd = gst_gl_window_get_window_handle (window);
      gst_object_unref (window);
      ret = wgl->fromNative((HGLRC) handle, (HWND) hwnd);
    }
  }
#endif
  if (!ret) {
    gchar *platform_s = gst_gl_platform_to_string (platform);
    g_warning ("Unimplemented configuration!  This means either:\n"
        "1. Qt6 wasn't built with support for \'%s\'\n"
        "2. The qmlgl plugin was built without support for your platform.\n"
        "3. The necessary code to convert from a GstGLContext to Qt's "
        "native context type for \'%s\' currently does not exist."
        "4. Qt failed to wrap an existing native context.",
        platform_s, platform_s);
    g_free (platform_s);
  }

  qt_gl_context->doneCurrent();
  delete qt_gl_context;

  gst_gl_context_activate (context, FALSE);
  gst_gl_context_activate (context, TRUE);

  return ret;
}
