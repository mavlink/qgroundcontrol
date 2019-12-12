/*
 * Siren Depayloader Gst Element
 *
 *   @author: Youness Alaoui <kakaroto@kakaroto.homelinux.net>
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

#include <string.h>
#include <stdlib.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>
#include "gstrtpsirendepay.h"
#include "gstrtputils.h"

static GstStaticPadTemplate gst_rtp_siren_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "clock-rate = (int) 16000, " "encoding-name = (string) \"SIREN\"")
    /* This is the default, so the peer doesn't have to specify it */
    /*  " "dct-length = (int) 320") */
    );

     static GstStaticPadTemplate gst_rtp_siren_depay_src_template =
         GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-siren, " "dct-length = (int) 320")
    );

     static GstBuffer *gst_rtp_siren_depay_process (GstRTPBaseDepayload *
    depayload, GstRTPBuffer * rtp);
     static gboolean gst_rtp_siren_depay_setcaps (GstRTPBaseDepayload *
    depayload, GstCaps * caps);

G_DEFINE_TYPE (GstRTPSirenDepay, gst_rtp_siren_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

     static void gst_rtp_siren_depay_class_init (GstRTPSirenDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_siren_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_siren_depay_setcaps;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_siren_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_siren_depay_sink_template);
  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Siren packet depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts Siren audio from RTP packets",
      "Philippe Kalaf <philippe.kalaf@collabora.co.uk>");
}

static void
gst_rtp_siren_depay_init (GstRTPSirenDepay * rtpsirendepay)
{

}

static gboolean
gst_rtp_siren_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstCaps *srccaps;
  gboolean ret;

  srccaps = gst_caps_new_simple ("audio/x-siren",
      "dct-length", G_TYPE_INT, 320, NULL);
  ret = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);

  GST_DEBUG ("set caps on source: %" GST_PTR_FORMAT " (ret=%d)", srccaps, ret);
  gst_caps_unref (srccaps);

  /* always fixed clock rate of 16000 */
  depayload->clock_rate = 16000;

  return ret;
}

static GstBuffer *
gst_rtp_siren_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstBuffer *outbuf;

  outbuf = gst_rtp_buffer_get_payload_buffer (rtp);

  if (outbuf) {
    gst_rtp_drop_non_audio_meta (depayload, outbuf);
  }

  return outbuf;
}

gboolean
gst_rtp_siren_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpsirendepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SIREN_DEPAY);
}
