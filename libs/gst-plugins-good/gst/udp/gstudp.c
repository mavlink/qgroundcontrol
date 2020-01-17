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

#include <gst/net/gstnetaddressmeta.h>

#include "gstudpsrc.h"
#include "gstmultiudpsink.h"
#include "gstudpsink.h"
#include "gstdynudpsink.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  /* not using GLIB_CHECK_VERSION on purpose, run-time version matters */
  if (glib_check_version (2, 36, 0) != NULL) {
    GST_WARNING ("Your GLib version is < 2.36, UDP multicasting support may "
        "be broken, see https://bugzilla.gnome.org/show_bug.cgi?id=688378");
  }

  /* register info of the netaddress metadata so that we can use it from
   * multiple threads right away. Note that the plugin loading is always
   * serialized */
  gst_net_address_meta_get_info ();

  if (!gst_element_register (plugin, "udpsink", GST_RANK_NONE,
          GST_TYPE_UDPSINK))
    return FALSE;

  if (!gst_element_register (plugin, "multiudpsink", GST_RANK_NONE,
          GST_TYPE_MULTIUDPSINK))
    return FALSE;

  if (!gst_element_register (plugin, "dynudpsink", GST_RANK_NONE,
          GST_TYPE_DYNUDPSINK))
    return FALSE;

  if (!gst_element_register (plugin, "udpsrc", GST_RANK_NONE, GST_TYPE_UDPSRC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    udp,
    "transfer data via UDP",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
