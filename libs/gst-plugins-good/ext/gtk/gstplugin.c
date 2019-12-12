/*
 * GStreamer
 * Copyright (C) 2015 Matthew Waters <matthew@centricular.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstgtksink.h"
#if defined(HAVE_GTK3_GL)
#include "gstgtkglsink.h"
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "gtksink",
          GST_RANK_NONE, GST_TYPE_GTK_SINK)) {
    return FALSE;
  }
#if defined(HAVE_GTK3_GL)
  if (!gst_element_register (plugin, "gtkglsink",
          GST_RANK_NONE, GST_TYPE_GTK_GL_SINK)) {
    return FALSE;
  }
#endif

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gtk,
    "Gtk+ sink",
    plugin_init, PACKAGE_VERSION, GST_LICENSE, GST_PACKAGE_NAME,
    GST_PACKAGE_ORIGIN)
