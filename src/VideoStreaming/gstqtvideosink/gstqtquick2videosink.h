/*
    Copyright (C) 2013 Collabora Ltd.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

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

#ifndef __GST_QT_QUICK2_VIDEO_SINK_H__
#define __GST_QT_QUICK2_VIDEO_SINK_H__

#include <gst/video/gstvideosink.h>
#include <QtGlobal>

#define GST_TYPE_QT_QUICK2_VIDEO_SINK \
    (gst_qt_quick2_video_sink_get_type ())
#define GST_QT_QUICK2_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_QT_QUICK2_VIDEO_SINK, GstQtQuick2VideoSink))
#define GST_IS_QT_QUICK2_VIDEO_SINK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_QT_QUICK2_VIDEO_SINK))
#define GST_QT_QUICK2_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_QT_QUICK2_VIDEO_SINK, GstQtQuick2VideoSinkClass))
#define GST_IS_QT_QUICK2_VIDEO_SINK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_QT_QUICK2_VIDEO_SINK))
#define GST_QT_QUICK2_VIDEO_SINK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_QT_QUICK2_VIDEO_SINK, GstQtQuick2VideoSinkClass))

typedef struct _GstQtQuick2VideoSink GstQtQuick2VideoSink;
typedef struct _GstQtQuick2VideoSinkClass GstQtQuick2VideoSinkClass;
typedef struct _GstQtQuick2VideoSinkPrivate GstQtQuick2VideoSinkPrivate;

struct _GstQtQuick2VideoSink
{
    GstVideoSink parent_instance;
    GstQtQuick2VideoSinkPrivate *priv;
};

struct _GstQtQuick2VideoSinkClass
{
    GstVideoSinkClass parent_class;

    gpointer (*update_node)(GstQtQuick2VideoSink *self,
                            gpointer node, qreal x, qreal y, qreal w, qreal h);
};

GType gst_qt_quick2_video_sink_get_type (void);

#endif /* __GST_QT_QUICK2_VIDEO_SINK_H__ */
