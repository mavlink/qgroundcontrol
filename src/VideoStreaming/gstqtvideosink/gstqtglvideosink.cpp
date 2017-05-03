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

/*
 * Note on the marshallers
 *
 * If it ever needs to be recreated, here is what you need to do:
 *
 * cat > marshaller.src
 * VOID:POINTER,FLOAT,FLOAT,FLOAT,FLOAT
 * VOID:POINTER,DOUBLE,DOUBLE,DOUBLE,DOUBLE
 * POINTER:POINTER,FLOAT,FLOAT,FLOAT,FLOAT
 * POINTER:POINTER,DOUBLE,DOUBLE,DOUBLE,DOUBLE
 * Ctrl+D (EOF)
 *
 * glib-genmarshal --header marshaller.list > gstqtvideosinkmarshal.h
 * glib-genmarshal --body marshaller.list > gstqtvideosinkmarshal.c
 *
 */

#include "gstqtglvideosink.h"
#include "gstqtvideosinkmarshal.h"
#include "delegates/qtvideosinkdelegate.h"


guint GstQtGLVideoSink::s_signals[];

DEFINE_TYPE(GstQtGLVideoSink, GST_TYPE_QT_GL_VIDEO_SINK_BASE)

//------------------------------

void GstQtGLVideoSink::emit_update(gpointer sink)
{
    g_signal_emit(sink, GstQtGLVideoSink::s_signals[UPDATE_SIGNAL], 0, NULL);
}

//------------------------------

void GstQtGLVideoSink::base_init(gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);

    gst_element_class_set_details_simple(element_class, "Qt GL video sink", "Sink/Video",
        "A video sink that can draw on any Qt GL surface",
        "George Kiagiadakis <george.kiagiadakis@collabora.com>");
}

void GstQtGLVideoSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    GObjectClass *object_class = G_OBJECT_CLASS(g_class);
    object_class->set_property = GstQtGLVideoSink::set_property;

    GstQtGLVideoSinkClass *qt_video_sink_class = reinterpret_cast<GstQtGLVideoSinkClass*>(g_class);
    qt_video_sink_class->paint = GstQtGLVideoSink::paint;

    /**
     * GstQtGLVideoSink::paint
     * @painter: A valid QPainter pointer that will be used to paint the video
     * @x: The x coordinate of the target area rectangle
     * @y: The y coordinate of the target area rectangle
     * @width: The width of the target area rectangle
     * @height: The height of the target area rectangle
     *
     * This is an action signal that you can call from your Qt surface class inside
     * its paint function to render the video. It takes a QPainter* and the target
     * area rectangle as arguments. You should schedule to call this function to
     * repaint the surface whenever the ::update signal is emitted.
     *
     * Note that the x,y,width and height arguments are actually qreal. This means
     * that on architectures like arm they will be float instead of double. You should
     * cast the arguments to qreal if they are not already when emitting this signal.
     */

    s_signals[PAINT_SIGNAL] =
        g_signal_new("paint", G_TYPE_FROM_CLASS(g_class),
                     static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                     G_STRUCT_OFFSET(GstQtGLVideoSinkClass, paint),
                     NULL, NULL,
                     qRealIsDouble() ?
                        g_cclosure_user_marshal_VOID__POINTER_DOUBLE_DOUBLE_DOUBLE_DOUBLE :
                        g_cclosure_user_marshal_VOID__POINTER_FLOAT_FLOAT_FLOAT_FLOAT,
                     G_TYPE_NONE, 5,
                     G_TYPE_POINTER, G_TYPE_QREAL, G_TYPE_QREAL, G_TYPE_QREAL, G_TYPE_QREAL);

    /**
     * GstQtGLVideoSink::update
     *
     * This signal is emitted when the surface should be repainted. It should
     * be connected to QWidget::update() or QGraphicsItem::update() or any
     * other similar function in your surface.
     */
    s_signals[UPDATE_SIGNAL] =
        g_signal_new("update", G_TYPE_FROM_CLASS(g_class),
                     G_SIGNAL_RUN_LAST,
                     0, NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);


    /**
     * GstQtGLVideoSink::glcontext
     *
     * This property holds a pointer to the QGLContext that will be used to render
     * the video using OpenGL acceleration. You must set this to a valid QGLContext
     * pointer before the element changes state to READY, or else the state change will fail.
     **/
    g_object_class_install_property(object_class, PROP_GLCONTEXT,
        g_param_spec_pointer("glcontext", "GL context",
                             "The QGLContext that will be used to do OpenGL-accelerated rendering",
                             static_cast<GParamFlags>(G_PARAM_WRITABLE)));
}

void GstQtGLVideoSink::init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);

    GstQtVideoSinkBase *sinkBase = GST_QT_VIDEO_SINK_BASE(instance);
    sinkBase->delegate = new QtVideoSinkDelegate(GST_ELEMENT(sinkBase));
}

//------------------------------

void GstQtGLVideoSink::set_property(GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sinkBase = GST_QT_VIDEO_SINK_BASE(object);

    switch (prop_id) {
    case PROP_GLCONTEXT:
        sinkBase->delegate->setGLContext(static_cast<QGLContext*>(g_value_get_pointer(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

//------------------------------

void GstQtGLVideoSink::paint(GstQtGLVideoSink *sink, gpointer painter,
                           qreal x, qreal y, qreal width, qreal height)
{
    GST_QT_VIDEO_SINK_BASE(sink)->delegate->paint(static_cast<QPainter*>(painter),
                                                  QRectF(x, y, width, height));
}
