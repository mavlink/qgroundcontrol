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

#include "gstqtvideosinkbase.h"
#include "delegates/qtvideosinkdelegate.h"
#include "painters/genericsurfacepainter.h"
#include <cstring>
#include <QCoreApplication>

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
# define CAPS_FORMATS "{ ARGB, xRGB, RGB, RGB16 }"
#else
# define CAPS_FORMATS "{ BGRA, BGRx, RGB, RGB16 }"
#endif

GstVideoSinkClass *GstQtVideoSinkBase::s_parent_class = NULL;

DEFINE_TYPE(GstQtVideoSinkBase, GST_TYPE_VIDEO_SINK)

//------------------------------

void GstQtVideoSinkBase::base_init(gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);

    static GstStaticPadTemplate sink_pad_template =
        GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
            GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (CAPS_FORMATS))
        );

    gst_element_class_add_pad_template(
            element_class, gst_static_pad_template_get(&sink_pad_template));
}

void GstQtVideoSinkBase::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    s_parent_class = reinterpret_cast<GstVideoSinkClass*>(g_type_class_peek_parent(g_class));

    GObjectClass *object_class = G_OBJECT_CLASS(g_class);
    object_class->finalize = GstQtVideoSinkBase::finalize;
    object_class->set_property = GstQtVideoSinkBase::set_property;
    object_class->get_property = GstQtVideoSinkBase::get_property;

    GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);
    element_class->change_state = GstQtVideoSinkBase::change_state;

    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(g_class);
    base_sink_class->set_caps = GstQtVideoSinkBase::set_caps;

    GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS(g_class);
    video_sink_class->show_frame = GstQtVideoSinkBase::show_frame;

    /**
     * GstQtVideoSinkBase::pixel-aspect-ratio
     *
     * The pixel aspect ratio of the display device.
     **/
    g_object_class_install_property(object_class, PROP_PIXEL_ASPECT_RATIO,
        g_param_spec_string("pixel-aspect-ratio", "Pixel aspect ratio",
                            "The pixel aspect ratio of the display device",
                            "1/1", static_cast<GParamFlags>(G_PARAM_READWRITE)));

    /**
     * GstQtVideoSinkBase::force-aspect-ratio
     *
     * If set to TRUE, the sink will scale the video respecting its original aspect ratio
     * and any remaining space will be filled with black.
     * If set to FALSE, the sink will scale the video to fit the whole drawing area.
     **/
    g_object_class_install_property(object_class, PROP_FORCE_ASPECT_RATIO,
        g_param_spec_boolean("force-aspect-ratio", "Force aspect ratio",
                             "When enabled, scaling will respect original aspect ratio",
                             FALSE, static_cast<GParamFlags>(G_PARAM_READWRITE)));

}

void GstQtVideoSinkBase::init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(instance);
    Q_UNUSED(g_class);

    /* sink->delegate is initialized in the subclasses */
}

void GstQtVideoSinkBase::finalize(GObject *object)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(object);

    delete sink->delegate;
    sink->delegate = 0;
}

//------------------------------

void GstQtVideoSinkBase::set_property(GObject *object, guint prop_id,
                                      const GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(object);

    switch (prop_id) {
    case PROP_PIXEL_ASPECT_RATIO:
      {
        GValue tmp;
        std::memset(&tmp, 0, sizeof(GValue));
        g_value_init(&tmp, GST_TYPE_FRACTION);
        if (g_value_transform(value, &tmp)) {
            int n = gst_value_get_fraction_numerator(&tmp);
            int d = gst_value_get_fraction_denominator(&tmp);
            sink->delegate->setPixelAspectRatio(Fraction(n, d));
        } else {
            GST_WARNING_OBJECT(object, "Could not transform string to aspect ratio");
        }
        g_value_unset(&tmp);
        break;
      }
    case PROP_FORCE_ASPECT_RATIO:
        sink->delegate->setForceAspectRatio(g_value_get_boolean(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void GstQtVideoSinkBase::get_property(GObject *object, guint prop_id,
                                      GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(object);

    switch (prop_id) {
    case PROP_PIXEL_ASPECT_RATIO:
      {
        GValue tmp;
        Fraction par = sink->delegate->pixelAspectRatio();
        std::memset(&tmp, 0, sizeof(GValue));
        g_value_init(&tmp, GST_TYPE_FRACTION);
        gst_value_set_fraction(&tmp, par.numerator, par.denominator);
        g_value_transform(&tmp, value);
        g_value_unset(&tmp);
        break;
      }
    case PROP_FORCE_ASPECT_RATIO:
        g_value_set_boolean(value, sink->delegate->forceAspectRatio());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

//------------------------------

GstStateChangeReturn GstQtVideoSinkBase::change_state(GstElement *element, GstStateChange transition)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(element);

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        sink->delegate->setActive(true);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        sink->delegate->setActive(false);
        break;
    default:
        break;
    }

    return GST_ELEMENT_CLASS(s_parent_class)->change_state(element, transition);
}

//------------------------------

gboolean GstQtVideoSinkBase::set_caps(GstBaseSink *base, GstCaps *caps)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(base);

    GST_LOG_OBJECT(sink, "new caps %" GST_PTR_FORMAT, caps);
    BufferFormat format = BufferFormat::fromCaps(caps);
    if (GenericSurfacePainter::supportedPixelFormats().contains(format.videoFormat())) {
        QCoreApplication::postEvent(sink->delegate,
                                    new BaseDelegate::BufferFormatEvent(format));
        return TRUE;
    } else {
        return FALSE;
    }
}

//------------------------------

GstFlowReturn GstQtVideoSinkBase::show_frame(GstVideoSink *video_sink, GstBuffer *buffer)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(video_sink);

    GST_TRACE_OBJECT(sink, "Posting new buffer (%" GST_PTR_FORMAT") for rendering.", buffer);

    QCoreApplication::postEvent(sink->delegate, new BaseDelegate::BufferEvent(buffer));

    return GST_FLOW_OK;
}
