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

#include "gstqtglvideosinkbase.h"
#include "painters/openglsurfacepainter.h"
#include "delegates/qtvideosinkdelegate.h"
#include <QCoreApplication>

#define CAPS_FORMATS "{ BGRA, BGRx, ARGB, xRGB, RGB, RGB16, BGR, v308, AYUV, YV12, I420 }"

const char * const GstQtGLVideoSinkBase::s_colorbalance_labels[] = {
    "contrast", "brightness", "hue", "saturation"
};

GstQtVideoSinkBaseClass *GstQtGLVideoSinkBase::s_parent_class = 0;

//------------------------------

DEFINE_TYPE_WITH_CODE(GstQtGLVideoSinkBase, GST_TYPE_QT_VIDEO_SINK_BASE, init_interfaces)

void GstQtGLVideoSinkBase::init_interfaces(GType type)
{
    static const GInterfaceInfo colorbalance_info = {
        (GInterfaceInitFunc) &GstQtGLVideoSinkBase::colorbalance_init, NULL, NULL
    };

    g_type_add_interface_static(type, GST_TYPE_COLOR_BALANCE, &colorbalance_info);
}

//------------------------------

void GstQtGLVideoSinkBase::base_init(gpointer g_class)
{
    GstElementClass *element_class = GST_ELEMENT_CLASS(g_class);
    element_class->padtemplates = NULL; //get rid of the pad template of the base class

    static GstStaticPadTemplate sink_pad_template =
        GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
            GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (CAPS_FORMATS))
        );

    gst_element_class_add_pad_template(
            element_class, gst_static_pad_template_get(&sink_pad_template));
}

void GstQtGLVideoSinkBase::class_init(gpointer g_class, gpointer class_data)
{
    Q_UNUSED(class_data);

    s_parent_class = reinterpret_cast<GstQtVideoSinkBaseClass*>(g_type_class_peek_parent(g_class));

    GObjectClass *object_class = G_OBJECT_CLASS(g_class);
    object_class->finalize = GstQtGLVideoSinkBase::finalize;
    object_class->set_property = GstQtGLVideoSinkBase::set_property;
    object_class->get_property = GstQtGLVideoSinkBase::get_property;

    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(g_class);
    base_sink_class->start = GstQtGLVideoSinkBase::start;
    base_sink_class->set_caps = GstQtGLVideoSinkBase::set_caps;

    g_object_class_install_property(object_class, PROP_CONTRAST,
        g_param_spec_int("contrast", "Contrast", "The contrast of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));
    g_object_class_install_property(object_class, PROP_BRIGHTNESS,
        g_param_spec_int("brightness", "Brightness", "The brightness of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));
    g_object_class_install_property(object_class, PROP_HUE,
        g_param_spec_int("hue", "Hue", "The hue of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));
    g_object_class_install_property(object_class, PROP_SATURATION,
        g_param_spec_int("saturation", "Saturation", "The saturation of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));
}

void GstQtGLVideoSinkBase::init(GTypeInstance *instance, gpointer g_class)
{
    Q_UNUSED(g_class);
    GstQtGLVideoSinkBase *self = GST_QT_GL_VIDEO_SINK_BASE(instance);

    GstColorBalanceChannel *channel;
    self->m_channels_list = NULL;

    for (int i=0; i < LABEL_LAST; i++) {
        channel = GST_COLOR_BALANCE_CHANNEL(g_object_new(GST_TYPE_COLOR_BALANCE_CHANNEL, NULL));
        channel->label = g_strdup(s_colorbalance_labels[i]);
        channel->min_value = -100;
        channel->max_value = 100;

        self->m_channels_list = g_list_append(self->m_channels_list, channel);
    }
}

void GstQtGLVideoSinkBase::finalize(GObject *object)
{
    GstQtGLVideoSinkBase *self = GST_QT_GL_VIDEO_SINK_BASE(object);

    while (self->m_channels_list) {
        GstColorBalanceChannel *channel =  GST_COLOR_BALANCE_CHANNEL(self->m_channels_list->data);
        g_object_unref(channel);
        self->m_channels_list = g_list_next(self->m_channels_list);
    }

    g_list_free(self->m_channels_list);

    G_OBJECT_CLASS(s_parent_class)->finalize(object);
}

//------------------------------


void GstQtGLVideoSinkBase::colorbalance_init(GstColorBalanceInterface *balance_interface, gpointer data)
{
    Q_UNUSED(data);
    balance_interface->list_channels = GstQtGLVideoSinkBase::colorbalance_list_channels;
    balance_interface->set_value = GstQtGLVideoSinkBase::colorbalance_set_value;
    balance_interface->get_value = GstQtGLVideoSinkBase::colorbalance_get_value;
    balance_interface->get_balance_type = GstQtGLVideoSinkBase::colorbalance_get_balance_type;
}

const GList *GstQtGLVideoSinkBase::colorbalance_list_channels(GstColorBalance *balance)
{
    return GST_QT_GL_VIDEO_SINK_BASE(balance)->m_channels_list;
}

void GstQtGLVideoSinkBase::colorbalance_set_value(GstColorBalance *balance,
                                                  GstColorBalanceChannel *channel, gint value)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(balance);

    if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_CONTRAST])) {
        sink->delegate->setContrast(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_BRIGHTNESS])) {
        sink->delegate->setBrightness(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_HUE])) {
        sink->delegate->setHue(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_SATURATION])) {
        sink->delegate->setSaturation(value);
    } else {
        GST_WARNING_OBJECT(sink, "Unknown colorbalance channel %s", channel->label);
    }
}

gint GstQtGLVideoSinkBase::colorbalance_get_value(GstColorBalance *balance,
                                                  GstColorBalanceChannel *channel)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(balance);

    if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_CONTRAST])) {
        return sink->delegate->contrast();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_BRIGHTNESS])) {
        return sink->delegate->brightness();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_HUE])) {
        return sink->delegate->hue();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_SATURATION])) {
        return sink->delegate->saturation();
    } else {
        GST_WARNING_OBJECT(sink, "Unknown colorbalance channel %s", channel->label);
    }

    return 0;
}

GstColorBalanceType GstQtGLVideoSinkBase::colorbalance_get_balance_type(GstColorBalance *balance)
{
    Q_UNUSED(balance);
    return GST_COLOR_BALANCE_HARDWARE;
}

//------------------------------

void GstQtGLVideoSinkBase::set_property(GObject *object, guint prop_id,
                                        const GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(object);

    switch (prop_id) {
    case PROP_CONTRAST:
        sink->delegate->setContrast(g_value_get_int(value));
        break;
    case PROP_BRIGHTNESS:
        sink->delegate->setBrightness(g_value_get_int(value));
        break;
    case PROP_HUE:
        sink->delegate->setHue(g_value_get_int(value));
        break;
    case PROP_SATURATION:
        sink->delegate->setSaturation(g_value_get_int(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void GstQtGLVideoSinkBase::get_property(GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(object);

    switch (prop_id) {
    case PROP_CONTRAST:
        g_value_set_int(value, sink->delegate->contrast());
        break;
    case PROP_BRIGHTNESS:
        g_value_set_int(value, sink->delegate->brightness());
        break;
    case PROP_HUE:
        g_value_set_int(value, sink->delegate->hue());
        break;
    case PROP_SATURATION:
        g_value_set_int(value, sink->delegate->saturation());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

//------------------------------

gboolean GstQtGLVideoSinkBase::start(GstBaseSink *base)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(base);

    //fail on purpose if the user hasn't set a context
    if (sink->delegate->supportedPainterTypes() == QtVideoSinkDelegate::Generic) {
        GST_WARNING_OBJECT(sink, "Neither GLSL nor ARB Fragment Program are supported "
                                 "for painting. Did you forget to set a gl context?");
        return FALSE;
    } else {
        return TRUE;
    }
}

gboolean GstQtGLVideoSinkBase::set_caps(GstBaseSink *base, GstCaps *caps)
{
    GstQtVideoSinkBase *sink = GST_QT_VIDEO_SINK_BASE(base);

    GST_LOG_OBJECT(sink, "new caps %" GST_PTR_FORMAT, caps);
    BufferFormat format = BufferFormat::fromCaps(caps);
    if (OpenGLSurfacePainter::supportedPixelFormats().contains(format.videoFormat())) {
        QCoreApplication::postEvent(sink->delegate,
                                    new BaseDelegate::BufferFormatEvent(format));
        return TRUE;
    } else {
        return FALSE;
    }
}
