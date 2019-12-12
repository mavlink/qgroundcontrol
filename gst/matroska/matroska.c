/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * matroska.c: plugin loader
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

#include "matroska-demux.h"
#include "matroska-parse.h"
#include "matroska-read-common.h"
#include "matroska-mux.h"
#include "matroska-ids.h"
#include "webm-mux.h"

#include <gst/pbutils/pbutils.h>

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret;

  gst_pb_utils_init ();

  gst_matroska_register_tags ();

  GST_DEBUG_CATEGORY_INIT (matroskareadcommon_debug, "matroskareadcommon", 0,
      "Matroska demuxer/parser shared debug");

  ret = gst_matroska_demux_plugin_init (plugin);
  ret &= gst_matroska_parse_plugin_init (plugin);
  ret &= gst_element_register (plugin, "matroskamux", GST_RANK_PRIMARY,
      GST_TYPE_MATROSKA_MUX);
  ret &= gst_element_register (plugin, "webmmux", GST_RANK_PRIMARY,
      GST_TYPE_WEBM_MUX);

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    matroska,
    "Matroska and WebM stream handling",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
