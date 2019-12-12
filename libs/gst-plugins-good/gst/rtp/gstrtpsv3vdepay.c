/* GStreamer
 * Copyright (C) <2005> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "gstrtpsv3vdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY (rtpsv3vdepay_debug);
#define GST_CAT_DEFAULT rtpsv3vdepay_debug

static GstStaticPadTemplate gst_rtp_sv3v_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-svq, " "svqversion = (int) 3")
    );

static GstStaticPadTemplate gst_rtp_sv3v_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) 90000, "
        "encoding-name = (string) { \"X-SV3V-ES\", \"X-SORENSON-VIDEO\" , \"X-SORENSONVIDEO\" , \"X-SorensonVideo\" }")
    );

#define gst_rtp_sv3v_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpSV3VDepay, gst_rtp_sv3v_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_sv3v_depay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_sv3v_depay_change_state (GstElement *
    element, GstStateChange transition);

static GstBuffer *gst_rtp_sv3v_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
gboolean gst_rtp_sv3v_depay_setcaps (GstRTPBaseDepayload * filter,
    GstCaps * caps);

static void
gst_rtp_sv3v_depay_class_init (GstRtpSV3VDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_sv3v_depay_process;
  gstrtpbasedepayload_class->set_caps = gst_rtp_sv3v_depay_setcaps;

  gobject_class->finalize = gst_rtp_sv3v_depay_finalize;

  gstelement_class->change_state = gst_rtp_sv3v_depay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_sv3v_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_sv3v_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP SVQ3 depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts SVQ3 video from RTP packets (no RFC)",
      "Wim Taymans <wim.taymans@gmail.com>");
}

static void
gst_rtp_sv3v_depay_init (GstRtpSV3VDepay * rtpsv3vdepay)
{
  rtpsv3vdepay->adapter = gst_adapter_new ();
}

static void
gst_rtp_sv3v_depay_finalize (GObject * object)
{
  GstRtpSV3VDepay *rtpsv3vdepay;

  rtpsv3vdepay = GST_RTP_SV3V_DEPAY (object);

  g_object_unref (rtpsv3vdepay->adapter);
  rtpsv3vdepay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/* only on the sink */
gboolean
gst_rtp_sv3v_depay_setcaps (GstRTPBaseDepayload * filter, GstCaps * caps)
{
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gint clock_rate;

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;         /* default */
  filter->clock_rate = clock_rate;

  /* will set caps later */

  return TRUE;
}

static GstBuffer *
gst_rtp_sv3v_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpSV3VDepay *rtpsv3vdepay;
  static struct
  {
    guint width, height;
  } resolutions[7] = {
    {
    160, 128}, {
    128, 96}, {
    176, 144}, {
    352, 288}, {
    704, 576}, {
    240, 180}, {
    320, 240}
  };
  gint payload_len;
  guint8 *payload;
  gboolean M;
  gboolean C, S, E;
  GstBuffer *outbuf = NULL;
  guint16 seq;

  rtpsv3vdepay = GST_RTP_SV3V_DEPAY (depayload);


  /* flush on sequence number gaps */
  seq = gst_rtp_buffer_get_seq (rtp);

  GST_DEBUG ("timestamp %" GST_TIME_FORMAT ", sequence number:%d",
      GST_TIME_ARGS (GST_BUFFER_PTS (rtp->buffer)), seq);

  if (seq != rtpsv3vdepay->nextseq) {
    GST_DEBUG ("Sequence discontinuity, clearing adapter");
    gst_adapter_clear (rtpsv3vdepay->adapter);
  }
  rtpsv3vdepay->nextseq = seq + 1;

  payload_len = gst_rtp_buffer_get_payload_len (rtp);
  if (payload_len < 3)
    goto bad_packet;

  payload = gst_rtp_buffer_get_payload (rtp);

  M = gst_rtp_buffer_get_marker (rtp);

  /* This is all a guess:
   *                      1 1 1 1 1 1
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |0|C|S|E|0|0|0|0|0|0|0|0|0|0|0|0|
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * C: config, packet contains config info
   * S: start, packet contains start of frame
   * E: end, packet contains end of frame
   */
  /* this seems to indicate a packet with a config string sent before each
   * keyframe */
  C = (payload[0] & 0x40) == 0x40;

  /* redundant with the RTP marker bit */
  S = (payload[0] & 0x20) == 0x20;
  E = (payload[0] & 0x10) == 0x10;

  GST_DEBUG ("M:%d, C:%d, S:%d, E:%d", M, C, S, E);

  GST_MEMDUMP ("incoming buffer", payload, payload_len);

  if (G_UNLIKELY (C)) {
    GstCaps *caps;
    GstBuffer *codec_data;
    GstMapInfo cmap;
    guint8 res;

    GST_DEBUG ("Configuration packet");

    /* if we already have caps, we don't need to do anything. FIXME, check if
     * something changed. */
    if (G_UNLIKELY (gst_pad_has_current_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD
                (depayload)))) {
      GST_DEBUG ("Already configured, skipping config parsing");
      goto beach;
    }

    res = payload[2] >> 5;

    /* width and height, according to http://wiki.multimedia.cx/index.php?title=Sorenson_Video_1#Stream_Format_And_Header */
    if (G_LIKELY (res < 7)) {
      rtpsv3vdepay->width = resolutions[res].width;
      rtpsv3vdepay->height = resolutions[res].height;
    } else {
      /* extended width/height, they're contained in the following 24bit */
      rtpsv3vdepay->width = ((payload[2] & 0x1f) << 7) | (payload[3] >> 1);
      rtpsv3vdepay->height =
          (payload[3] & 0x1) << 11 | payload[4] << 3 | (payload[5] >> 5);
    }

    /* CodecData needs to be 'SEQH' + len (32bit) + data according to
     * ffmpeg's libavcodec/svq3.c:svq3_decode_init */
    codec_data = gst_buffer_new_and_alloc (payload_len + 6);
    gst_buffer_map (codec_data, &cmap, GST_MAP_WRITE);
    memcpy (cmap.data, "SEQH", 4);
    GST_WRITE_UINT32_LE (cmap.data + 4, payload_len - 2);
    memcpy (cmap.data + 8, payload + 2, payload_len - 2);
    GST_MEMDUMP ("codec_data", cmap.data, gst_buffer_get_size (codec_data));
    gst_buffer_unmap (codec_data, &cmap);

    caps = gst_caps_new_simple ("video/x-svq",
        "svqversion", G_TYPE_INT, 3,
        "width", G_TYPE_INT, rtpsv3vdepay->width,
        "height", G_TYPE_INT, rtpsv3vdepay->height,
        "codec_data", GST_TYPE_BUFFER, codec_data, NULL);
    gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), caps);
    gst_caps_unref (caps);

    GST_DEBUG ("Depayloader now configured");

    rtpsv3vdepay->configured = TRUE;

    goto beach;
  }

  if (G_LIKELY (rtpsv3vdepay->configured)) {
    GstBuffer *tmpbuf;

    GST_DEBUG ("Storing incoming payload");
    /* store data in adapter, stip off 2 bytes header */
    tmpbuf = gst_rtp_buffer_get_payload_subbuffer (rtp, 2, -1);
    gst_adapter_push (rtpsv3vdepay->adapter, tmpbuf);

    if (G_UNLIKELY (M)) {
      /* frame is completed: push contents of adapter */
      guint avail;

      avail = gst_adapter_available (rtpsv3vdepay->adapter);
      GST_DEBUG ("Returning completed output buffer [%d bytes]", avail);
      outbuf = gst_adapter_take_buffer (rtpsv3vdepay->adapter, avail);
      gst_rtp_drop_non_video_meta (rtpsv3vdepay, outbuf);
    }
  }

beach:
  return outbuf;

  /* ERRORS */
bad_packet:
  {
    GST_ELEMENT_WARNING (rtpsv3vdepay, STREAM, DECODE,
        (NULL), ("Packet was too short"));
    return NULL;
  }
}

static GstStateChangeReturn
gst_rtp_sv3v_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpSV3VDepay *rtpsv3vdepay;
  GstStateChangeReturn ret;

  rtpsv3vdepay = GST_RTP_SV3V_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_adapter_clear (rtpsv3vdepay->adapter);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_sv3v_depay_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (rtpsv3vdepay_debug, "rtpsv3vdepay", 0,
      "RTP SV3V depayloader");

  return gst_element_register (plugin, "rtpsv3vdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_SV3V_DEPAY);
}
