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

#define GST_CAT_DEFAULT gst_qgc_video_sink_bin_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

static void gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void gst_qgc_video_sink_bin_dispose(GObject *object);

static gboolean gst_qgc_video_sink_bin_sink_pad_query(GstPad *pad, GstObject *parent, GstQuery *query);

#define DEFAULT_ENABLE_LAST_SAMPLE TRUE
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

enum {
    PROP_0,
    PROP_ENABLE_LAST_SAMPLE,
    PROP_LAST_SAMPLE,
    PROP_WIDGET,
    PROP_FORCE_ASPECT_RATIO,
    PROP_PIXEL_ASPECT_RATIO,
    PROP_SYNC,
};

#define gst_qgc_video_sink_bin_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstQgcVideoSinkBin, gst_qgc_video_sink_bin, GST_TYPE_BIN, GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "qgcsinkbin", 0, "QGC Video Sink Bin"));

// GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(qgcvideosinkbin, "qgcvideosinkbin", GST_RANK_NONE, GST_TYPE_QGC_VIDEO_SINK_BIN, qgc_element_init(plugin));
G_BEGIN_DECLS
gboolean G_PASTE(gst_element_register_, qgcvideosinkbin)(GstPlugin *plugin)
{
    {
        {
            qgc_element_init(plugin);
        }
    }
    return gst_element_register(plugin, "qgcvideosinkbin", GST_RANK_NONE, (gst_qgc_video_sink_bin_get_type()));
}
G_END_DECLS;

static void
gst_qgc_video_sink_bin_class_init(GstQgcVideoSinkBinClass *klass)
{
    GObjectClass *const gobject_klass = G_OBJECT_CLASS(klass);
    GstElementClass *const gstelement_klass = GST_ELEMENT_CLASS(klass);

    parent_class = g_type_class_peek_parent(GST_BIN_CLASS(klass));

    gobject_klass->dispose = gst_qgc_video_sink_bin_dispose;
    gobject_klass->get_property = gst_qgc_video_sink_bin_get_property;
    gobject_klass->set_property = gst_qgc_video_sink_bin_set_property;

    g_object_class_install_property(gobject_klass, PROP_ENABLE_LAST_SAMPLE,
        g_param_spec_boolean(PROP_ENABLE_LAST_SAMPLE_NAME, "Enable Last Buffer",
            "Enable the last-sample property", DEFAULT_ENABLE_LAST_SAMPLE,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_klass, PROP_LAST_SAMPLE,
        g_param_spec_boxed(PROP_LAST_SAMPLE_NAME, "Last Sample",
            "The last sample received in the sink", GST_TYPE_SAMPLE,
            (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_klass, PROP_WIDGET,
        g_param_spec_pointer(PROP_WIDGET_NAME, "QQuickItem",
            "The QQuickItem to place in the object hierarchy",
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_klass, PROP_FORCE_ASPECT_RATIO,
        g_param_spec_boolean(PROP_FORCE_ASPECT_RATIO_NAME, "Force aspect ratio",
            "When enabled, scaling will respect original aspect ratio",
            DEFAULT_FORCE_ASPECT_RATIO,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_klass, PROP_PIXEL_ASPECT_RATIO,
        gst_param_spec_fraction(PROP_PIXEL_ASPECT_RATIO_NAME, "Pixel Aspect Ratio",
            "The pixel aspect ratio of the device", DEFAULT_PAR_N, DEFAULT_PAR_D,
            G_MAXINT, 1, 1, 1,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_klass, PROP_SYNC,
        g_param_spec_boolean(PROP_SYNC_NAME, "Sync",
            "Sync on the clock", DEFAULT_SYNC,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata(gstelement_klass,
        "QGC Video Sink Bin", "Sink/Video/Bin",
        "Video rendering for QGC",
        "Andrew Voznytsa <andrew.voznytsa@gmail.com>, Tomaz Canabrava <tcanabrava@kde.org>");
}

static void
gst_qgc_video_sink_bin_init(GstQgcVideoSinkBin *vsb)
{
    gboolean initialized = FALSE;
    gboolean ret = FALSE;
    GstElement *glcolorconvert = NULL;
    GstPad *pad = NULL;
    GstPad *ghostpad = NULL;

    vsb->glupload = gst_element_factory_make("glupload", NULL);
    if (!vsb->glupload) {
        GST_ERROR_OBJECT(vsb, "gst_element_factory_make('glupload') failed");
        goto init_failed;
    }

    vsb->qmlglsink = gst_element_factory_make("qml6glsink", NULL);
    if (!vsb->qmlglsink) {
        GST_ERROR_OBJECT(vsb, "gst_element_factory_make('qml6glsink') failed");
        goto init_failed;
    }

    glcolorconvert = gst_element_factory_make("glcolorconvert", NULL);
    if (!glcolorconvert) {
        GST_ERROR_OBJECT(vsb, "gst_element_factory_make('glcolorconvert' failed)");
        goto init_failed;
    }

    pad = gst_element_get_static_pad(vsb->glupload, "sink");
    if (!pad) {
        GST_ERROR_OBJECT(vsb, "gst_element_get_static_pad(glupload, 'sink') failed");
        goto init_failed;
    }

    (void) gst_object_ref(vsb->glupload);
    (void) gst_object_ref(vsb->qmlglsink);

    gst_bin_add_many(GST_BIN(vsb), vsb->glupload, glcolorconvert, vsb->qmlglsink, NULL);

    ret = gst_element_link_many(vsb->glupload, glcolorconvert, vsb->qmlglsink, NULL);
    glcolorconvert = NULL;
    if (!ret) {
        GST_ERROR_OBJECT(vsb, "gst_element_link_many() failed");
        goto init_failed;
    }

    ghostpad = gst_ghost_pad_new("sink", pad);
    if (!ghostpad) {
        GST_ERROR_OBJECT(vsb, "gst_ghost_pad_new('sink') failed");
        goto init_failed;
    }

    gst_pad_set_query_function(ghostpad, gst_qgc_video_sink_bin_sink_pad_query);

    if (!gst_element_add_pad(GST_ELEMENT(vsb), ghostpad)) {
        GST_ERROR_OBJECT(vsb, "gst_element_add_pad() failed");
        goto init_failed;
    }

    initialized = TRUE;

init_failed:
    if (pad) {
        gst_object_unref(pad);
        pad = NULL;
    }

    if (glcolorconvert) {
        gst_object_unref(glcolorconvert);
        glcolorconvert = NULL;
    }

    if (!initialized) {
        if (vsb->qmlglsink) {
            gst_object_unref(vsb->qmlglsink);
            vsb->qmlglsink = NULL;
        }

        if (vsb->glupload) {
            gst_object_unref(vsb->glupload);
            vsb->glupload = NULL;
        }
    }
}

static gboolean
gst_qgc_video_sink_bin_sink_pad_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
    GstQgcVideoSinkBin *const vsb = GST_QGC_VIDEO_SINK_BIN(parent);
    GstElement *element = NULL;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
        element = vsb->glupload;
        break;
    case GST_QUERY_CONTEXT:
        element = vsb->qmlglsink;
        break;
    default:
        return gst_pad_query_default(pad, parent, query);
    }

    if (!element) {
        GST_ERROR_OBJECT(vsb, "No element found");
        return FALSE;
    }

    GstPad *sinkpad = gst_element_get_static_pad(element, "sink");
    if (!sinkpad) {
        GST_ERROR_OBJECT(vsb, "No sink pad found");
        return FALSE;
    }

    const gboolean ret = gst_pad_query(sinkpad, query);

    gst_object_unref(sinkpad);
    sinkpad = NULL;

    return ret;
}

static void
gst_qgc_video_sink_bin_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *const vsb = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        if (vsb->qmlglsink) {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_ENABLE_LAST_SAMPLE_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        }
        break;
    case PROP_LAST_SAMPLE:
        if (vsb->qmlglsink) {
            GstSample *sample = NULL;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_LAST_SAMPLE_NAME, &sample, NULL);
            gst_value_set_sample(value, sample);
            if (sample != NULL) {
                gst_sample_unref(sample);
                sample = NULL;
            }
        }
        break;
    case PROP_WIDGET:
        if (vsb->qmlglsink) {
            gpointer widget = NULL;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_WIDGET_NAME, &widget, NULL);
            g_value_set_pointer(value, widget);
        }
        break;
    case PROP_FORCE_ASPECT_RATIO:
        if (vsb->qmlglsink) {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_FORCE_ASPECT_RATIO_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        }
        break;
    case PROP_PIXEL_ASPECT_RATIO:
        if (vsb->qmlglsink) {
            gint num = 0, den = 1;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_PIXEL_ASPECT_RATIO_NAME, &num, &den, NULL);
            gst_value_set_fraction(value, num, den);
        }
        break;
    case PROP_SYNC:
        if (vsb->qmlglsink) {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_SYNC_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        }
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *const vsb = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        g_object_set(G_OBJECT(vsb->qmlglsink), PROP_ENABLE_LAST_SAMPLE_NAME, g_value_get_boolean(value), NULL);
        break;
    case PROP_WIDGET:
        g_object_set(G_OBJECT(vsb->qmlglsink), PROP_WIDGET_NAME, g_value_get_pointer(value), NULL);
        break;
    case PROP_FORCE_ASPECT_RATIO:
        g_object_set(G_OBJECT(vsb->qmlglsink), PROP_FORCE_ASPECT_RATIO_NAME, g_value_get_boolean(value), NULL);
        break;
    case PROP_PIXEL_ASPECT_RATIO:
        g_object_set(G_OBJECT(vsb->qmlglsink), PROP_PIXEL_ASPECT_RATIO_NAME, gst_value_get_fraction_numerator(value), gst_value_get_fraction_denominator(value), NULL);
        break;
    case PROP_SYNC:
        g_object_set(G_OBJECT(vsb->qmlglsink), PROP_SYNC_NAME, g_value_get_boolean(value), NULL);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_qgc_video_sink_bin_dispose(GObject *object)
{
    GstQgcVideoSinkBin *const vsb = GST_QGC_VIDEO_SINK_BIN(object);

    if (vsb->qmlglsink) {
        gst_object_unref(vsb->qmlglsink);
        vsb->qmlglsink = NULL;
    }

    if (vsb->glupload) {
        gst_object_unref(vsb->glupload);
        vsb->glupload = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}
