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

#ifndef __GST_QML6_GL_SRC_H__
#define __GST_QML6_GL_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include "qt6glwindow.h"

G_BEGIN_DECLS

#define GST_TYPE_QML6_GL_SRC (gst_qml6_gl_src_get_type())
G_DECLARE_FINAL_TYPE (GstQml6GLSrc, gst_qml6_gl_src, GST, QML6_GL_SRC, GstPushSrc)
#define GST_QML6_GL_SRC_CAST(obj) ((GstQml6GLSrc*)(obj))

/**
 * GstQml6GLSrc:
 *
 * Opaque #GstQml6GLSrc object
 */
struct _GstQml6GLSrc
{
  /* <private> */
  GstPushSrc            parent;

  QQuickWindow         *qwindow;
  Qt6GLWindow          *window;

  GstVideoInfo          v_info;

  GstGLDisplay         *display;
  GstGLContext         *context;
  GstGLContext         *qt_context;

  gboolean              default_fbo;
  gboolean              downstream_supports_affine_meta;
  gboolean              pending_image_orientation;
};

G_END_DECLS

#endif /* __GST_QML6_GL_SRC_H__ */
