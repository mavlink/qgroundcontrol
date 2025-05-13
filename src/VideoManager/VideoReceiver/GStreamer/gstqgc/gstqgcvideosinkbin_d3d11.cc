/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "gstqgcvideosinkbin_d3d11.h"
#include "gstqgcelements.h"

#include <gst/d3d11/gstd3d11.h>

#define GST_CAT_DEFAULT gst_qgc_video_sink_bin_d3d11_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

#define DEFAULT_FORCE_ASPECT_RATIO TRUE

#define PROP_WIDGET_NAME                "widget"
#define PROP_FORCE_ASPECT_RATIO_NAME    "force-aspect-ratio"

enum
{
    PROP_0,
    PROP_WIDGET,
    PROP_FORCE_ASPECT_RATIO,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

enum
{
    SIGNAL_0,
    SIGNAL_LAST
};

static guint gst_qgc_video_sink_bin_d3d11_signals[SIGNAL_LAST] = { 0 };

#define gst_qgc_video_sink_bin_d3d11_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstQgcVideoSinkBinD3d11,
    gst_qgc_video_sink_bin_d3d11,
    GST_TYPE_BIN,
    GST_DEBUG_CATEGORY_INIT(
        GST_CAT_DEFAULT,
        "qgcsinkbin",
        0,
        "QGC Video Sink Bin"));

GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(qgcvideosinkbind3d11,"qgcvideosinkbind3d11",
                                      GST_RANK_NONE,
                                      GST_TYPE_QGC_VIDEO_SINK_BIN_D3D11,
                                      qgc_element_init(plugin));

static void gst_qgc_video_sink_bin_d3d11_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_d3d11_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_d3d11_dispose(GObject *object);
static void gst_qgc_video_sink_bin_d3d11_finalize(GObject *object);

static void
gst_qgc_video_sink_bin_d3d11_class_init(GstQgcVideoSinkBinD3d11Class *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    object_class->set_property = gst_qgc_video_sink_bin_d3d11_set_property;
    object_class->get_property = gst_qgc_video_sink_bin_d3d11_get_property;
    object_class->dispose = gst_qgc_video_sink_bin_d3d11_dispose;
    object_class->finalize = gst_qgc_video_sink_bin_d3d11_finalize;

    properties[PROP_WIDGET] = g_param_spec_pointer(
        "widget", "QQuickItem",
        "Owning QML item – handed off to qml6d3d11sink",
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_FORCE_ASPECT_RATIO] = g_param_spec_boolean(
        "force-aspect-ratio", "Force aspect ratio",
        "Maintain original pixel aspect during scaling",
        DEFAULT_FORCE_ASPECT_RATIO,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    g_object_class_install_properties(object_class, PROP_LAST, properties);

    gst_element_class_set_static_metadata(element_class,
        "QGC Video Sink Bin", "Sink/Video/Bin",
        "D3D11 accelerated video sink wrapper used by QGroundControl",
        "QGroundControl team"
    );
}

static void
gst_qgc_video_sink_bin_d3d11_init(GstQgcVideoSinkBinD3d11 *self)
{
    self->d3d11upload = gst_element_factory_make("d3d11upload", NULL);
    if (!self->d3d11upload) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('d3d11upload') failed");
        return;
    }

    self->d3d11colorconvert = gst_element_factory_make("d3d11colorconvert", NULL);
    if (!self->d3d11colorconvert) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('d3d11colorconvert') failed");
        return;
    }

    self->qml6d3d11sink = gst_element_factory_make("qml6d3d11sink", NULL);
    if (!self->qml6d3d11sink) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('qml6d3d11sink') failed");
        return;
    }

    g_return_if_fail(gst_bin_add_many(GST_BIN(self), self->d3d11upload, self->d3d11colorconvert, self->qml6d3d11sink, NULL));
    g_return_if_fail(gst_element_link_many(self->d3d11upload, self->d3d11colorconvert, self->qml6d3d11sink, NULL));

    gst_element_sync_state_with_parent(self->d3d11upload);
    gst_element_sync_state_with_parent(self->d3d11colorconvert);
}

static void
gst_qgc_video_sink_bin_d3d11_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBinD3d11 *self = GST_QGC_VIDEO_SINK_BIN_D3D11(object);

    switch (prop_id) {
    case PROP_WIDGET:
        g_object_set(self->qml6d3d11sink,
                     PROP_WIDGET_NAME,
                     g_value_get_pointer(value),
                     NULL);
        break;
    case PROP_FORCE_ASPECT_RATIO:
        g_object_set(self->qml6d3d11sink,
                     PROP_FORCE_ASPECT_RATIO_NAME,
                     g_value_get_boolean(value),
                     NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_d3d11_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBinD3d11 *self = GST_QGC_VIDEO_SINK_BIN_D3D11(object);

    switch (prop_id) {
    case PROP_WIDGET: {
        gpointer widget = NULL;
        g_object_get(self->qml6d3d11sink,
                     PROP_WIDGET_NAME,
                     &widget,
                     NULL);
        g_value_set_pointer(value, widget);
        break;
    }
    case PROP_FORCE_ASPECT_RATIO: {
        gboolean enable = FALSE;
        g_object_get(self->qml6d3d11sink,
                     PROP_FORCE_ASPECT_RATIO_NAME,
                     &enable,
                     NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_d3d11_dispose(GObject *object)
{
    GstQgcVideoSinkBinD3d11 *self = GST_QGC_VIDEO_SINK_BIN_D3D11(object);

    (void) self;

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
gst_qgc_video_sink_bin_d3d11_finalize(GObject *object)
{
    GstQgcVideoSinkBinD3d11 *self = GST_QGC_VIDEO_SINK_BIN_D3D11(object);

    (void) self;

    G_OBJECT_CLASS(parent_class)->finalize(object);
}
