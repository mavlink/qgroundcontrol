/* GStreamer
 * Copyright (C) <2008> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/rtp/gstrtpbuffer.h>

#include <string.h>
#include "gstrtpmp1sdepay.h"
#include "gstrtputils.h"

/* RtpMP1SDepay signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LAST
};

static GstStaticPadTemplate gst_rtp_mp1s_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,systemstream=(boolean)true")
    );

/* The spec says video/MP1S but I have seen streams with other/MP1S so we will
 * allow them both */
static GstStaticPadTemplate gst_rtp_mp1s_depay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"other\", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"MP1S\";"
        "application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"MP1S\"")
    );

G_DEFINE_TYPE (GstRtpMP1SDepay, gst_rtp_mp1s_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_mp1s_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_mp1s_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void
gst_rtp_mp1s_depay_class_init (GstRtpMP1SDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_mp1s_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_mp1s_depay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp1s_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp1s_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG1 System Stream depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts MPEG1 System Streams from RTP packets (RFC 3555)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_mp1s_depay_init (GstRtpMP1SDepay * rtpmp1sdepay)
{
}

static gboolean
gst_rtp_mp1s_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstCaps *srccaps;
  GstStructure *structure;
  gint clock_rate;
  gboolean res;

  structure = gst_caps_get_structure (caps, 0);
  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;         /* default */
  depayload->clock_rate = clock_rate;

  srccaps = gst_caps_new_simple ("video/mpeg",
      "systemstream", G_TYPE_BOOLEAN, TRUE, NULL);
  res = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);
  gst_caps_unref (srccaps);

  return res;
}

static GstBuffer *
gst_rtp_mp1s_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstBuffer *outbuf;

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);

  if (outbuf) {
    GST_DEBUG ("gst_rtp_mp1s_depay_chain: pushing buffer of size %"
        G_GSIZE_FORMAT, gst_buffer_get_size (outbuf));

    gst_rtp_drop_meta (GST_ELEMENT_CAST (depayload), outbuf, 0);
  }

  return outbuf;
}

gboolean
gst_rtp_mp1s_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp1sdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP1S_DEPAY);
}
