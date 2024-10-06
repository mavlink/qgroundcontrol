#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

void qgc_element_init(GstPlugin *plugin);

gboolean gst_element_register_qgcvideosinkbin(GstPlugin * plugin);
// GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbin);

G_END_DECLS
