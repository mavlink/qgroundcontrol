#include "gstqgcelements.h"
#include "qgc_version.h"

static gboolean plugin_init(GstPlugin* plugin)
{
    if (!GST_ELEMENT_REGISTER(qgcvideosinkbin, plugin))
        return FALSE;
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
