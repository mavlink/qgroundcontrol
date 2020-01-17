/* GStreamer taglib-based muxer base class
 * Copyright (C) 2006 Christophe Fergeau  <teuf@gnome.org>
 * Copyright (C) 2006 Tim-Philipp Müller <tim centricular net>
 * Copyright (C) 2006 Sebastian Dröge <slomo@circular-chaos.org>

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
#include <config.h>
#endif

#include "gstapev2mux.h"
#include "gstid3v2mux.h"

#include <gst/tag/tag.h>

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "id3v2mux", GST_RANK_NONE,
          GST_TYPE_ID3V2_MUX) ||
      !gst_element_register (plugin, "apev2mux", GST_RANK_NONE,
          GST_TYPE_APEV2_MUX))
    return FALSE;

  gst_tag_register_musicbrainz_tags ();

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    taglib,
    "Tag writing plug-in based on taglib",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
