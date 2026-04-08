#include "gstqgcvideosinkbin.h"
#include "gstqgcelements.h"

#include <gst/gl/gl.h>
#if defined(__APPLE__) && defined(__MACH__)
#include <gst/app/gstappsink.h>
#endif

#define GST_CAT_DEFAULT gst_qgc_video_sink_bin_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

#define DEFAULT_ENABLE_LAST_SAMPLE FALSE
#define DEFAULT_FORCE_ASPECT_RATIO TRUE
#define DEFAULT_PAR_N 0
#define DEFAULT_PAR_D 1
#define DEFAULT_SYNC TRUE
#define DEFAULT_MAX_LATENESS G_GINT64_CONSTANT(-1)

#define PROP_ENABLE_LAST_SAMPLE_NAME    "enable-last-sample"
#define PROP_LAST_SAMPLE_NAME           "last-sample"
#define PROP_WIDGET_NAME                "widget"
#define PROP_FORCE_ASPECT_RATIO_NAME    "force-aspect-ratio"
#define PROP_PIXEL_ASPECT_RATIO_NAME    "pixel-aspect-ratio"
#define PROP_SYNC_NAME                  "sync"
#define PROP_MAX_LATENESS_NAME          "max-lateness"

enum
{
    PROP_0,
    PROP_ENABLE_LAST_SAMPLE,
    PROP_LAST_SAMPLE,
    PROP_WIDGET,
    PROP_FORCE_ASPECT_RATIO,
    PROP_PIXEL_ASPECT_RATIO,
    PROP_SYNC,
    PROP_MAX_LATENESS,
    PROP_LAST
};

static GParamSpec *properties[PROP_LAST];

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

static void
gst_qgc_video_sink_bin_class_init(GstQgcVideoSinkBinClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    object_class->set_property = gst_qgc_video_sink_bin_set_property;
    object_class->get_property = gst_qgc_video_sink_bin_get_property;

    properties[PROP_ENABLE_LAST_SAMPLE] = g_param_spec_boolean(
        "enable-last-sample", "Enable last sample",
        "Retain the most recent buffer for UI snapshotting",
        DEFAULT_ENABLE_LAST_SAMPLE,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_LAST_SAMPLE] = g_param_spec_boxed(
        "last-sample", "Last sample",
        "Last preroll/played sample held by the sink",
        GST_TYPE_SAMPLE,
        (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_WIDGET] = g_param_spec_pointer(
        "widget", "QQuickItem",
        "Owning QML item – handed off to qml6glsink",
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_FORCE_ASPECT_RATIO] = g_param_spec_boolean(
        "force-aspect-ratio", "Force aspect ratio",
        "Maintain original pixel aspect during scaling",
        DEFAULT_FORCE_ASPECT_RATIO,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_PIXEL_ASPECT_RATIO] = gst_param_spec_fraction(
        "pixel-aspect-ratio", "Pixel aspect ratio",
        "Pixel aspect ratio of the display surface",
        DEFAULT_PAR_N, DEFAULT_PAR_D,
        G_MAXINT, 1,
        1, 1,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_SYNC] = g_param_spec_boolean(
        "sync", "Sync",
        "Synchronise frame presentation to the pipeline clock",
        DEFAULT_SYNC,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    properties[PROP_MAX_LATENESS] = g_param_spec_int64(
        "max-lateness", "Max lateness",
        "Maximum number of nanoseconds a buffer can be late before it is dropped (-1 unlimited)",
        -1, G_MAXINT64,
        DEFAULT_MAX_LATENESS,
        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
    );

    g_object_class_install_properties(object_class, PROP_LAST, properties);

    gst_element_class_set_static_metadata(element_class,
        "QGC Video Sink Bin", "Sink/Video/Bin",
        "Hardware-accelerated video sink (D3D11 on Windows, GL elsewhere) for QGroundControl",
        "QGroundControl team"
    );
}

static gboolean
gst_qgc_video_sink_bin_ghost_pad(GstQgcVideoSinkBin *self, GstElement *inner)
{
    GstPad *sinkpad = gst_element_get_static_pad(inner, "sink");
    if (!sinkpad) {
        GST_ERROR_OBJECT(self, "gst_element_get_static_pad('sink') failed");
        return FALSE;
    }

    GstPad *ghostpad = gst_ghost_pad_new("sink", sinkpad);
    gst_object_unref(sinkpad);
    if (!ghostpad) {
        GST_ERROR_OBJECT(self, "gst_ghost_pad_new('sink') failed");
        return FALSE;
    }

    if (!gst_element_add_pad(GST_ELEMENT(self), ghostpad)) {
        GST_ERROR_OBJECT(self, "gst_element_add_pad() failed");
        return FALSE;
    }

    return TRUE;
}

static void
gst_qgc_video_sink_bin_init(GstQgcVideoSinkBin *self)
{
    self->using_d3d11 = FALSE;
    self->using_appsink = FALSE;

#ifdef QGC_GST_D3D11_SINK
    // Prefer D3D11 sink on Windows — zero-copy from D3D hardware decoders,
    // no OpenGL interop needed, renders via Qt's native D3D11 RHI backend.
    self->d3d11sink = gst_element_factory_make("qml6d3d11sink", NULL);
    if (self->d3d11sink) {
        GST_INFO_OBJECT(self, "Using qml6d3d11sink (D3D11 rendering path)");

        if (!gst_bin_add(GST_BIN(self), self->d3d11sink)) {
            GST_ERROR_OBJECT(self, "Failed to add qml6d3d11sink to bin");
            gst_object_unref(self->d3d11sink);
            self->d3d11sink = NULL;
        } else if (gst_qgc_video_sink_bin_ghost_pad(self, self->d3d11sink)) {
            self->using_d3d11 = TRUE;
            return;
        } else {
            gst_bin_remove(GST_BIN(self), self->d3d11sink);
            self->d3d11sink = NULL;
        }
    }
#endif

#if defined(__APPLE__) && defined(__MACH__)
    // macOS Metal path: use appsink with videoconvert to avoid OpenGL dependency.
    // Frames are extracted via the new-sample callback and pushed to a QVideoSink,
    // which renders through Qt's native Metal RHI backend.
    self->videoconvert = gst_element_factory_make("videoconvert", NULL);
    self->appsink = gst_element_factory_make("appsink", "qgcappsink");
    if (self->videoconvert && self->appsink) {
        // Accept BGRA so QVideoFrame can use a simple single-plane copy
        GstCaps *caps = gst_caps_from_string("video/x-raw,format=BGRA");
        g_object_set(self->appsink,
                     "caps", caps,
                     "emit-signals", TRUE,
                     "max-buffers", 2,
                     "drop", TRUE,
                     "sync", FALSE,
                     NULL);
        gst_caps_unref(caps);

        gst_bin_add_many(GST_BIN(self), self->videoconvert, self->appsink, NULL);
        if (gst_element_link(self->videoconvert, self->appsink)
            && gst_qgc_video_sink_bin_ghost_pad(self, self->videoconvert)) {
            self->using_appsink = TRUE;
            GST_INFO_OBJECT(self, "Using appsink (macOS Metal rendering path)");
            return;
        }

        GST_WARNING_OBJECT(self, "Failed to link appsink path, falling back to GL");
        gst_bin_remove(GST_BIN(self), self->videoconvert);
        gst_bin_remove(GST_BIN(self), self->appsink);
        self->videoconvert = NULL;
        self->appsink = NULL;
    } else {
        GST_WARNING_OBJECT(self, "Failed to create appsink path elements: videoconvert=%p appsink=%p, falling back to GL",
                           (void *)self->videoconvert, (void *)self->appsink);
        gst_clear_object(&self->videoconvert);
        gst_clear_object(&self->appsink);
    }
#endif

    // GL path: glsinkbin wraps qml6glsink with automatic glupload/glcolorconvert
    self->glsinkbin = gst_element_factory_make("glsinkbin", NULL);
    if (!self->glsinkbin) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('glsinkbin') failed");
        return;
    }

    self->qmlglsink = gst_element_factory_make("qml6glsink", NULL);
    if (!self->qmlglsink) {
        GST_ERROR_OBJECT(self, "gst_element_factory_make('qml6glsink') failed");
        gst_clear_object(&self->glsinkbin);
        return;
    }

    // glsinkbin takes ownership of qmlglsink (sinks its floating ref)
    g_object_set(self->glsinkbin,
                 "sink", self->qmlglsink,
                 PROP_ENABLE_LAST_SAMPLE_NAME, FALSE,
                 NULL);

    if (!gst_bin_add(GST_BIN(self), self->glsinkbin)) {
        GST_ERROR_OBJECT(self, "Failed to add glsinkbin to bin");
        // glsinkbin owns qmlglsink — clearing it frees both
        self->qmlglsink = NULL;
        gst_clear_object(&self->glsinkbin);
        return;
    }

    gst_qgc_video_sink_bin_ghost_pad(self, self->glsinkbin);
}

static void
gst_qgc_video_sink_bin_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstQgcVideoSinkBin *self = GST_QGC_VIDEO_SINK_BIN(object);

    // Route properties to the active sink element.
    // For the appsink path, widget/force-aspect-ratio/pixel-aspect-ratio are no-ops.
    GstElement *activeSink = self->using_d3d11 ? self->d3d11sink
                           : self->using_appsink ? self->appsink
                           : self->qmlglsink;
    GstElement *activeBin  = self->using_d3d11 ? self->d3d11sink
                           : self->using_appsink ? self->appsink
                           : self->glsinkbin;

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE:
        if (G_LIKELY(activeBin))
            g_object_set(activeBin,
                         PROP_ENABLE_LAST_SAMPLE_NAME,
                         g_value_get_boolean(value),
                         NULL);
        break;
    case PROP_WIDGET:
        if (self->using_appsink)
            break; // appsink path does not use a widget
        if (G_LIKELY(activeSink))
            g_object_set(activeSink,
                         PROP_WIDGET_NAME,
                         g_value_get_pointer(value),
                         NULL);
        break;
    case PROP_FORCE_ASPECT_RATIO:
        if (self->using_appsink)
            break;
        if (G_LIKELY(activeSink))
            g_object_set(activeSink,
                         PROP_FORCE_ASPECT_RATIO_NAME,
                         g_value_get_boolean(value),
                         NULL);
        break;
    case PROP_PIXEL_ASPECT_RATIO: {
        if (self->using_appsink)
            break;
        const gint num = gst_value_get_fraction_numerator(value);
        const gint den = gst_value_get_fraction_denominator(value);
        if (G_LIKELY(activeSink))
            g_object_set(activeSink,
                         PROP_PIXEL_ASPECT_RATIO_NAME,
                         num,
                         den,
                         NULL);
        break;
    }
    case PROP_SYNC:
        if (G_LIKELY(activeBin))
            g_object_set(activeBin,
                         PROP_SYNC_NAME,
                         g_value_get_boolean(value),
                         NULL);
        break;
    case PROP_MAX_LATENESS:
        if (G_LIKELY(activeSink))
            g_object_set(activeSink,
                         PROP_MAX_LATENESS_NAME,
                         g_value_get_int64(value),
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

    GstElement *activeSink = self->using_d3d11 ? self->d3d11sink
                           : self->using_appsink ? self->appsink
                           : self->qmlglsink;
    GstElement *activeBin  = self->using_d3d11 ? self->d3d11sink
                           : self->using_appsink ? self->appsink
                           : self->glsinkbin;

    switch (prop_id) {
    case PROP_ENABLE_LAST_SAMPLE: {
        gboolean enable = FALSE;
        if (G_LIKELY(activeBin))
            g_object_get(activeBin,
                         PROP_ENABLE_LAST_SAMPLE_NAME,
                         &enable,
                         NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_LAST_SAMPLE: {
        GstSample *sample = NULL;
        if (G_LIKELY(activeBin))
            g_object_get(activeBin,
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
        if (self->using_appsink) {
            g_value_set_pointer(value, NULL);
            break;
        }
        gpointer widget = NULL;
        if (G_LIKELY(activeSink))
            g_object_get(activeSink,
                         PROP_WIDGET_NAME,
                         &widget,
                         NULL);
        g_value_set_pointer(value, widget);
        break;
    }
    case PROP_FORCE_ASPECT_RATIO: {
        if (self->using_appsink) {
            g_value_set_boolean(value, FALSE);
            break;
        }
        gboolean enable = FALSE;
        if (G_LIKELY(activeSink))
            g_object_get(activeSink,
                         PROP_FORCE_ASPECT_RATIO_NAME,
                         &enable,
                         NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_PIXEL_ASPECT_RATIO: {
        if (self->using_appsink) {
            gst_value_set_fraction(value, 1, 1);
            break;
        }
        gint num = 0, den = 1;
        if (G_LIKELY(activeSink))
            g_object_get(activeSink,
                         PROP_PIXEL_ASPECT_RATIO_NAME,
                         &num, &den,
                         NULL);
        gst_value_set_fraction(value, num, den);
        break;
    }
    case PROP_SYNC: {
        gboolean enable = FALSE;
        if (G_LIKELY(activeBin))
            g_object_get(activeBin,
                         PROP_SYNC_NAME,
                         &enable,
                         NULL);
        g_value_set_boolean(value, enable);
        break;
    }
    case PROP_MAX_LATENESS: {
        gint64 lateness = DEFAULT_MAX_LATENESS;
        if (G_LIKELY(activeSink))
            g_object_get(activeSink,
                         PROP_MAX_LATENESS_NAME,
                         &lateness,
                         NULL);
        g_value_set_int64(value, lateness);
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

