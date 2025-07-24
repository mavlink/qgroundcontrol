/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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

#ifndef __GST_QT6_SINK_H__
#define __GST_QT6_SINK_H__

#include <gst/gst.h>
#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#include <gst/gl/gl.h>
#include "qt6glitem.h"

typedef struct _GstQml6GLSinkPrivate GstQml6GLSinkPrivate;

G_BEGIN_DECLS

#define GST_TYPE_QML6_GL_SINK (gst_qml6_gl_sink_get_type())
G_DECLARE_FINAL_TYPE (GstQml6GLSink, gst_qml6_gl_sink, GST, QML6_GL_SINK, GstVideoSink)
#define GST_QML6_GL_SINK_CAST(obj) ((GstQml6GLSink*)(obj))

/**
 * GstQml6GLSink:
 *
 * Opaque #GstQml6GLSink object
 */
struct _GstQml6GLSink
{
  /* <private> */
  GstVideoSink          parent;

  GstVideoInfo          v_info;
  GstBufferPool        *pool;

  GstGLDisplay         *display;
  GstGLContext         *context;
  GstGLContext         *qt_context;

  QSharedPointer<Qt6GLVideoItemInterface> widget;
};

GstQml6GLSink *    gst_qml6_gl_sink_new (void);

G_END_DECLS

#endif /* __GST_QT6_SINK_H__ */
