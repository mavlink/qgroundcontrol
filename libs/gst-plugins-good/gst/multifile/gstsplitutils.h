/* GStreamer Split Source Utility Functions
 * Copyright (C) 2011 Collabora Ltd. <tim.muller@collabora.co.uk>
 * Copyright (C) 2014 Jan Schmidt <jan@centricular.com>
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

#ifndef __GST_SPLITUTILS_H__
#define __GST_SPLITUTILS_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#ifdef G_OS_WIN32
#define DEFAULT_PATTERN_MATCH_MODE MATCH_MODE_UTF8
#else
#define DEFAULT_PATTERN_MATCH_MODE MATCH_MODE_AUTO
#endif

gchar **
gst_split_util_find_files (const gchar * dirname,
    const gchar * basename, GError ** err);

G_END_DECLS

#endif
