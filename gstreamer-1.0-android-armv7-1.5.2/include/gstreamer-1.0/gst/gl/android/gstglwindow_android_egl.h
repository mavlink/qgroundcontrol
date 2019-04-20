/*
 * GStreamer
 * Copyright (C) 2012 Matthew Waters <ystreet00@gmail.com>
 * Copyright (C) 2013 Sebastian Dröge <slomo@circular-chaos.org>
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

#ifndef __GST_GL_WINDOW_ANDROID_EGL_H__
#define __GST_GL_WINDOW_ANDROID_EGL_H__

#include <gst/gl/gl.h>

G_BEGIN_DECLS

#define GST_GL_TYPE_WINDOW_ANDROID_EGL         (gst_gl_window_android_egl_get_type())
#define GST_GL_WINDOW_ANDROID_EGL(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), GST_GL_TYPE_WINDOW_ANDROID_EGL, GstGLWindowAndroidEGL))
#define GST_GL_WINDOW_ANDROID_EGL_CLASS(k)     (G_TYPE_CHECK_CLASS((k), GST_GL_TYPE_WINDOW_ANDROID_EGL, GstGLWindowAndroidEGLClass))
#define GST_GL_IS_WINDOW_ANDROID_EGL(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), GST_GL_TYPE_WINDOW_ANDROID_EGL))
#define GST_GL_IS_WINDOW_ANDROID_EGL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), GST_GL_TYPE_WINDOW_ANDROID_EGL))
#define GST_GL_WINDOW_ANDROID_EGL_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GST_GL_TYPE_WINDOW_ANDROID_EGL, GstGLWindowAndroidEGL_Class))

typedef struct _GstGLWindowAndroidEGL        GstGLWindowAndroidEGL;
typedef struct _GstGLWindowAndroidEGLClass   GstGLWindowAndroidEGLClass;

struct _GstGLWindowAndroidEGL {
  /*< private >*/
  GstGLWindow parent;

  /* This is actually an ANativeWindow */
  EGLNativeWindowType native_window;
  gint window_width, window_height;

  gpointer _reserved[GST_PADDING];
};

struct _GstGLWindowAndroidEGLClass {
  /*< private >*/
  GstGLWindowClass parent_class;

  /*< private >*/
  gpointer _reserved[GST_PADDING];
};

GType gst_gl_window_android_egl_get_type     (void);

GstGLWindowAndroidEGL * gst_gl_window_android_egl_new  (void);

G_END_DECLS

#endif /* __GST_GL_WINDOW_ANDROID_H__ */
