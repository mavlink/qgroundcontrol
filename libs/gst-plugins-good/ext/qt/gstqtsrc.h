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

#ifndef __GST_QT_SRC_H__
#define __GST_QT_SRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include "qtwindow.h"

typedef struct _GstQtSrc GstQtSrc;
typedef struct _GstQtSrcClass GstQtSrcClass;

G_BEGIN_DECLS

GType gst_qt_src_get_type (void);
#define GST_TYPE_QT_SRC            (gst_qt_src_get_type())
#define GST_QT_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QT_SRC,GstQtSrc))
#define GST_QT_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QT_SRC,GstQtSrcClass))
#define GST_IS_QT_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QT_SRC))
#define GST_IS_QT_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QT_SRC))
#define GST_QT_SRC_CAST(obj)       ((GstQtSrc*)(obj))

/**
 * GstQtSrc:
 *
 * Opaque #GstQtSrc object
 */
struct _GstQtSrc
{
  /* <private> */
  GstPushSrc            parent;

  QQuickWindow          *qwindow;
  QtGLWindow            *window;

  GstVideoInfo          v_info;

  GstGLDisplay         *display;
  GstGLContext         *context;
  GstGLContext         *qt_context;

  gboolean              default_fbo;
  gboolean              downstream_supports_affine_meta;
  gboolean              pending_image_orientation;
};

/**
 * GstQtSrcClass:
 *
 * The #GstQtSrcClass struct only contains private data
 */
struct _GstQtSrcClass
{
  /* <private> */
  GstPushSrcClass object_class;
};

G_END_DECLS

#endif /* __GST_QT_SRC_H__ */
