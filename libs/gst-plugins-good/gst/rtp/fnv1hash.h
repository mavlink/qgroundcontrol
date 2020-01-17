/* GStreamer
 * Copyright (C) 2007 Thomas Vander Stichele <thomas at apestaart dot org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_FNV1_HASH_H__
#define __GST_FNV1_HASH_H__

#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

guint32 fnv1_hash_32_new (void);
guint32 fnv1_hash_32_update (guint32 hash, const guchar *data, guint length);
guint32 fnv1_hash_32_to_24 (guint32 hash);

G_END_DECLS

#endif /* __GST_FNV1_HASH_H__ */

