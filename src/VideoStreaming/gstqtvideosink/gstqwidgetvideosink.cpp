/*
    Copyright (C) 2010 George Kiagiadakis <kiagiadakis.george@gmail.com>
    Copyright (C) 2012 Collabora Ltd. <info@collabora.com>

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

#include "gstqwidgetvideosink.h"
#include "delegates/qwidgetvideosinkdelegate.h"

DEFINE_TYPE(GstQWidgetVideoSink, GST_TYPE_QT_VIDEO_SINK_BASE)

//------------------------------

void GstQWidgetVideoSink::base_init(gpointer gclass)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

    gst_element_class_set_details_simple(element_class, "QWidget video sink", "Sink/Video",
        "A video sink that draws on a QWidget using QPainter",
        "George Kiagiadakis <george.kiagiadakis@collabora.com>");
}

void GstQWidgetVideoSink::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
    gobject_class->set_property = GstQWidgetVideoSink::set_property;
    gobject_class->get_property = GstQWidgetVideoSink::get_property;

    /**
     * GstQWidgetVideoSink::widget
     *
     * This property holds a pointer to the QWidget on which the sink will paint the video.
     * You can set this property at any time, even if the element is in PLAYING
     * state. You can also set this property to NULL at any time to release
     * the widget. In this case, qwidgetvideosink will behave like a fakesink,
     * i.e. it will silently drop all the frames that it receives. It is also safe
     * to delete the widget that has been set as this property; the sink will be
     * signaled and this property will automatically be set to NULL.
     **/
    g_object_class_install_property(gobject_class, PROP_WIDGET,
        g_param_spec_pointer("widget", "Widget",
                             "The widget on which this element will paint the video",
                             static_cast<GParamFlags>(G_PARAM_READWRITE)));
}

void GstQWidgetVideoSink::init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);

    GstQtVideoSinkBase *sinkBase = GST_QT_VIDEO_SINK_BASE(instance);
    sinkBase->delegate = new QWidgetVideoSinkDelegate(GST_ELEMENT(sinkBase));
}

void GstQWidgetVideoSink::set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sinkBase = GST_QT_VIDEO_SINK_BASE(object);
    QWidgetVideoSinkDelegate *delegate = static_cast<QWidgetVideoSinkDelegate*>(sinkBase->delegate);

    switch (prop_id) {
    case PROP_WIDGET:
        delegate->setWidget(static_cast<QWidget*>(g_value_get_pointer(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void GstQWidgetVideoSink::get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sinkBase = GST_QT_VIDEO_SINK_BASE(object);
    QWidgetVideoSinkDelegate *delegate = static_cast<QWidgetVideoSinkDelegate*>(sinkBase->delegate);

    switch (prop_id) {
    case PROP_WIDGET:
        g_value_set_pointer(value, delegate->widget());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}
