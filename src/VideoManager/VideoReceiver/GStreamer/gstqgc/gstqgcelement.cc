#include "gstqgcelements.h"

#define GST_CAT_DEFAULT gst_qgc_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

void
qgc_element_init(GstPlugin *plugin)
{
    static gsize res = FALSE;
    if (g_once_init_enter(&res)) {
        GST_DEBUG_CATEGORY_INIT (gst_qgc_debug, "qgc", 0, "QGC");
        g_once_init_leave(&res, TRUE);
    }
}
