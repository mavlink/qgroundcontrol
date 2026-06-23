#ifndef GST_QGC_ELEMENTS_H
#define GST_QGC_ELEMENTS_H

#include <gst/gst.h>

G_BEGIN_DECLS

extern GstDebugCategory *gst_qgc_debug;

void qgc_element_init(GstPlugin *plugin);

GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbin);

G_END_DECLS

#endif // GST_QGC_ELEMENTS_H
