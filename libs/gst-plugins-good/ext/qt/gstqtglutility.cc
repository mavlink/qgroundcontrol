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

#include "gstqtglutility.h"
#include <QtGui/QGuiApplication>

#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
#include <QX11Info>
#include <gst/gl/x11/gstgldisplay_x11.h>
#endif

#if GST_GL_HAVE_WINDOW_WAYLAND && GST_GL_HAVE_PLATFORM_EGL && defined (HAVE_QT_WAYLAND)
#include <qpa/qplatformnativeinterface.h>
#include <gst/gl/wayland/gstgldisplay_wayland.h>
#endif

#if GST_GL_HAVE_PLATFORM_EGL && (defined (HAVE_QT_EGLFS) || defined (HAVE_QT_ANDROID))
#if GST_GL_HAVE_WINDOW_VIV_FB
#include <qpa/qplatformnativeinterface.h>
#include <gst/gl/viv-fb/gstgldisplay_viv_fb.h>
#else
#include <gst/gl/egl/gstegl.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#ifdef HAVE_QT_QPA_HEADER
#include <qpa/qplatformnativeinterface.h>
#endif
#endif
#endif

#include <gst/gl/gstglfuncs.h>

#define GST_CAT_DEFAULT qt_gl_utils_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

GstGLDisplay *
gst_qt_get_gl_display ()
{
  GstGLDisplay *display = NULL;
  QGuiApplication *app = static_cast<QGuiApplication *> (QCoreApplication::instance ());
  static volatile gsize _debug;

  g_assert (app != NULL);

  if (g_once_init_enter (&_debug)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "qtglutility", 0,
        "Qt gl utility functions");
    g_once_init_leave (&_debug, 1);
  }
  GST_INFO ("QGuiApplication::instance()->platformName() %s", app->platformName().toUtf8().data());

#if GST_GL_HAVE_WINDOW_X11 && defined (HAVE_QT_X11)
  if (QString::fromUtf8 ("xcb") == app->platformName())
    display = (GstGLDisplay *)
        gst_gl_display_x11_new_with_display (QX11Info::display ());
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
      QPlatformNativeInterface *native =
          QGuiApplication::platformNativeInterface();
      EGLDisplay egl_display = (EGLDisplay)
          native->nativeResourceForWindow("egldisplay", NULL);

      However we seem to have no way for getting the EGLNativeDisplayType, aka
      native_display, via public API. As such we have to assume that display 0
      is always used. Only way around that is parsing the index the same way as
      Qt does in QEGLDeviceIntegration::fbDeviceName(), so let's do that.
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

  return display;
}

gboolean
gst_qt_get_gl_wrapcontext (GstGLDisplay * display,
    GstGLContext **wrap_glcontext, GstGLContext **context)
{
  GstGLPlatform platform = (GstGLPlatform) 0;
  GstGLAPI gl_api;
  guintptr gl_handle;
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
  if (gl_handle)
    *wrap_glcontext =
        gst_gl_context_new_wrapped (display, gl_handle,
        platform, gl_api);

  if (!*wrap_glcontext) {
    GST_ERROR ("cannot wrap qt OpenGL context");
    return FALSE;
  }

  (void) platform;
  (void) gl_api;
  (void) gl_handle;

  gst_gl_context_activate(*wrap_glcontext, TRUE);
  if (!gst_gl_context_fill_info (*wrap_glcontext, &error)) {
    GST_ERROR ("failed to retrieve qt context info: %s", error->message);
    g_object_unref (*wrap_glcontext);
    *wrap_glcontext = NULL;
    return FALSE;
  } else {
    gst_gl_display_filter_gl_api (display, gst_gl_context_get_gl_api (*wrap_glcontext));
#if GST_GL_HAVE_WINDOW_WIN32 && GST_GL_HAVE_PLATFORM_WGL && defined (HAVE_QT_WIN32)  
    g_return_val_if_fail (context != NULL, FALSE);

    G_STMT_START {
      GstGLWindow *window;
      HDC device;

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
      *context = gst_gl_context_new (display);
      window = gst_gl_context_get_window (*context);
      device = (HDC) gst_gl_window_get_display (window);

      wglMakeCurrent (device, 0);
      gst_object_unref (window);
      if (!gst_gl_context_create (*context, *wrap_glcontext, &error)) {
        GST_ERROR ("failed to create shared GL context: %s", error->message);
        g_object_unref (*context);
        *context = NULL;
        g_object_unref (*wrap_glcontext);
        *wrap_glcontext = NULL;
        wglMakeCurrent (device, (HGLRC) gl_handle);
        return FALSE;
      }
      wglMakeCurrent (device, (HGLRC) gl_handle);
    } G_STMT_END;
#endif
    gst_gl_context_activate (*wrap_glcontext, FALSE);
  }

  return TRUE;
}
