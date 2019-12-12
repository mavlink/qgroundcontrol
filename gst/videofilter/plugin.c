/* GStreamer
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

#include "gstgamma.h"
#include "gstvideoflip.h"
#include "gstvideobalance.h"
#include "gstvideomedian.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  return (gst_element_register (plugin, "gamma", GST_RANK_NONE, GST_TYPE_GAMMA)
      && gst_element_register (plugin, "videobalance", GST_RANK_NONE,
          GST_TYPE_VIDEO_BALANCE)
      && gst_element_register (plugin, "videoflip", GST_RANK_NONE,
          GST_TYPE_VIDEO_FLIP)
      && gst_element_register (plugin, "videomedian", GST_RANK_NONE,
          GST_TYPE_VIDEO_MEDIAN));
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    videofilter,
    "Video filters plugin",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
