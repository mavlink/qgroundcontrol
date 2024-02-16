/*
 * GStreamer
 * Copyright (C) 2020 Matthew Waters <matthew@centricular.com>
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

#ifndef __GST_QML6_GL_OVERLAY_H__
#define __GST_QML6_GL_OVERLAY_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include "qt6glrenderer.h"
#include "qt6glitem.h"

typedef struct _GstQml6GLOverlay GstQml6GLOverlay;
typedef struct _GstQml6GLOverlayClass GstQml6GLOverlayClass;
typedef struct _GstQml6GLOverlayPrivate GstQml6GLOverlayPrivate;

G_BEGIN_DECLS

GType gst_qml6_gl_overlay_get_type (void);
#define GST_TYPE_QML6_GL_OVERLAY            (gst_qml6_gl_overlay_get_type())
#define GST_QML6_GL_OVERLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_QML6_GL_OVERLAY,GstQml6GLOverlay))
#define GST_QML6_GL_OVERLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_QML6_GL_OVERLAY,GstQml6GLOverlayClass))
#define GST_IS_QML6_GL_OVERLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_QML6_GL_OVERLAY))
#define GST_IS_QML6_GL_OVERLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_QML6_GL_OVERLAY))
#define GST_QML6_GL_OVERLAY_CAST(obj)       ((GstQml6GLOverlay*)(obj))

/**
 * GstQml6GLOverlay:
 *
 * Opaque #GstQml6GLOverlay object
 */
struct _GstQml6GLOverlay
{
  /* <private> */
  GstGLFilter           parent;

  gchar                *qml_scene;

  GstQt6QuickRenderer     *renderer;

  QSharedPointer<Qt6GLVideoItemInterface> widget;
};

/**
 * GstQml6GLOverlayClass:
 *
 * The #GstQml6GLOverlayClass struct only contains private data
 */
struct _GstQml6GLOverlayClass
{
  /* <private> */
  GstGLFilterClass parent_class;
};

GstQml6GLOverlay *    gst_qml6_gl_overlay_new (void);

G_END_DECLS

#endif /* __GST_QML6_GL_OVERLAY_H__ */
