/* GStreamer
 * Copyright (C) 1999,2000 Erik Walthinsen <omega@cse.ogi.edu>
 *                    2000 Wim Taymans <wtay@chello.be>
 *                    2006 Wim Taymans <wim@fluendo.com>
 *                    2006 David A. Schleef <ds@schleef.org>
 *
 * gstmultifilesink.c:
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
#  include "config.h"
#endif

#include <gst/gst.h>

#include "gstmultifilesink.h"
#include "gstmultifilesrc.h"
#include "gstsplitfilesrc.h"
#include "gstsplitmuxsink.h"
#include "gstsplitmuxsrc.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gst_element_register (plugin, "multifilesrc", GST_RANK_NONE,
      gst_multi_file_src_get_type ());
  gst_element_register (plugin, "multifilesink", GST_RANK_NONE,
      gst_multi_file_sink_get_type ());
  gst_element_register (plugin, "splitfilesrc", GST_RANK_NONE,
      gst_split_file_src_get_type ());

  if (!register_splitmuxsink (plugin))
    return FALSE;

  if (!register_splitmuxsrc (plugin))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    multifile,
    "Reads/Writes buffers from/to sequentially named files",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
