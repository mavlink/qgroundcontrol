#pragma once

#include <gst/gst.h>

G_BEGIN_DECLS

void qgc_element_init(GstPlugin *plugin);

GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbin);
GST_ELEMENT_REGISTER_DECLARE(qgcvideosinkbind3d11);

G_END_DECLS
