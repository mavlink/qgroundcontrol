/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file
 *   @brief GStreamer plugin for QGC's Video Receiver
 *   @author Andrew Voznyts <andrew.voznytsa@gmail.com>
 *   @author Tomaz Canabrava <tcanabrava@kde.org>
 */

#include <glib-object.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC(gst_qgc_video_sink_bin_debug);
#define GST_CAT_DEFAULT gst_qgc_video_sink_bin_debug

typedef struct _GstQgcVideoSinkElement GstQgcVideoSinkElement;

typedef struct _GstQgcVideoSinkBin {
    GstBin bin;
    GstElement* glupload;
    GstElement* qmlglsink;
} GstQgcVideoSinkBin;

typedef struct _GstQgcVideoSinkBinClass {
    GstBinClass parent_class;
} GstQgcVideoSinkBinClass;

#define GST_TYPE_VIDEO_SINK_BIN (_vsb_get_type())
#define GST_QGC_VIDEO_SINK_BIN_CAST(obj) ((GstQgcVideoSinkBin *)(obj))
#define GST_QGC_VIDEO_SINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_VIDEO_SINK_BIN, GstQgcVideoSinkBin))
#define GST_QGC_VIDEO_SINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_VIDEO_SINK_BIN, GstQgcVideoSinkBinClass))
#define GST_IS_VIDEO_SINK_BIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_VIDEO_SINK_BIN))
#define GST_IS_VIDEO_SINK_BIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_VIDEO_SINK_BIN))

enum {
    PROP_0,
    PROP_ENABLE_LAST_SAMPLE,
    PROP_LAST_SAMPLE,
    PROP_WIDGET,
    PROP_FORCE_ASPECT_RATIO,
    PROP_PIXEL_ASPECT_RATIO,
    PROP_SYNC,
};

#define PROP_ENABLE_LAST_SAMPLE_NAME    "enable-last-sample"
#define PROP_LAST_SAMPLE_NAME           "last-sample"
#define PROP_WIDGET_NAME                "widget"
#define PROP_FORCE_ASPECT_RATIO_NAME    "force-aspect-ratio"
#define PROP_PIXEL_ASPECT_RATIO_NAME    "pixel-aspect-ratio"
#define PROP_SYNC_NAME                  "sync"

#define DEFAULT_ENABLE_LAST_SAMPLE TRUE
#define DEFAULT_FORCE_ASPECT_RATIO TRUE
#define DEFAULT_PAR_N 0
#define DEFAULT_PAR_D 1
#define DEFAULT_SYNC TRUE

static GstBinClass *parent_class;

static void _vsb_init(GstQgcVideoSinkBin *vsb);
static void _vsb_dispose(GObject *object);
static void _vsb_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void _vsb_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static GType _vsb_get_type(void);
static void _vsb_class_init(GstQgcVideoSinkBinClass *klass);

static gboolean
_vsb_sink_pad_query(GstPad* pad, GstObject* parent, GstQuery* query)
{
    GstQgcVideoSinkBin *vsb;
    GstElement* element;
    
    vsb = GST_QGC_VIDEO_SINK_BIN(parent);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
        element = vsb->glupload;
        break;
    case GST_QUERY_CONTEXT:
        element = vsb->qmlglsink;
        break;
    default:
        return gst_pad_query_default (pad, parent, query);
    }

    if (element == NULL) {
        GST_ERROR_OBJECT(vsb, "No element found");
        return FALSE;
    }

    GstPad* sinkpad = gst_element_get_static_pad(element, "sink");

    if (sinkpad == NULL) {
        GST_ERROR_OBJECT(vsb, "No sink pad found");
        return FALSE;
    }

    const gboolean ret = gst_pad_query(sinkpad, query);

    gst_object_unref(sinkpad);
    sinkpad = NULL;

    return ret;
}

static void
_vsb_init(GstQgcVideoSinkBin *vsb)
{
    gboolean initialized        = FALSE;
    GstElement* glcolorconvert  = NULL;
    GstPad* pad                 = NULL;

    do {
        if ((vsb->glupload = gst_element_factory_make("glupload", NULL)) == NULL) {
            GST_ERROR_OBJECT(vsb, "gst_element_factory_make('glupload') failed");
            break;
        }

        if ((vsb->qmlglsink = gst_element_factory_make("qmlglsink", NULL)) == NULL) {
            GST_ERROR_OBJECT(vsb, "gst_element_factory_make('qmlglsink') failed");
            break;
        }

        if ((glcolorconvert = gst_element_factory_make("glcolorconvert", NULL)) == NULL) {
            GST_ERROR_OBJECT(vsb, "gst_element_factory_make('glcolorconvert' failed)");
            break;
        }

        if ((pad = gst_element_get_static_pad(vsb->glupload, "sink")) == NULL) {
            GST_ERROR_OBJECT(vsb, "gst_element_get_static_pad(glupload, 'sink') failed");
            break;
        }

        gst_object_ref(vsb->glupload);
        gst_object_ref(vsb->qmlglsink);

        gst_bin_add_many(GST_BIN(vsb), vsb->glupload, glcolorconvert, vsb->qmlglsink, NULL);

        gboolean ret = gst_element_link_many(vsb->glupload, glcolorconvert, vsb->qmlglsink, NULL);

        glcolorconvert = NULL;

        if (!ret) {
            GST_ERROR_OBJECT(vsb, "gst_element_link_many() failed");
            break;
        }

        GstPad* ghostpad;

        if ((ghostpad = gst_ghost_pad_new("sink", pad)) == NULL) {
            GST_ERROR_OBJECT(vsb, "gst_ghost_pad_new('sink') failed");
            break;
        }

        gst_pad_set_query_function(ghostpad, _vsb_sink_pad_query);

        if (!gst_element_add_pad(GST_ELEMENT(vsb), ghostpad)) {
            GST_ERROR_OBJECT(vsb, "gst_element_add_pad() failed");
            break;
        }

        initialized = TRUE;
    } while(0);

    if (pad != NULL) {
        gst_object_unref(pad);
        pad = NULL;
    }

    if (glcolorconvert != NULL) {
        gst_object_unref(glcolorconvert);
        glcolorconvert = NULL;
    }

    if (!initialized) {
        if (vsb->qmlglsink != NULL) {
            gst_object_unref(vsb->qmlglsink);
            vsb->qmlglsink = NULL;
        }

        if (vsb->glupload != NULL) {
            gst_object_unref(vsb->glupload);
            vsb->glupload = NULL;
        }
    }
}

static void
_vsb_dispose(GObject *object)
{
    GstQgcVideoSinkBin *vsb;

    vsb = GST_QGC_VIDEO_SINK_BIN(object);

    if (vsb->qmlglsink != NULL) {
        gst_object_unref(vsb->qmlglsink);
        vsb->qmlglsink = NULL;
    }

    if (vsb->glupload != NULL) {
        gst_object_unref(vsb->glupload);
        vsb->glupload = NULL;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
_vsb_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *vsb;

    vsb = GST_QGC_VIDEO_SINK_BIN(object);

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        do {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_ENABLE_LAST_SAMPLE_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        } while(0);
        break;
    case PROP_LAST_SAMPLE:
        do {
            GstSample *sample = NULL;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_LAST_SAMPLE_NAME, &sample, NULL);
            gst_value_set_sample(value, sample);
            if (sample != NULL) {
                gst_sample_unref(sample);
                sample = NULL;
            }
        } while(0);
        break;
    case PROP_WIDGET:
        do {
            gpointer widget = NULL;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_WIDGET_NAME, &widget, NULL);
            g_value_set_pointer(value, widget);
        } while(0);
        break;
    case PROP_FORCE_ASPECT_RATIO:
        do {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_FORCE_ASPECT_RATIO_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        } while(0);
        break;
    case PROP_PIXEL_ASPECT_RATIO:
        do {
            gint num = 0, den = 1;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_PIXEL_ASPECT_RATIO_NAME, &num, &den, NULL);
            gst_value_set_fraction(value, num, den);
        } while(0);
        break;
    case PROP_SYNC:
        do {
            gboolean enable = FALSE;
            g_object_get(G_OBJECT(vsb->qmlglsink), PROP_SYNC_NAME, &enable, NULL);
            g_value_set_boolean(value, enable);
        } while(0);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
_vsb_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *vsb;

    vsb = GST_QGC_VIDEO_SINK_BIN(object);

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

static GType
_vsb_get_type(void)
{
    static GType _vsb_type = 0;

    if (!_vsb_type) {
        static const GTypeInfo _vsb_info = {
            sizeof(GstQgcVideoSinkBinClass),
            NULL,
            NULL,
            (GClassInitFunc)_vsb_class_init,
            NULL,
            NULL,
            sizeof(GstQgcVideoSinkBin),
            0,
            (GInstanceInitFunc)_vsb_init,
            NULL};

        _vsb_type = g_type_register_static(GST_TYPE_BIN, "GstQgcVideoSinkBin", &_vsb_info, (GTypeFlags)0);
    }

    return _vsb_type;
}

static void
_vsb_class_init(GstQgcVideoSinkBinClass *klass)
{
    GObjectClass *gobject_klass;
    GstElementClass *gstelement_klass;

    gobject_klass = (GObjectClass *)klass;
    gstelement_klass = (GstElementClass *)klass;

    parent_class = g_type_class_peek_parent(klass);

    gobject_klass->dispose = _vsb_dispose;
    gobject_klass->get_property = _vsb_get_property;
    gobject_klass->set_property = _vsb_set_property;

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

gboolean
gst_qgc_video_sink_bin_plugin_init(GstPlugin *plugin)
{
    GST_DEBUG_CATEGORY_INIT(gst_qgc_video_sink_bin_debug, "qgcvideosinkbin", 0, "QGC Video Sink Bin");
    return gst_element_register(plugin, "qgcvideosinkbin", GST_RANK_NONE, GST_TYPE_VIDEO_SINK_BIN);
}
