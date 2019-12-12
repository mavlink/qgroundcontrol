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

#include <gst/audio/audio.h>

#include "alaw-encode.h"
#include "alaw-decode.h"

GstStaticPadTemplate alaw_dec_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 8000, 192000 ], " "channels = (int) [ 1, 2 ]")
    );

GstStaticPadTemplate alaw_dec_sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-alaw, "
        "rate = [ 8000 , 192000 ], " "channels = [ 1 , 2 ]")
    );

GstStaticPadTemplate alaw_enc_sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 8000, 192000 ], " "channels = (int) [ 1, 2 ]")
    );

GstStaticPadTemplate alaw_enc_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-alaw, "
        "rate = [ 8000 , 192000 ], " "channels = [ 1 , 2 ]")
    );

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "alawenc",
          GST_RANK_PRIMARY, GST_TYPE_ALAW_ENC) ||
      !gst_element_register (plugin, "alawdec",
          GST_RANK_PRIMARY, GST_TYPE_ALAW_DEC))
    return FALSE;

  return TRUE;
}

/* FIXME 0.11: merge alaw and mulaw into one plugin? */
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    alaw,
    "ALaw audio conversion routines",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
