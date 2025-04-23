/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "gstqgcvideosinkbin.h"
#include "gstqgcelements.h"

#include <gst/gl/gstglmemory.h>
#include <gst/gsterror.h>

#define GST_CAT_DEFAULT gst_qgc_video_sink_bin_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

#define DEFAULT_ENABLE_LAST_SAMPLE FALSE
#define DEFAULT_FORCE_ASPECT_RATIO TRUE
#define DEFAULT_PAR_N 0
#define DEFAULT_PAR_D 1
#define DEFAULT_SYNC TRUE

#define PROP_ENABLE_LAST_SAMPLE_NAME    "enable-last-sample"
#define PROP_LAST_SAMPLE_NAME           "last-sample"
#define PROP_WIDGET_NAME                "widget"
#define PROP_FORCE_ASPECT_RATIO_NAME    "force-aspect-ratio"
#define PROP_PIXEL_ASPECT_RATIO_NAME    "pixel-aspect-ratio"
#define PROP_SYNC_NAME                  "sync"

enum
{
    PROP_0,
    PROP_ENABLE_LAST_SAMPLE,
    PROP_LAST_SAMPLE,
    PROP_WIDGET,
    PROP_FORCE_ASPECT_RATIO,
    PROP_PIXEL_ASPECT_RATIO,
    PROP_SYNC,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

enum
{
    SIGNAL_0,
    LAST_SIGNAL
};

static guint gst_qgc_video_sink_bin_signals[LAST_SIGNAL] = { 0 };

static GstStaticPadTemplate sink_pad_template =
    GST_STATIC_PAD_TEMPLATE("sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS(
            "video/x-raw(" GST_CAPS_FEATURE_MEMORY_GL_MEMORY "), "
            "format = (string) { RGBA, BGRA, RGB, YV12, NV12 }, "
            "width = " GST_VIDEO_SIZE_RANGE ", "
            "height = " GST_VIDEO_SIZE_RANGE ", "
            "framerate = " GST_VIDEO_FPS_RANGE ", "
            "texture-target = (string) { " GST_GL_TEXTURE_TARGET_2D_STR ", "
                                           GST_GL_TEXTURE_TARGET_EXTERNAL_OES_STR " } "));

#define gst_qgc_video_sink_bin_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(
    GstQgcVideoSinkBin,
    gst_qgc_video_sink_bin,
    GST_TYPE_BIN,
    GST_DEBUG_CATEGORY_INIT(
        GST_CAT_DEFAULT,
        "qgcsinkbin",
        0,
        "QGC Video Sink Bin"));

GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(qgcvideosinkbin,"qgcvideosinkbin",
                                      GST_RANK_NONE,
                                      GST_TYPE_QGC_VIDEO_SINK_BIN,
                                      qgc_element_init(plugin));

static void gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_dispose(GObject *object);
static void gst_qgc_video_sink_bin_finalize(GObject *object);

static void
gst_qgc_video_sink_bin_class_init(GstQgcVideoSinkBinClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    object_class->set_property = gst_qgc_video_sink_bin_set_property;
    object_class->get_property = gst_qgc_video_sink_bin_get_property;
    object_class->dispose = gst_qgc_video_sink_bin_dispose;
    object_class->finalize = gst_qgc_video_sink_bin_finalize;

    properties[PROP_ENABLE_LAST_SAMPLE] = g_param_spec_boolean(
        "enable-last-sample", "Enable last sample",
        "Retain the most recent buffer for UI snapshotting",
        DEFAULT_ENABLE_LAST_SAMPLE,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties[PROP_LAST_SAMPLE] = g_param_spec_boxed(
        "last-sample", "Last sample",
        "Last preroll/played sample held by the sink",
        GST_TYPE_SAMPLE,
        (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    properties[PROP_WIDGET] = g_param_spec_pointer(
        "widget", "QQuickItem",
        "Owning QML item – handed off to qml6glsink",
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties[PROP_FORCE_ASPECT_RATIO] = g_param_spec_boolean(
        "force-aspect-ratio", "Force aspect ratio",
        "Maintain original pixel aspect during scaling",
        DEFAULT_FORCE_ASPECT_RATIO,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties[PROP_PIXEL_ASPECT_RATIO] = gst_param_spec_fraction(
        "pixel-aspect-ratio", "Pixel aspect ratio",
        "Pixel aspect ratio of the display surface",
        DEFAULT_PAR_N, DEFAULT_PAR_D,
        G_MAXINT, 1,
        1, 1,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    properties[PROP_SYNC] =
        g_param_spec_boolean("sync", "Sync",
                             "Synchronise frame presentation to the pipeline clock",
                             DEFAULT_SYNC,
                             (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(object_class, PROP_LAST, properties);

    gst_element_class_add_static_pad_template(element_class, &sink_pad_template);

    gst_element_class_set_static_metadata(element_class,
        "QGC Video Sink Bin", "Sink/Video/Bin",
        "GL accelerated video sink wrapper used by QGroundControl",
        "QGroundControl team");
}

static void
gst_qgc_video_sink_bin_init(GstQgcVideoSinkBin *self)
{
    self->glsinkbin = gst_element_factory_make("glsinkbin", NULL);
    if (!self->glsinkbin) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('glsinkbin') failed");
        return;
    }

    self->qmlglsink = gst_element_factory_make("qml6glsink", NULL);
    if (!self->qmlglsink) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('qml6glsink') failed");
        return;
    }

    g_object_set(self->glsinkbin,
                 "sink", self->qmlglsink,
                 PROP_ENABLE_LAST_SAMPLE_NAME, FALSE,
                 NULL);

    g_return_if_fail(gst_bin_add(GST_BIN(self), self->glsinkbin));

    GstPad *sinkpad = gst_element_get_static_pad(self->glsinkbin, "sink");
    if (!sinkpad) {
        GST_ERROR_OBJECT(self, "gst_element_get_static_pad('sink') failed");
        return;
    }

    GstPad *ghostpad = gst_ghost_pad_new("sink", sinkpad);
    if (!ghostpad) {
        GST_ERROR_OBJECT(self, "gst_ghost_pad_new('sink') failed");
        gst_object_unref(sinkpad);
        return;
    }

    if (!gst_element_add_pad(GST_ELEMENT(self), ghostpad)) {
        GST_ERROR_OBJECT(self, "gst_element_add_pad() failed");
    }

    if (sinkpad) {
        gst_object_unref(sinkpad);
    }
}

static void
gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        g_object_set(self->glsinkbin,
                     PROP_ENABLE_LAST_SAMPLE_NAME,
                     g_value_get_boolean(value),
                     NULL);
        break;
    case PROP_WIDGET:
        g_object_set(self->qmlglsink,
                     PROP_WIDGET_NAME,
                     g_value_get_pointer(value),
                     NULL);
        break;
    case PROP_FORCE_ASPECT_RATIO:
        g_object_set(self->glsinkbin,
                     PROP_FORCE_ASPECT_RATIO_NAME,
                     g_value_get_boolean(value),
                     NULL);
        break;
    case PROP_PIXEL_ASPECT_RATIO: {
        const gint num = gst_value_get_fraction_numerator(value);
        const gint den = gst_value_get_fraction_denominator(value);
        g_object_set(self->qmlglsink,
                     PROP_PIXEL_ASPECT_RATIO_NAME,
                     num,
                     den,
                     NULL);
        break;
    }
    case PROP_SYNC:
        g_object_set(self->glsinkbin,
                     PROP_SYNC_NAME,
                     g_value_get_boolean(value),
                     NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE: {
        gboolean enable = FALSE;
        g_object_get(self->glsinkbin,
                     PROP_ENABLE_LAST_SAMPLE_NAME,
                     &enable,
                     NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_LAST_SAMPLE: {
        GstSample *sample = NULL;
        g_object_get(self->glsinkbin,
                     PROP_LAST_SAMPLE_NAME,
                     &sample,
                     NULL);
        gst_value_set_sample(value, sample);
        if (sample) {
            gst_sample_unref(sample);
        }
        break;
    }
    case PROP_WIDGET: {
        gpointer widget = NULL;
        g_object_get(self->qmlglsink,
                     PROP_WIDGET_NAME,
                     &widget,
                     NULL);
        g_value_set_pointer(value, widget);
        break;
    }
    case PROP_FORCE_ASPECT_RATIO: {
        gboolean enable = FALSE;
        g_object_get(self->glsinkbin,
                     PROP_FORCE_ASPECT_RATIO_NAME,
                     &enable,
                     NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_PIXEL_ASPECT_RATIO: {
        gint num = 0, den = 1;
        g_object_get(self->qmlglsink,
                     PROP_PIXEL_ASPECT_RATIO_NAME,
                     &num, &den,
                     NULL);
        gst_value_set_fraction(value, num, den);
        break;
    }
    case PROP_SYNC: {
        gboolean enable = FALSE;
        g_object_get(self->glsinkbin,
                     PROP_SYNC_NAME,
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
gst_qgc_video_sink_bin_dispose(GObject *object)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    (void) self;

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
gst_qgc_video_sink_bin_finalize(GObject *object)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    (void) self;

    G_OBJECT_CLASS(parent_class)->finalize(object);
}
