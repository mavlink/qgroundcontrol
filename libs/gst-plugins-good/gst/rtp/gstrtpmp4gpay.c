/* GStreamer
 * Copyright (C) <2006> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/base/gstbitreader.h>
#include <gst/rtp/gstrtpbuffer.h>

#include "gstrtpmp4gpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpmp4gpay_debug);
#define GST_CAT_DEFAULT (rtpmp4gpay_debug)

static GstStaticPadTemplate gst_rtp_mp4g_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,"
        "mpegversion=(int) 4,"
        "systemstream=(boolean)false;"
        "audio/mpeg," "mpegversion=(int) 4, " "stream-format=(string) raw")
    );

static GstStaticPadTemplate gst_rtp_mp4g_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) { \"video\", \"audio\", \"application\" }, "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], "
        "encoding-name = (string) \"MPEG4-GENERIC\", "
        /* required string params */
        "streamtype = (string) { \"4\", \"5\" }, "      /* 4 = video, 5 = audio */
        /* "profile-level-id = (string) [1,MAX], " */
        /* "config = (string) [1,MAX]" */
        "mode = (string) { \"generic\", \"CELP-cbr\", \"CELP-vbr\", \"AAC-lbr\", \"AAC-hbr\" } "
        /* Optional general parameters */
        /* "objecttype = (string) [1,MAX], " */
        /* "constantsize = (string) [1,MAX], " *//* constant size of each AU */
        /* "constantduration = (string) [1,MAX], " *//* constant duration of each AU */
        /* "maxdisplacement = (string) [1,MAX], " */
        /* "de-interleavebuffersize = (string) [1,MAX], " */
        /* Optional configuration parameters */
        /* "sizelength = (string) [1, 16], " *//* max 16 bits, should be enough... */
        /* "indexlength = (string) [1, 8], " */
        /* "indexdeltalength = (string) [1, 8], " */
        /* "ctsdeltalength = (string) [1, 64], " */
        /* "dtsdeltalength = (string) [1, 64], " */
        /* "randomaccessindication = (string) {0, 1}, " */
        /* "streamstateindication = (string) [0, 64], " */
        /* "auxiliarydatasizelength = (string) [0, 64]" */ )
    );


static void gst_rtp_mp4g_pay_finalize (GObject * object);

static GstStateChangeReturn gst_rtp_mp4g_pay_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_rtp_mp4g_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mp4g_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);
static gboolean gst_rtp_mp4g_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);

#define gst_rtp_mp4g_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpMP4GPay, gst_rtp_mp4g_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_mp4g_pay_class_init (GstRtpMP4GPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->finalize = gst_rtp_mp4g_pay_finalize;

  gstelement_class->change_state = gst_rtp_mp4g_pay_change_state;

  gstrtpbasepayload_class->set_caps = gst_rtp_mp4g_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_mp4g_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_mp4g_pay_sink_event;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp4g_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp4g_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG4 ES payloader",
      "Codec/Payloader/Network/RTP",
      "Payload MPEG4 elementary streams as RTP packets (RFC 3640)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpmp4gpay_debug, "rtpmp4gpay", 0,
      "MP4-generic RTP Payloader");
}

static void
gst_rtp_mp4g_pay_init (GstRtpMP4GPay * rtpmp4gpay)
{
  rtpmp4gpay->adapter = gst_adapter_new ();
}

static void
gst_rtp_mp4g_pay_reset (GstRtpMP4GPay * rtpmp4gpay)
{
  GST_DEBUG_OBJECT (rtpmp4gpay, "reset");

  gst_adapter_clear (rtpmp4gpay->adapter);
}

static void
gst_rtp_mp4g_pay_cleanup (GstRtpMP4GPay * rtpmp4gpay)
{
  gst_rtp_mp4g_pay_reset (rtpmp4gpay);

  g_free (rtpmp4gpay->params);
  rtpmp4gpay->params = NULL;

  if (rtpmp4gpay->config)
    gst_buffer_unref (rtpmp4gpay->config);
  rtpmp4gpay->config = NULL;

  g_free (rtpmp4gpay->profile);
  rtpmp4gpay->profile = NULL;

  rtpmp4gpay->streamtype = NULL;
  rtpmp4gpay->mode = NULL;

  rtpmp4gpay->frame_len = 0;
}

static void
gst_rtp_mp4g_pay_finalize (GObject * object)
{
  GstRtpMP4GPay *rtpmp4gpay;

  rtpmp4gpay = GST_RTP_MP4G_PAY (object);

  gst_rtp_mp4g_pay_cleanup (rtpmp4gpay);

  g_object_unref (rtpmp4gpay->adapter);
  rtpmp4gpay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static const unsigned int sampling_table[16] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

static gboolean
gst_rtp_mp4g_pay_parse_audio_config (GstRtpMP4GPay * rtpmp4gpay,
    GstBuffer * buffer)
{
  GstMapInfo map;
  guint8 objectType = 0;
  guint8 samplingIdx = 0;
  guint8 channelCfg = 0;
  GstBitReader br;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  gst_bit_reader_init (&br, map.data, map.size);

  /* any object type is fine, we need to copy it to the profile-level-id field. */
  if (!gst_bit_reader_get_bits_uint8 (&br, &objectType, 5))
    goto too_short;
  if (objectType == 0)
    goto invalid_object;

  if (!gst_bit_reader_get_bits_uint8 (&br, &samplingIdx, 4))
    goto too_short;
  /* only fixed values for now */
  if (samplingIdx > 12 && samplingIdx != 15)
    goto wrong_freq;

  if (!gst_bit_reader_get_bits_uint8 (&br, &channelCfg, 4))
    goto too_short;
  if (channelCfg > 7)
    goto wrong_channels;

  /* rtp rate depends on sampling rate of the audio */
  if (samplingIdx == 15) {
    guint32 rate = 0;

    /* index of 15 means we get the rate in the next 24 bits */
    if (!gst_bit_reader_get_bits_uint32 (&br, &rate, 24))
      goto too_short;

    rtpmp4gpay->rate = rate;
  } else {
    /* else use the rate from the table */
    rtpmp4gpay->rate = sampling_table[samplingIdx];
  }

  rtpmp4gpay->frame_len = 1024;

  switch (objectType) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    {
      guint8 frameLenFlag = 0;

      if (gst_bit_reader_get_bits_uint8 (&br, &frameLenFlag, 1))
        if (frameLenFlag)
          rtpmp4gpay->frame_len = 960;

      break;
    }
    default:
      break;
  }

  /* extra rtp params contain the number of channels */
  g_free (rtpmp4gpay->params);
  rtpmp4gpay->params = g_strdup_printf ("%d", channelCfg);
  /* audio stream type */
  rtpmp4gpay->streamtype = "5";
  /* mode only high bitrate for now */
  rtpmp4gpay->mode = "AAC-hbr";
  /* profile */
  g_free (rtpmp4gpay->profile);
  rtpmp4gpay->profile = g_strdup_printf ("%d", objectType);

  GST_DEBUG_OBJECT (rtpmp4gpay,
      "objectType: %d, samplingIdx: %d (%d), channelCfg: %d, frame_len %d",
      objectType, samplingIdx, rtpmp4gpay->rate, channelCfg,
      rtpmp4gpay->frame_len);

  gst_buffer_unmap (buffer, &map);
  return TRUE;

  /* ERROR */
too_short:
  {
    GST_ELEMENT_ERROR (rtpmp4gpay, STREAM, FORMAT,
        (NULL), ("config string too short"));
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
invalid_object:
  {
    GST_ELEMENT_ERROR (rtpmp4gpay, STREAM, FORMAT,
        (NULL), ("invalid object type"));
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
wrong_freq:
  {
    GST_ELEMENT_ERROR (rtpmp4gpay, STREAM, NOT_IMPLEMENTED,
        (NULL), ("unsupported frequency index %d", samplingIdx));
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
wrong_channels:
  {
    GST_ELEMENT_ERROR (rtpmp4gpay, STREAM, NOT_IMPLEMENTED,
        (NULL), ("unsupported number of channels %d, must < 8", channelCfg));
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
}

#define VOS_STARTCODE                   0x000001B0

static gboolean
gst_rtp_mp4g_pay_parse_video_config (GstRtpMP4GPay * rtpmp4gpay,
    GstBuffer * buffer)
{
  GstMapInfo map;
  guint32 code;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  if (map.size < 5)
    goto too_short;

  code = GST_READ_UINT32_BE (map.data);

  g_free (rtpmp4gpay->profile);
  if (code == VOS_STARTCODE) {
    /* get profile */
    rtpmp4gpay->profile = g_strdup_printf ("%d", (gint) map.data[4]);
  } else {
    GST_ELEMENT_WARNING (rtpmp4gpay, STREAM, FORMAT,
        (NULL), ("profile not found in config string, assuming \'1\'"));
    rtpmp4gpay->profile = g_strdup ("1");
  }

  /* fixed rate */
  rtpmp4gpay->rate = 90000;
  /* video stream type */
  rtpmp4gpay->streamtype = "4";
  /* no params for video */
  rtpmp4gpay->params = NULL;
  /* mode */
  rtpmp4gpay->mode = "generic";

  GST_LOG_OBJECT (rtpmp4gpay, "profile %s", rtpmp4gpay->profile);

  gst_buffer_unmap (buffer, &map);

  return TRUE;

  /* ERROR */
too_short:
  {
    GST_ELEMENT_ERROR (rtpmp4gpay, STREAM, FORMAT,
        (NULL), ("config string too short"));
    gst_buffer_unmap (buffer, &map);
    return FALSE;
  }
}

static gboolean
gst_rtp_mp4g_pay_new_caps (GstRtpMP4GPay * rtpmp4gpay)
{
  gchar *config;
  GValue v = { 0 };
  gboolean res;

#define MP4GCAPS						\
  "streamtype", G_TYPE_STRING, rtpmp4gpay->streamtype, 		\
  "profile-level-id", G_TYPE_STRING, rtpmp4gpay->profile,	\
  "mode", G_TYPE_STRING, rtpmp4gpay->mode,			\
  "config", G_TYPE_STRING, config,				\
  "sizelength", G_TYPE_STRING, "13",				\
  "indexlength", G_TYPE_STRING, "3",				\
  "indexdeltalength", G_TYPE_STRING, "3",			\
  NULL

  g_value_init (&v, GST_TYPE_BUFFER);
  gst_value_set_buffer (&v, rtpmp4gpay->config);
  config = gst_value_serialize (&v);

  /* hmm, silly */
  if (rtpmp4gpay->params) {
    res = gst_rtp_base_payload_set_outcaps (GST_RTP_BASE_PAYLOAD (rtpmp4gpay),
        "encoding-params", G_TYPE_STRING, rtpmp4gpay->params, MP4GCAPS);
  } else {
    res = gst_rtp_base_payload_set_outcaps (GST_RTP_BASE_PAYLOAD (rtpmp4gpay),
        MP4GCAPS);
  }

  g_value_unset (&v);
  g_free (config);

#undef MP4GCAPS
  return res;
}

static gboolean
gst_rtp_mp4g_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstRtpMP4GPay *rtpmp4gpay;
  GstStructure *structure;
  const GValue *codec_data;
  const gchar *media_type = NULL;
  gboolean res;

  rtpmp4gpay = GST_RTP_MP4G_PAY (payload);

  structure = gst_caps_get_structure (caps, 0);

  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    GST_LOG_OBJECT (rtpmp4gpay, "got codec_data");
    if (G_VALUE_TYPE (codec_data) == GST_TYPE_BUFFER) {
      GstBuffer *buffer;
      const gchar *name;

      buffer = gst_value_get_buffer (codec_data);
      GST_LOG_OBJECT (rtpmp4gpay, "configuring codec_data");

      name = gst_structure_get_name (structure);

      /* parse buffer */
      if (!strcmp (name, "audio/mpeg")) {
        res = gst_rtp_mp4g_pay_parse_audio_config (rtpmp4gpay, buffer);
        media_type = "audio";
      } else if (!strcmp (name, "video/mpeg")) {
        res = gst_rtp_mp4g_pay_parse_video_config (rtpmp4gpay, buffer);
        media_type = "video";
      } else {
        res = FALSE;
      }
      if (!res)
        goto config_failed;

      /* now we can configure the buffer */
      if (rtpmp4gpay->config)
        gst_buffer_unref (rtpmp4gpay->config);

      rtpmp4gpay->config = gst_buffer_copy (buffer);
    }
  }
  if (media_type == NULL)
    goto config_failed;

  gst_rtp_base_payload_set_options (payload, media_type, TRUE, "MPEG4-GENERIC",
      rtpmp4gpay->rate);

  res = gst_rtp_mp4g_pay_new_caps (rtpmp4gpay);

  return res;

  /* ERRORS */
config_failed:
  {
    GST_DEBUG_OBJECT (rtpmp4gpay, "failed to parse config");
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_mp4g_pay_flush (GstRtpMP4GPay * rtpmp4gpay)
{
  guint avail, total;
  GstBuffer *outbuf;
  GstFlowReturn ret;
  guint mtu;

  /* the data available in the adapter is either smaller
   * than the MTU or bigger. In the case it is smaller, the complete
   * adapter contents can be put in one packet. In the case the
   * adapter has more than one MTU, we need to fragment the MPEG data
   * over multiple packets. */
  total = avail = gst_adapter_available (rtpmp4gpay->adapter);

  ret = GST_FLOW_OK;
  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtpmp4gpay);

  while (avail > 0) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    guint packet_len;
    GstRTPBuffer rtp = { NULL };
    GstBuffer *paybuf;

    /* this will be the total length of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (avail, 0, 0);

    /* fill one MTU or all available bytes, we need 4 spare bytes for
     * the AU header. */
    towrite = MIN (packet_len, mtu - 4);

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    GST_DEBUG_OBJECT (rtpmp4gpay,
        "avail %d, towrite %d, packet_len %d, payload_len %d", avail, towrite,
        packet_len, payload_len);

    /* create buffer to hold the payload, also make room for the 4 header bytes. */
    outbuf = gst_rtp_buffer_new_allocate (4, 0, 0);

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);

    /* copy payload */
    payload = gst_rtp_buffer_get_payload (&rtp);

    /* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
     * |AU-headers-length|AU-header|AU-header|      |AU-header|padding|
     * |                 |   (1)   |   (2)   |      |   (n)   | bits  |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- .. -+-+-+-+-+-+-+-+-+-+
     */
    /* AU-headers-length, we only have 1 AU-header */
    payload[0] = 0x00;
    payload[1] = 0x10;          /* we use 16 bits for the header */

    /* +---------------------------------------+
     * |     AU-size                           |
     * +---------------------------------------+
     * |     AU-Index / AU-Index-delta         |
     * +---------------------------------------+
     * |     CTS-flag                          |
     * +---------------------------------------+
     * |     CTS-delta                         |
     * +---------------------------------------+
     * |     DTS-flag                          |
     * +---------------------------------------+
     * |     DTS-delta                         |
     * +---------------------------------------+
     * |     RAP-flag                          |
     * +---------------------------------------+
     * |     Stream-state                      |
     * +---------------------------------------+
     */
    /* The AU-header, no CTS, DTS, RAP, Stream-state 
     *
     * AU-size is always the total size of the AU, not the fragmented size 
     */
    payload[2] = (total & 0x1fe0) >> 5;
    payload[3] = (total & 0x1f) << 3;   /* we use 13 bits for the size, 3 bits index */

    /* marker only if the packet is complete */
    gst_rtp_buffer_set_marker (&rtp, avail <= payload_len);

    gst_rtp_buffer_unmap (&rtp);

    paybuf = gst_adapter_take_buffer_fast (rtpmp4gpay->adapter, payload_len);
    gst_rtp_copy_meta (GST_ELEMENT_CAST (rtpmp4gpay), outbuf, paybuf, 0);
    outbuf = gst_buffer_append (outbuf, paybuf);

    GST_BUFFER_PTS (outbuf) = rtpmp4gpay->first_timestamp;
    GST_BUFFER_DURATION (outbuf) = rtpmp4gpay->first_duration;

    GST_BUFFER_OFFSET (outbuf) = GST_BUFFER_OFFSET_NONE;

    if (rtpmp4gpay->discont) {
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
      /* Only the first outputted buffer has the DISCONT flag */
      rtpmp4gpay->discont = FALSE;
    }

    ret = gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (rtpmp4gpay), outbuf);

    avail -= payload_len;
  }

  return ret;
}

/* we expect buffers as exactly one complete AU
 */
static GstFlowReturn
gst_rtp_mp4g_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpMP4GPay *rtpmp4gpay;

  rtpmp4gpay = GST_RTP_MP4G_PAY (basepayload);

  rtpmp4gpay->first_timestamp = GST_BUFFER_PTS (buffer);
  rtpmp4gpay->first_duration = GST_BUFFER_DURATION (buffer);
  rtpmp4gpay->discont = GST_BUFFER_IS_DISCONT (buffer);

  /* we always encode and flush a full AU */
  gst_adapter_push (rtpmp4gpay->adapter, buffer);

  return gst_rtp_mp4g_pay_flush (rtpmp4gpay);
}

static gboolean
gst_rtp_mp4g_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  GstRtpMP4GPay *rtpmp4gpay;

  rtpmp4gpay = GST_RTP_MP4G_PAY (payload);

  GST_DEBUG ("Got event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    case GST_EVENT_EOS:
      /* This flush call makes sure that the last buffer is always pushed
       * to the base payloader */
      gst_rtp_mp4g_pay_flush (rtpmp4gpay);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mp4g_pay_reset (rtpmp4gpay);
      break;
    default:
      break;
  }

  /* let parent handle event too */
  return GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload, event);
}

static GstStateChangeReturn
gst_rtp_mp4g_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstRtpMP4GPay *rtpmp4gpay;

  rtpmp4gpay = GST_RTP_MP4G_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_mp4g_pay_cleanup (rtpmp4gpay);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_mp4g_pay_cleanup (rtpmp4gpay);
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_mp4g_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpmp4gpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_MP4G_PAY);
}
