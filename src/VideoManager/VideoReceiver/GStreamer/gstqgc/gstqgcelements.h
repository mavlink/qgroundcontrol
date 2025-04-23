#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

void qgc_element_init(GstPlugin *plugin);

GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbin);

G_END_DECLS
