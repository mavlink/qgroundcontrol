/* GStreamer
 *
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
 *     @Author: Reynaldo H. Verdejo Pinochet <r.verdejo@sisa.samsung.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more
 */

#ifndef __GST_SOUP_UTILS_H__
#define __GST_SOUP_UTILS_H__

#include <glib.h>
#include <gst/gst.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

void gst_soup_util_log_setup (SoupSession * session, SoupLoggerLogLevel level,
    GstElement * element);

G_END_DECLS

#endif
