#include "gstqgcelements.h"
#include "qgc_version.h"

#define GST_CAT_DEFAULT gst_qgc_debug
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

void qgc_element_init(GstPlugin* plugin)
{
    (void) plugin;
    static gsize res = FALSE;
    if (g_once_init_enter(&res)) {
        GST_DEBUG_CATEGORY_INIT(gst_qgc_debug, "qgc", 0, "QGC");
        g_once_init_leave(&res, TRUE);
    }
}

static gboolean plugin_init(GstPlugin* plugin)
{
    if (!GST_ELEMENT_REGISTER(qgcqvideosink, plugin))
        return FALSE;
    return TRUE;
}

#define GST_PACKAGE_NAME "GStreamer plugin for QGC's Video Receiver"
#define GST_PACKAGE_ORIGIN "https://qgroundcontrol.com/"
#define GST_LICENSE "LGPL"
#define PACKAGE "QGC Video Receiver"
#define PACKAGE_VERSION QGC_APP_VERSION_STR

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, qgc, "QGC Video Receiver Plugin", plugin_init, PACKAGE_VERSION,
                  GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
