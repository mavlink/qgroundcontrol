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

#include "gstrtpmp4vpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpmp4vpay_debug);
#define GST_CAT_DEFAULT (rtpmp4vpay_debug)

static GstStaticPadTemplate gst_rtp_mp4v_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpeg,"
        "mpegversion=(int) 4, systemstream=(boolean)false;" "video/x-divx")
    );

static GstStaticPadTemplate gst_rtp_mp4v_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"MP4V-ES\""
        /* two string params
         *
         "profile-level-id = (string) [1,MAX]"
         "config = (string) [1,MAX]"
         */
    )
    );

#define DEFAULT_CONFIG_INTERVAL 0

enum
{
  PROP_0,
  PROP_CONFIG_INTERVAL
};


static void gst_rtp_mp4v_pay_finalize (GObject * object);

static void gst_rtp_mp4v_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_mp4v_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_rtp_mp4v_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_mp4v_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);
static gboolean gst_rtp_mp4v_pay_sink_event (GstRTPBasePayload * pay,
    GstEvent * event);

#define gst_rtp_mp4v_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpMP4VPay, gst_rtp_mp4v_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_mp4v_pay_class_init (GstRtpMP4VPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->set_property = gst_rtp_mp4v_pay_set_property;
  gobject_class->get_property = gst_rtp_mp4v_pay_get_property;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp4v_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_mp4v_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP MPEG4 Video payloader", "Codec/Payloader/Network/RTP",
      "Payload MPEG-4 video as RTP packets (RFC 3016)",
      "Wim Taymans <wim.taymans@gmail.com>");

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CONFIG_INTERVAL,
      g_param_spec_int ("config-interval", "Config Send Interval",
          "Send Config Insertion Interval in seconds (configuration headers "
          "will be multiplexed in the data stream when detected.) "
          "(0 = disabled, -1 = send with every IDR frame)",
          -1, 3600, DEFAULT_CONFIG_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  gobject_class->finalize = gst_rtp_mp4v_pay_finalize;

  gstrtpbasepayload_class->set_caps = gst_rtp_mp4v_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_mp4v_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_mp4v_pay_sink_event;

  GST_DEBUG_CATEGORY_INIT (rtpmp4vpay_debug, "rtpmp4vpay", 0,
      "MP4 video RTP Payloader");
}

static void
gst_rtp_mp4v_pay_init (GstRtpMP4VPay * rtpmp4vpay)
{
  rtpmp4vpay->adapter = gst_adapter_new ();
  rtpmp4vpay->rate = 90000;
  rtpmp4vpay->profile = 1;
  rtpmp4vpay->need_config = TRUE;
  rtpmp4vpay->config_interval = DEFAULT_CONFIG_INTERVAL;
  rtpmp4vpay->last_config = -1;

  rtpmp4vpay->config = NULL;
}

static void
gst_rtp_mp4v_pay_finalize (GObject * object)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  if (rtpmp4vpay->config) {
    gst_buffer_unref (rtpmp4vpay->config);
    rtpmp4vpay->config = NULL;
  }
  g_object_unref (rtpmp4vpay->adapter);
  rtpmp4vpay->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_rtp_mp4v_pay_new_caps (GstRtpMP4VPay * rtpmp4vpay)
{
  gchar *profile, *config;
  GValue v = { 0 };
  gboolean res;

  profile = g_strdup_printf ("%d", rtpmp4vpay->profile);
  g_value_init (&v, GST_TYPE_BUFFER);
  gst_value_set_buffer (&v, rtpmp4vpay->config);
  config = gst_value_serialize (&v);

  res = gst_rtp_base_payload_set_outcaps (GST_RTP_BASE_PAYLOAD (rtpmp4vpay),
      "profile-level-id", G_TYPE_STRING, profile,
      "config", G_TYPE_STRING, config, NULL);

  g_value_unset (&v);

  g_free (profile);
  g_free (config);

  return res;
}

static gboolean
gst_rtp_mp4v_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstRtpMP4VPay *rtpmp4vpay;
  GstStructure *structure;
  const GValue *codec_data;
  gboolean res;

  rtpmp4vpay = GST_RTP_MP4V_PAY (payload);

  gst_rtp_base_payload_set_options (payload, "video", TRUE, "MP4V-ES",
      rtpmp4vpay->rate);

  res = TRUE;

  structure = gst_caps_get_structure (caps, 0);
  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    GST_LOG_OBJECT (rtpmp4vpay, "got codec_data");
    if (G_VALUE_TYPE (codec_data) == GST_TYPE_BUFFER) {
      GstBuffer *buffer;

      buffer = gst_value_get_buffer (codec_data);

      if (gst_buffer_get_size (buffer) < 5)
        goto done;

      gst_buffer_extract (buffer, 4, &rtpmp4vpay->profile, 1);
      GST_LOG_OBJECT (rtpmp4vpay, "configuring codec_data, profile %d",
          rtpmp4vpay->profile);

      if (rtpmp4vpay->config)
        gst_buffer_unref (rtpmp4vpay->config);
      rtpmp4vpay->config = gst_buffer_copy (buffer);
      res = gst_rtp_mp4v_pay_new_caps (rtpmp4vpay);
    }
  }

done:
  return res;
}

static void
gst_rtp_mp4v_pay_empty (GstRtpMP4VPay * rtpmp4vpay)
{
  gst_adapter_clear (rtpmp4vpay->adapter);
}

#define RTP_HEADER_LEN 12

static GstFlowReturn
gst_rtp_mp4v_pay_flush (GstRtpMP4VPay * rtpmp4vpay)
{
  guint avail, mtu;
  GstBuffer *outbuf;
  GstBuffer *outbuf_data = NULL;
  GstFlowReturn ret;
  GstBufferList *list = NULL;

  /* the data available in the adapter is either smaller
   * than the MTU or bigger. In the case it is smaller, the complete
   * adapter contents can be put in one packet. In the case the
   * adapter has more than one MTU, we need to split the MP4V data
   * over multiple packets. */
  avail = gst_adapter_available (rtpmp4vpay->adapter);

  if (rtpmp4vpay->config == NULL && rtpmp4vpay->need_config) {
    /* when we don't have a config yet, flush things out */
    gst_adapter_flush (rtpmp4vpay->adapter, avail);
    avail = 0;
  }

  if (!avail)
    return GST_FLOW_OK;

  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtpmp4vpay);

  /* Use buffer lists. Each frame will be put into a list
   * of buffers and the whole list will be pushed downstream
   * at once */
  list = gst_buffer_list_new_sized ((avail / (mtu - RTP_HEADER_LEN)) + 1);

  while (avail > 0) {
    guint towrite;
    guint payload_len;
    guint packet_len;
    GstRTPBuffer rtp = { NULL };

    /* this will be the total length of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (avail, 0, 0);

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, mtu);

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    /* create buffer without payload. The payload will be put
     * in next buffer instead. Both buffers will be merged */
    outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

    /* Take buffer with the payload from the adapter */
    outbuf_data = gst_adapter_take_buffer_fast (rtpmp4vpay->adapter,
        payload_len);

    avail -= payload_len;

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);
    gst_rtp_buffer_set_marker (&rtp, avail == 0);
    gst_rtp_buffer_unmap (&rtp);
    gst_rtp_copy_video_meta (rtpmp4vpay, outbuf, outbuf_data);
    outbuf = gst_buffer_append (outbuf, outbuf_data);

    GST_BUFFER_PTS (outbuf) = rtpmp4vpay->first_timestamp;

    /* add to list */
    gst_buffer_list_insert (list, -1, outbuf);
  }

  /* push the whole buffer list at once */
  ret =
      gst_rtp_base_payload_push_list (GST_RTP_BASE_PAYLOAD (rtpmp4vpay), list);

  return ret;
}

#define VOS_STARTCODE                   0x000001B0
#define VOS_ENDCODE                     0x000001B1
#define USER_DATA_STARTCODE             0x000001B2
#define GOP_STARTCODE                   0x000001B3
#define VISUAL_OBJECT_STARTCODE         0x000001B5
#define VOP_STARTCODE                   0x000001B6

static gboolean
gst_rtp_mp4v_pay_depay_data (GstRtpMP4VPay * enc, guint8 * data, guint size,
    gint * strip, gboolean * vopi)
{
  guint32 code;
  gboolean result;
  *vopi = FALSE;

  *strip = 0;

  if (size < 5)
    return FALSE;

  code = GST_READ_UINT32_BE (data);
  GST_DEBUG_OBJECT (enc, "start code 0x%08x", code);

  switch (code) {
    case VOS_STARTCODE:
    case 0x00000101:
    {
      gint i;
      guint8 profile;
      gboolean newprofile = FALSE;
      gboolean equal;

      if (code == VOS_STARTCODE) {
        /* profile_and_level_indication */
        profile = data[4];

        GST_DEBUG_OBJECT (enc, "VOS profile 0x%08x", profile);

        if (profile != enc->profile) {
          newprofile = TRUE;
          enc->profile = profile;
        }
      }

      /* up to the next GOP_STARTCODE or VOP_STARTCODE is
       * the config information */
      code = 0xffffffff;
      for (i = 5; i < size - 4; i++) {
        code = (code << 8) | data[i];
        if (code == GOP_STARTCODE || code == VOP_STARTCODE)
          break;
      }
      i -= 3;
      /* see if config changed */
      equal = FALSE;
      if (enc->config) {
        if (gst_buffer_get_size (enc->config) == i) {
          equal = gst_buffer_memcmp (enc->config, 0, data, i) == 0;
        }
      }
      /* if config string changed or new profile, make new caps */
      if (!equal || newprofile) {
        if (enc->config)
          gst_buffer_unref (enc->config);
        enc->config = gst_buffer_new_and_alloc (i);

        gst_buffer_fill (enc->config, 0, data, i);

        gst_rtp_mp4v_pay_new_caps (enc);
      }
      *strip = i;
      /* we need to flush out the current packet. */
      result = TRUE;
      break;
    }
    case VOP_STARTCODE:
      GST_DEBUG_OBJECT (enc, "VOP");
      /* VOP startcode, we don't have to flush the packet */
      result = FALSE;
      /* vop-coding-type == I-frame */
      if (size > 4 && (data[4] >> 6 == 0)) {
        GST_DEBUG_OBJECT (enc, "VOP-I");
        *vopi = TRUE;
      }
      break;
    case GOP_STARTCODE:
      GST_DEBUG_OBJECT (enc, "GOP");
      *vopi = TRUE;
      result = TRUE;
      break;
    case 0x00000100:
      enc->need_config = FALSE;
      result = TRUE;
      break;
    default:
      if (code >= 0x20 && code <= 0x2f) {
        GST_DEBUG_OBJECT (enc, "short header");
        result = FALSE;
      } else {
        GST_DEBUG_OBJECT (enc, "other startcode");
        /* all other startcodes need a flush */
        result = TRUE;
      }
      break;
  }
  return result;
}

/* we expect buffers starting on startcodes.
 */
static GstFlowReturn
gst_rtp_mp4v_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpMP4VPay *rtpmp4vpay;
  GstFlowReturn ret;
  guint avail;
  guint packet_len;
  GstMapInfo map;
  gsize size;
  gboolean flush;
  gint strip;
  GstClockTime timestamp, duration;
  gboolean vopi;
  gboolean send_config;
  GstClockTime running_time = GST_CLOCK_TIME_NONE;

  ret = GST_FLOW_OK;
  send_config = FALSE;

  rtpmp4vpay = GST_RTP_MP4V_PAY (basepayload);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  size = map.size;
  timestamp = GST_BUFFER_PTS (buffer);
  duration = GST_BUFFER_DURATION (buffer);
  avail = gst_adapter_available (rtpmp4vpay->adapter);

  if (duration == -1)
    duration = 0;

  /* empty buffer, take timestamp */
  if (avail == 0) {
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
  }

  /* depay incoming data and see if we need to start a new RTP
   * packet */
  flush =
      gst_rtp_mp4v_pay_depay_data (rtpmp4vpay, map.data, size, &strip, &vopi);
  gst_buffer_unmap (buffer, &map);

  if (strip) {
    /* strip off config if requested, do not strip off if the
     * config_interval is set to -1 */
    if (!(rtpmp4vpay->config_interval > 0)
        && !(rtpmp4vpay->config_interval == -1)) {
      GstBuffer *subbuf;

      GST_LOG_OBJECT (rtpmp4vpay, "stripping config at %d, size %d", strip,
          (gint) size - strip);

      /* strip off header */
      subbuf = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_ALL, strip,
          size - strip);
      GST_BUFFER_PTS (subbuf) = timestamp;
      gst_buffer_unref (buffer);
      buffer = subbuf;

      size = gst_buffer_get_size (buffer);
    } else {
      running_time =
          gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
          timestamp);

      GST_LOG_OBJECT (rtpmp4vpay, "found config in stream");
      rtpmp4vpay->last_config = running_time;
    }
  }

  /* there is a config request, see if we need to insert it */
  if (vopi && (rtpmp4vpay->config_interval > 0) && rtpmp4vpay->config) {
    running_time =
        gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
        timestamp);

    if (rtpmp4vpay->last_config != -1) {
      guint64 diff;

      GST_LOG_OBJECT (rtpmp4vpay,
          "now %" GST_TIME_FORMAT ", last VOP-I %" GST_TIME_FORMAT,
          GST_TIME_ARGS (running_time),
          GST_TIME_ARGS (rtpmp4vpay->last_config));

      /* calculate diff between last config in milliseconds */
      if (running_time > rtpmp4vpay->last_config) {
        diff = running_time - rtpmp4vpay->last_config;
      } else {
        diff = 0;
      }

      GST_DEBUG_OBJECT (rtpmp4vpay,
          "interval since last config %" GST_TIME_FORMAT, GST_TIME_ARGS (diff));

      /* bigger than interval, queue config */
      if (GST_TIME_AS_SECONDS (diff) >= rtpmp4vpay->config_interval) {
        GST_DEBUG_OBJECT (rtpmp4vpay, "time to send config");
        send_config = TRUE;
      }
    } else {
      /* no known previous config time, send now */
      GST_DEBUG_OBJECT (rtpmp4vpay, "no previous config time, send now");
      send_config = TRUE;
    }
  }

  if (vopi && (rtpmp4vpay->config_interval == -1)) {
    GST_DEBUG_OBJECT (rtpmp4vpay, "sending config before current IDR frame");
    /* send config before every IDR frame */
    send_config = TRUE;
  }

  if (send_config) {
    /* we need to send config now first */
    GST_LOG_OBJECT (rtpmp4vpay, "inserting config in stream");

    /* insert header */
    buffer = gst_buffer_append (gst_buffer_ref (rtpmp4vpay->config), buffer);

    GST_BUFFER_PTS (buffer) = timestamp;
    size = gst_buffer_get_size (buffer);

    if (running_time != -1) {
      rtpmp4vpay->last_config = running_time;
    }
  }

  /* if we need to flush, do so now */
  if (flush) {
    ret = gst_rtp_mp4v_pay_flush (rtpmp4vpay);
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
    avail = 0;
  }

  /* get packet length of data and see if we exceeded MTU. */
  packet_len = gst_rtp_buffer_calc_packet_len (avail + size, 0, 0);

  if (gst_rtp_base_payload_is_filled (basepayload,
          packet_len, rtpmp4vpay->duration + duration)) {
    ret = gst_rtp_mp4v_pay_flush (rtpmp4vpay);
    rtpmp4vpay->first_timestamp = timestamp;
    rtpmp4vpay->duration = 0;
  }

  /* push new data */
  gst_adapter_push (rtpmp4vpay->adapter, buffer);

  rtpmp4vpay->duration += duration;

  return ret;
}

static gboolean
gst_rtp_mp4v_pay_sink_event (GstRTPBasePayload * pay, GstEvent * event)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (pay);

  GST_DEBUG ("Got event: %s", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEGMENT:
    case GST_EVENT_EOS:
      /* This flush call makes sure that the last buffer is always pushed
       * to the base payloader */
      gst_rtp_mp4v_pay_flush (rtpmp4vpay);
      break;
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_mp4v_pay_empty (rtpmp4vpay);
      break;
    default:
      break;
  }

  /* let parent handle event too */
  return GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (pay, event);
}

static void
gst_rtp_mp4v_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      rtpmp4vpay->config_interval = g_value_get_int (value);
      break;
    default:
      break;
  }
}

static void
gst_rtp_mp4v_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpMP4VPay *rtpmp4vpay;

  rtpmp4vpay = GST_RTP_MP4V_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      g_value_set_int (value, rtpmp4vpay->config_interval);
      break;
    default:
      break;
  }
}

gboolean
gst_rtp_mp4v_pay_plugin_init (GstPlugin * plugin)
{
  /* Note: This element is marked at a "+1" rank to make sure that
   * auto-plugging of payloaders for MPEG4 elementary streams don't
   * end up using the 'rtpmp4gpay' element (generic mpeg4) which isn't
   * as well supported as this RFC */
  return gst_element_register (plugin, "rtpmp4vpay",
      GST_RANK_SECONDARY + 1, GST_TYPE_RTP_MP4V_PAY);
}
