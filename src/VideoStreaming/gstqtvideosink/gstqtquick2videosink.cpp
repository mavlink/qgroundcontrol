/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). <qt-info@nokia.com>
    Copyright (C) 2011-2013 Collabora Ltd. <info@collabora.com>

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

#include "gstqtquick2videosink.h"
#include "gstqtvideosinkplugin.h"
#include "gstqtvideosinkmarshal.h"
#include "delegates/qtquick2videosinkdelegate.h"

#include <gst/video/colorbalance.h>

#include <cstring>
#include <QCoreApplication>

#define CAPS_FORMATS "{ BGRA, BGRx, ARGB, xRGB, RGB, RGB16, BGR, v308, AYUV, YV12, I420 }"

#define GST_QT_QUICK2_VIDEO_SINK_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_QT_QUICK2_VIDEO_SINK, GstQtQuick2VideoSinkPrivate))

struct _GstQtQuick2VideoSinkPrivate
{
    QtQuick2VideoSinkDelegate *delegate;
    GList *channels_list;
};

static void gst_qt_quick2_video_sink_colorbalance_init (GstColorBalanceInterface * iface, gpointer data);

#define parent_class gst_qt_quick2_video_sink_parent_class
G_DEFINE_TYPE_WITH_CODE (GstQtQuick2VideoSink, gst_qt_quick2_video_sink,
        GST_TYPE_VIDEO_SINK,
        G_IMPLEMENT_INTERFACE (GST_TYPE_COLOR_BALANCE,
                gst_qt_quick2_video_sink_colorbalance_init));

enum {
    PROP_0,
    PROP_PIXEL_ASPECT_RATIO,
    PROP_FORCE_ASPECT_RATIO,
    PROP_CONTRAST,
    PROP_BRIGHTNESS,
    PROP_HUE,
    PROP_SATURATION,
};

enum {
    ACTION_UPDATE_NODE,
    SIGNAL_UPDATE,
    LAST_SIGNAL
};

static guint s_signals[LAST_SIGNAL] = { 0 };

const char * const s_colorbalance_labels[] = {
    "contrast", "brightness", "hue", "saturation"
};

//index for s_colorbalance_labels
enum {
    LABEL_CONTRAST = 0,
    LABEL_BRIGHTNESS,
    LABEL_HUE,
    LABEL_SATURATION,
    LABEL_LAST
};

static void
gst_qt_quick2_video_sink_init (GstQtQuick2VideoSink *self)
{
    self->priv = GST_QT_QUICK2_VIDEO_SINK_GET_PRIVATE (self);

    // delegate
    self->priv->delegate = new QtQuick2VideoSinkDelegate(GST_ELEMENT(self));

    // colorbalance
    GstColorBalanceChannel *channel;
    self->priv->channels_list = NULL;

    for (int i=0; i < LABEL_LAST; i++) {
        channel = GST_COLOR_BALANCE_CHANNEL(g_object_new(GST_TYPE_COLOR_BALANCE_CHANNEL, NULL));
        channel->label = g_strdup(s_colorbalance_labels[i]);
        channel->min_value = -100;
        channel->max_value = 100;

        self->priv->channels_list = g_list_append(self->priv->channels_list, channel);
    }
}

static void
gst_qt_quick2_video_sink_finalize (GObject *gobject)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (gobject);

    delete self->priv->delegate;
    self->priv->delegate = 0;

    while (self->priv->channels_list) {
        GstColorBalanceChannel *channel =
            GST_COLOR_BALANCE_CHANNEL(self->priv->channels_list->data);
        g_object_unref(channel);
        self->priv->channels_list = g_list_next(self->priv->channels_list);
    }

    g_list_free(self->priv->channels_list);

    G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
gst_qt_quick2_video_sink_set_property (GObject *object,
                                       guint property_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (object);

    switch (property_id) {
    case PROP_PIXEL_ASPECT_RATIO:
      {
        GValue tmp;
        std::memset(&tmp, 0, sizeof(GValue));
        g_value_init(&tmp, GST_TYPE_FRACTION);
        if (g_value_transform(value, &tmp)) {
            int n = gst_value_get_fraction_numerator(&tmp);
            int d = gst_value_get_fraction_denominator(&tmp);
            self->priv->delegate->setPixelAspectRatio(Fraction(n, d));
        } else {
            GST_WARNING_OBJECT(object, "Could not transform string to aspect ratio");
        }
        g_value_unset(&tmp);
        break;
      }
    case PROP_FORCE_ASPECT_RATIO:
        self->priv->delegate->setForceAspectRatio(g_value_get_boolean(value));
        break;
    case PROP_CONTRAST:
        self->priv->delegate->setContrast(g_value_get_int(value));
        break;
    case PROP_BRIGHTNESS:
        self->priv->delegate->setBrightness(g_value_get_int(value));
        break;
    case PROP_HUE:
        self->priv->delegate->setHue(g_value_get_int(value));
        break;
    case PROP_SATURATION:
        self->priv->delegate->setSaturation(g_value_get_int(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gst_qt_quick2_video_sink_get_property (GObject *object,
                                       guint property_id,
                                       GValue *value,
                                       GParamSpec *pspec)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (object);

    switch (property_id) {
    case PROP_PIXEL_ASPECT_RATIO:
      {
        GValue tmp;
        Fraction par = self->priv->delegate->pixelAspectRatio();
        std::memset(&tmp, 0, sizeof(GValue));
        g_value_init(&tmp, GST_TYPE_FRACTION);
        gst_value_set_fraction(&tmp, par.numerator, par.denominator);
        g_value_transform(&tmp, value);
        g_value_unset(&tmp);
        break;
      }
    case PROP_FORCE_ASPECT_RATIO:
        g_value_set_boolean(value, self->priv->delegate->forceAspectRatio());
        break;
    case PROP_CONTRAST:
        g_value_set_int(value, self->priv->delegate->contrast());
        break;
    case PROP_BRIGHTNESS:
        g_value_set_int(value, self->priv->delegate->brightness());
        break;
    case PROP_HUE:
        g_value_set_int(value, self->priv->delegate->hue());
        break;
    case PROP_SATURATION:
        g_value_set_int(value, self->priv->delegate->saturation());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static GstStateChangeReturn
gst_qt_quick2_video_sink_change_state(GstElement *element,
                                      GstStateChange transition)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (element);

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        self->priv->delegate->setActive(true);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        self->priv->delegate->setActive(false);
        break;
    default:
        break;
    }

    return GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
}

static gboolean
gst_qt_quick2_video_sink_set_caps(GstBaseSink *sink, GstCaps *caps)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (sink);

    GST_LOG_OBJECT(self, "new caps %" GST_PTR_FORMAT, caps);
    BufferFormat format = BufferFormat::fromCaps(caps);

    //too lazy to do proper checks. if the format is not UNKNOWN, then
    //it should conform to the template caps formats, unless gstreamer
    //core has a bug.
    if (format.videoFormat() != GST_VIDEO_FORMAT_UNKNOWN) {
        QCoreApplication::postEvent(self->priv->delegate,
                                    new BaseDelegate::BufferFormatEvent(format));
        return TRUE;
    } else {
        return FALSE;
    }
}

static GstFlowReturn
gst_qt_quick2_video_sink_show_frame(GstVideoSink *sink, GstBuffer *buffer)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (sink);

    GST_TRACE_OBJECT(self, "Posting new buffer (%" GST_PTR_FORMAT ") for rendering.", buffer);

    QCoreApplication::postEvent(self->priv->delegate, new BaseDelegate::BufferEvent(buffer));

    return GST_FLOW_OK;
}

//------------------------------

static gpointer
gst_qt_quick2_video_sink_update_node(GstQtQuick2VideoSink *self, gpointer node,
                                     qreal x, qreal y, qreal w, qreal h)
{
      return self->priv->delegate->updateNode(static_cast<QSGNode*>(node),
                                              QRectF(x, y, w, h));
}

//------------------------------

static const GList *
gst_qt_quick2_video_sink_colorbalance_list_channels(GstColorBalance *balance)
{
    return GST_QT_QUICK2_VIDEO_SINK (balance)->priv->channels_list;
}

static void
gst_qt_quick2_video_sink_colorbalance_set_value(GstColorBalance *balance,
                                                  GstColorBalanceChannel *channel, gint value)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (balance);

    if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_CONTRAST])) {
        self->priv->delegate->setContrast(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_BRIGHTNESS])) {
        self->priv->delegate->setBrightness(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_HUE])) {
        self->priv->delegate->setHue(value);
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_SATURATION])) {
        self->priv->delegate->setSaturation(value);
    } else {
        GST_WARNING_OBJECT(self, "Unknown colorbalance channel %s", channel->label);
    }
}

static gint
gst_qt_quick2_video_sink_colorbalance_get_value(GstColorBalance *balance,
                                                GstColorBalanceChannel *channel)
{
    GstQtQuick2VideoSink *self = GST_QT_QUICK2_VIDEO_SINK (balance);

    if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_CONTRAST])) {
        return self->priv->delegate->contrast();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_BRIGHTNESS])) {
        return self->priv->delegate->brightness();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_HUE])) {
        return self->priv->delegate->hue();
    } else if (!qstrcmp(channel->label, s_colorbalance_labels[LABEL_SATURATION])) {
        return self->priv->delegate->saturation();
    } else {
        GST_WARNING_OBJECT(self, "Unknown colorbalance channel %s", channel->label);
    }

    return 0;
}

static GstColorBalanceType
gst_qt_quick2_video_sink_colorbalance_get_balance_type (GstColorBalance * balance)
{
    Q_UNUSED(balance);
    return GST_COLOR_BALANCE_HARDWARE;
}

static void
gst_qt_quick2_video_sink_colorbalance_init(GstColorBalanceInterface *iface, gpointer data)
{
    Q_UNUSED(data);
    iface->list_channels = gst_qt_quick2_video_sink_colorbalance_list_channels;
    iface->set_value = gst_qt_quick2_video_sink_colorbalance_set_value;
    iface->get_value = gst_qt_quick2_video_sink_colorbalance_get_value;
    iface->get_balance_type = gst_qt_quick2_video_sink_colorbalance_get_balance_type;
}

//------------------------------

static void
gst_qt_quick2_video_sink_class_init (GstQtQuick2VideoSinkClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = gst_qt_quick2_video_sink_finalize;
    gobject_class->set_property = gst_qt_quick2_video_sink_set_property;
    gobject_class->get_property = gst_qt_quick2_video_sink_get_property;

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    element_class->change_state = gst_qt_quick2_video_sink_change_state;

    GstBaseSinkClass *base_sink_class = GST_BASE_SINK_CLASS(klass);
    base_sink_class->set_caps = gst_qt_quick2_video_sink_set_caps;

    GstVideoSinkClass *video_sink_class = GST_VIDEO_SINK_CLASS(klass);
    video_sink_class->show_frame = gst_qt_quick2_video_sink_show_frame;

    GstQtQuick2VideoSinkClass *qtquick2_class = GST_QT_QUICK2_VIDEO_SINK_CLASS(klass);
    qtquick2_class->update_node = gst_qt_quick2_video_sink_update_node;

    /**
     * GstQtQuick2VideoSink::pixel-aspect-ratio
     *
     * The pixel aspect ratio of the display device.
     **/
    g_object_class_install_property(gobject_class, PROP_PIXEL_ASPECT_RATIO,
        g_param_spec_string("pixel-aspect-ratio", "Pixel aspect ratio",
                            "The pixel aspect ratio of the display device",
                            "1/1", static_cast<GParamFlags>(G_PARAM_READWRITE)));

    /**
     * GstQtQuick2VideoSink::force-aspect-ratio
     *
     * If set to TRUE, the sink will scale the video respecting its original aspect ratio
     * and any remaining space will be filled with black.
     * If set to FALSE, the sink will scale the video to fit the whole drawing area.
     **/
    g_object_class_install_property(gobject_class, PROP_FORCE_ASPECT_RATIO,
        g_param_spec_boolean("force-aspect-ratio", "Force aspect ratio",
                             "When enabled, scaling will respect original aspect ratio",
                             FALSE, static_cast<GParamFlags>(G_PARAM_READWRITE)));

    g_object_class_install_property(gobject_class, PROP_CONTRAST,
        g_param_spec_int("contrast", "Contrast", "The contrast of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));

    g_object_class_install_property(gobject_class, PROP_BRIGHTNESS,
        g_param_spec_int("brightness", "Brightness", "The brightness of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));

    g_object_class_install_property(gobject_class, PROP_HUE,
        g_param_spec_int("hue", "Hue", "The hue of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));

    g_object_class_install_property(gobject_class, PROP_SATURATION,
        g_param_spec_int("saturation", "Saturation", "The saturation of the video",
                         -100, 100, 0, static_cast<GParamFlags>(G_PARAM_READWRITE)));


    /**
     * GstQtQuick2VideoSink::update-node
     * @node: The QSGNode to update
     * @x: The x coordinate of the target area rectangle
     * @y: The y coordinate of the target area rectangle
     * @width: The width of the target area rectangle
     * @height: The height of the target area rectangle
     * @returns: The updated QGSNode
     *
     * This is an action signal that you can call from your QQuickItem subclass
     * inside its updateNode function to render the video. It takes a QSGNode*
     * and the item's area rectangle as arguments. You should schedule to call
     * this function to repaint the surface whenever the ::update signal is
     * emitted.
     *
     * Note that the x,y,width and height arguments are actually qreal.
     * This means that on architectures like arm they will be float instead
     * of double. You should cast the arguments to qreal if they are not
     * already when emitting this signal.
     */

    s_signals[ACTION_UPDATE_NODE] =
        g_signal_new("update-node", G_TYPE_FROM_CLASS(klass),
            static_cast<GSignalFlags>(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
            G_STRUCT_OFFSET(GstQtQuick2VideoSinkClass, update_node),
            NULL, NULL,
            qRealIsDouble() ?
              g_cclosure_user_marshal_POINTER__POINTER_DOUBLE_DOUBLE_DOUBLE_DOUBLE :
              g_cclosure_user_marshal_POINTER__POINTER_FLOAT_FLOAT_FLOAT_FLOAT,
            G_TYPE_POINTER, 5,
            G_TYPE_POINTER, G_TYPE_QREAL, G_TYPE_QREAL, G_TYPE_QREAL, G_TYPE_QREAL);

    /**
     * GstQtQuick2VideoSink::update
     *
     * This signal is emitted when the surface should be repainted. It should
     * be connected to QQuickItem::update().
     */
    s_signals[SIGNAL_UPDATE] =
        g_signal_new("update", G_TYPE_FROM_CLASS(klass),
            G_SIGNAL_RUN_LAST,
            0, NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);

    g_type_class_add_private (klass, sizeof (GstQtQuick2VideoSinkPrivate));

    static GstStaticPadTemplate sink_pad_template =
        GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
            GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (CAPS_FORMATS))
        );

    gst_element_class_add_pad_template(
            element_class, gst_static_pad_template_get(&sink_pad_template));

    gst_element_class_set_details_simple(element_class,
        "QtQuick2 video sink", "Sink/Video",
        "A video sink that can draw on a QQuickItem",
        "George Kiagiadakis <george.kiagiadakis@collabora.com>");
}
