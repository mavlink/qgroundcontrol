/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011-2012 Collabora Ltd. <info@collabora.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 2.1
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file
 *   @brief Extracted from QtGstreamer to avoid overly complex dependency
 *   @author Gus Grubba <mavlink@grubba.com>
 */

#ifndef GST_QT_GL_VIDEO_SINK_H
#define GST_QT_GL_VIDEO_SINK_H

#include "gstqtglvideosinkbase.h"

#ifndef GST_QT_VIDEO_SINK_NO_OPENGL

#define GST_TYPE_QT_GL_VIDEO_SINK \
  (GstQtGLVideoSink::get_type())

struct GstQtGLVideoSink
{
public:
    GstQtGLVideoSinkBase parent;

    static GType get_type();
    static void emit_update(gpointer sink);

private:
    enum {
        PROP_0,
        PROP_GLCONTEXT
    };

    enum {
        PAINT_SIGNAL,
        UPDATE_SIGNAL,
        LAST_SIGNAL
    };

    static void base_init(gpointer g_class);
    static void class_init(gpointer g_class, gpointer class_data);
    static void init(GTypeInstance *instance, gpointer g_class);

    static void set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec);

    static void paint(GstQtGLVideoSink *sink, gpointer painter,
                      qreal x, qreal y, qreal width, qreal height);

    static guint s_signals[LAST_SIGNAL];
};


struct GstQtGLVideoSinkClass
{
    GstQtGLVideoSinkBaseClass parent_class;

    /* paint action signal */
    void (*paint) (GstQtGLVideoSink *sink, gpointer painter,
                   qreal x, qreal y, qreal width, qreal height);
};

#endif // GST_QT_VIDEO_SINK_NO_OPENGL
#endif // GST_QT_GL_VIDEO_SINK_H
