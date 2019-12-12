/* GStreamer PCM/A-Law conversions
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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
#include "mulaw-encode.h"
#include "mulaw-decode.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define INT_FORMAT "S16LE"
#else
#define INT_FORMAT "S16BE"
#endif

GstStaticPadTemplate mulaw_dec_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " INT_FORMAT ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 8000, 192000 ], " "channels = (int) [ 1, 2 ]")
    );

GstStaticPadTemplate mulaw_dec_sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-mulaw, "
        "rate = [ 8000 , 192000 ], " "channels = [ 1 , 2 ]")
    );

GstStaticPadTemplate mulaw_enc_sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " INT_FORMAT ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 8000, 192000 ], " "channels = (int) [ 1, 2 ]")
    );

GstStaticPadTemplate mulaw_enc_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-mulaw, "
        "rate = [ 8000 , 192000 ], " "channels = [ 1 , 2 ]")
    );

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "mulawenc",
          GST_RANK_PRIMARY, GST_TYPE_MULAWENC) ||
      !gst_element_register (plugin, "mulawdec",
          GST_RANK_PRIMARY, GST_TYPE_MULAWDEC))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    mulaw,
    "MuLaw audio conversion routines",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
