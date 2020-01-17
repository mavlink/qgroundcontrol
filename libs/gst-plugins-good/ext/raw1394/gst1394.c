/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
#include <gst/gst.h>


#include "gstdv1394src.h"
#ifdef HAVE_LIBIEC61883
#include "gsthdv1394src.h"
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "dv1394src", GST_RANK_NONE,
          GST_TYPE_DV1394SRC))
    return FALSE;
#ifdef HAVE_LIBIEC61883
  if (!gst_element_register (plugin, "hdv1394src", GST_RANK_NONE,
          GST_TYPE_HDV1394SRC))
    return FALSE;
#endif

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    1394,
    "Source for video data via IEEE1394 interface",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
