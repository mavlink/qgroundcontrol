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

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/audio/audio.h>

#include "fnv1hash.h"
#include "gstrtpvorbispay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpvorbispay_debug);
#define GST_CAT_DEFAULT (rtpvorbispay_debug)

/* references:
 * http://www.rfc-editor.org/rfc/rfc5215.txt
 */

static GstStaticPadTemplate gst_rtp_vorbis_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) [1, MAX ], " "encoding-name = (string) \"VORBIS\""
        /* All required parameters
         *
         * "encoding-params = (string) <num channels>"
         * "configuration = (string) ANY"
         */
    )
    );

static GstStaticPadTemplate gst_rtp_vorbis_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-vorbis")
    );

#define DEFAULT_CONFIG_INTERVAL 0

enum
{
  PROP_0,
  PROP_CONFIG_INTERVAL
};

#define gst_rtp_vorbis_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpVorbisPay, gst_rtp_vorbis_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static gboolean gst_rtp_vorbis_pay_setcaps (GstRTPBasePayload * basepayload,
    GstCaps * caps);
static GstStateChangeReturn gst_rtp_vorbis_pay_change_state (GstElement *
    element, GstStateChange transition);
static GstFlowReturn gst_rtp_vorbis_pay_handle_buffer (GstRTPBasePayload * pad,
    GstBuffer * buffer);
static gboolean gst_rtp_vorbis_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);

static gboolean gst_rtp_vorbis_pay_parse_id (GstRTPBasePayload * basepayload,
    guint8 * data, guint size);
static gboolean gst_rtp_vorbis_pay_finish_headers (GstRTPBasePayload *
    basepayload);

static void gst_rtp_vorbis_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_vorbis_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void
gst_rtp_vorbis_pay_class_init (GstRtpVorbisPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gstelement_class->change_state = gst_rtp_vorbis_pay_change_state;

  gstrtpbasepayload_class->set_caps = gst_rtp_vorbis_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_vorbis_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_vorbis_pay_sink_event;

  gobject_class->set_property = gst_rtp_vorbis_pay_set_property;
  gobject_class->get_property = gst_rtp_vorbis_pay_get_property;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vorbis_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vorbis_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Vorbis payloader",
      "Codec/Payloader/Network/RTP",
      "Payload-encode Vorbis audio into RTP packets (RFC 5215)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpvorbispay_debug, "rtpvorbispay", 0,
      "Vorbis RTP Payloader");

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CONFIG_INTERVAL,
      g_param_spec_uint ("config-interval", "Config Send Interval",
          "Send Config Insertion Interval in seconds (configuration headers "
          "will be multiplexed in the data stream when detected.) (0 = disabled)",
          0, 3600, DEFAULT_CONFIG_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );
}

static void
gst_rtp_vorbis_pay_init (GstRtpVorbisPay * rtpvorbispay)
{
  rtpvorbispay->last_config = GST_CLOCK_TIME_NONE;
}

static void
gst_rtp_vorbis_pay_clear_packet (GstRtpVorbisPay * rtpvorbispay)
{
  if (rtpvorbispay->packet)
    gst_buffer_unref (rtpvorbispay->packet);
  rtpvorbispay->packet = NULL;
  g_list_free_full (rtpvorbispay->packet_buffers,
      (GDestroyNotify) gst_buffer_unref);
  rtpvorbispay->packet_buffers = NULL;
}

static void
gst_rtp_vorbis_pay_cleanup (GstRtpVorbisPay * rtpvorbispay)
{
  gst_rtp_vorbis_pay_clear_packet (rtpvorbispay);
  g_list_free_full (rtpvorbispay->headers, (GDestroyNotify) gst_buffer_unref);
  rtpvorbispay->headers = NULL;
  g_free (rtpvorbispay->config_data);
  rtpvorbispay->config_data = NULL;
  rtpvorbispay->last_config = GST_CLOCK_TIME_NONE;
}

static gboolean
gst_rtp_vorbis_pay_setcaps (GstRTPBasePayload * basepayload, GstCaps * caps)
{
  GstRtpVorbisPay *rtpvorbispay;
  GstStructure *s;
  const GValue *array;
  gint asize, i;
  GstBuffer *buf;
  GstMapInfo map;

  rtpvorbispay = GST_RTP_VORBIS_PAY (basepayload);

  s = gst_caps_get_structure (caps, 0);

  rtpvorbispay->need_headers = TRUE;

  if ((array = gst_structure_get_value (s, "streamheader")) == NULL)
    goto done;

  if (G_VALUE_TYPE (array) != GST_TYPE_ARRAY)
    goto done;

  if ((asize = gst_value_array_get_size (array)) < 3)
    goto done;

  for (i = 0; i < asize; i++) {
    const GValue *value;

    value = gst_value_array_get_value (array, i);
    if ((buf = gst_value_get_buffer (value)) == NULL)
      goto null_buffer;

    gst_buffer_map (buf, &map, GST_MAP_READ);
    if (map.size < 1)
      goto invalid_streamheader;

    /* no data packets allowed */
    if ((map.data[0] & 1) == 0)
      goto invalid_streamheader;

    /* we need packets with id 1, 3, 5 */
    if (map.data[0] != (i * 2) + 1)
      goto invalid_streamheader;

    if (i == 0) {
      /* identification, we need to parse this in order to get the clock rate. */
      if (G_UNLIKELY (!gst_rtp_vorbis_pay_parse_id (basepayload, map.data,
                  map.size)))
        goto parse_id_failed;
    }
    GST_DEBUG_OBJECT (rtpvorbispay, "collecting header %d", i);
    rtpvorbispay->headers =
        g_list_append (rtpvorbispay->headers, gst_buffer_ref (buf));
    gst_buffer_unmap (buf, &map);
  }
  if (!gst_rtp_vorbis_pay_finish_headers (basepayload))
    goto finish_failed;

done:
  return TRUE;

  /* ERRORS */
null_buffer:
  {
    GST_WARNING_OBJECT (rtpvorbispay, "streamheader with null buffer received");
    return FALSE;
  }
invalid_streamheader:
  {
    GST_WARNING_OBJECT (rtpvorbispay, "unable to parse initial header");
    gst_buffer_unmap (buf, &map);
    return FALSE;
  }
parse_id_failed:
  {
    GST_WARNING_OBJECT (rtpvorbispay, "unable to parse initial header");
    gst_buffer_unmap (buf, &map);
    return FALSE;
  }
finish_failed:
  {
    GST_WARNING_OBJECT (rtpvorbispay, "unable to finish headers");
    return FALSE;
  }
}

static void
gst_rtp_vorbis_pay_reset_packet (GstRtpVorbisPay * rtpvorbispay, guint8 VDT)
{
  guint payload_len;
  GstRTPBuffer rtp = { NULL };

  GST_LOG_OBJECT (rtpvorbispay, "reset packet");

  rtpvorbispay->payload_pos = 4;
  gst_rtp_buffer_map (rtpvorbispay->packet, GST_MAP_READ, &rtp);
  payload_len = gst_rtp_buffer_get_payload_len (&rtp);
  gst_rtp_buffer_unmap (&rtp);
  rtpvorbispay->payload_left = payload_len - 4;
  rtpvorbispay->payload_duration = 0;
  rtpvorbispay->payload_F = 0;
  rtpvorbispay->payload_VDT = VDT;
  rtpvorbispay->payload_pkts = 0;
}

static void
gst_rtp_vorbis_pay_init_packet (GstRtpVorbisPay * rtpvorbispay, guint8 VDT,
    GstClockTime timestamp)
{
  GST_LOG_OBJECT (rtpvorbispay, "starting new packet, VDT: %d", VDT);

  gst_rtp_vorbis_pay_clear_packet (rtpvorbispay);

  /* new packet allocate max packet size */
  rtpvorbispay->packet =
      gst_rtp_buffer_new_allocate_len (GST_RTP_BASE_PAYLOAD_MTU
      (rtpvorbispay), 0, 0);
  gst_rtp_vorbis_pay_reset_packet (rtpvorbispay, VDT);

  GST_BUFFER_PTS (rtpvorbispay->packet) = timestamp;
}

static GstFlowReturn
gst_rtp_vorbis_pay_flush_packet (GstRtpVorbisPay * rtpvorbispay)
{
  GstFlowReturn ret;
  guint8 *payload;
  guint hlen;
  GstRTPBuffer rtp = { NULL };
  GList *l;

  /* check for empty packet */
  if (!rtpvorbispay->packet || rtpvorbispay->payload_pos <= 4)
    return GST_FLOW_OK;

  GST_LOG_OBJECT (rtpvorbispay, "flushing packet");

  gst_rtp_buffer_map (rtpvorbispay->packet, GST_MAP_WRITE, &rtp);

  /* fix header */
  payload = gst_rtp_buffer_get_payload (&rtp);
  /*
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                     Ident                     | F |VDT|# pkts.|
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * F: Fragment type (0=none, 1=start, 2=cont, 3=end)
   * VDT: Vorbis data type (0=vorbis, 1=config, 2=comment, 3=reserved)
   * pkts: number of packets.
   */
  payload[0] = (rtpvorbispay->payload_ident >> 16) & 0xff;
  payload[1] = (rtpvorbispay->payload_ident >> 8) & 0xff;
  payload[2] = (rtpvorbispay->payload_ident) & 0xff;
  payload[3] = (rtpvorbispay->payload_F & 0x3) << 6 |
      (rtpvorbispay->payload_VDT & 0x3) << 4 |
      (rtpvorbispay->payload_pkts & 0xf);

  gst_rtp_buffer_unmap (&rtp);

  /* shrink the buffer size to the last written byte */
  hlen = gst_rtp_buffer_calc_header_len (0);
  gst_buffer_resize (rtpvorbispay->packet, 0, hlen + rtpvorbispay->payload_pos);

  GST_BUFFER_DURATION (rtpvorbispay->packet) = rtpvorbispay->payload_duration;

  for (l = g_list_last (rtpvorbispay->packet_buffers); l; l = l->prev) {
    GstBuffer *buf = GST_BUFFER_CAST (l->data);
    gst_rtp_copy_audio_meta (rtpvorbispay, rtpvorbispay->packet, buf);
    gst_buffer_unref (buf);
  }
  g_list_free (rtpvorbispay->packet_buffers);
  rtpvorbispay->packet_buffers = NULL;

  /* push, this gives away our ref to the packet, so clear it. */
  ret =
      gst_rtp_base_payload_push (GST_RTP_BASE_PAYLOAD (rtpvorbispay),
      rtpvorbispay->packet);
  rtpvorbispay->packet = NULL;

  return ret;
}

static gboolean
gst_rtp_vorbis_pay_finish_headers (GstRTPBasePayload * basepayload)
{
  GstRtpVorbisPay *rtpvorbispay = GST_RTP_VORBIS_PAY (basepayload);
  GList *walk;
  guint length, size, n_headers, configlen, extralen;
  gchar *cstr, *configuration;
  guint8 *data, *config;
  guint32 ident;
  gboolean res;

  GST_DEBUG_OBJECT (rtpvorbispay, "finish headers");

  if (!rtpvorbispay->headers)
    goto no_headers;

  /* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                     Number of packed headers                  |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Packed header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Packed header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          ....                                 |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * We only construct a config containing 1 packed header like this:
   *
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                   Ident                       | length       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              | n. of headers |    length1    |    length2   ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |             Identification Header            ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |         Comment Header                       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                        Comment Header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Setup Header                        ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                         Setup Header                         |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */

  /* we need 4 bytes for the number of headers (which is always 1), 3 bytes for
   * the ident, 2 bytes for length, 1 byte for n. of headers. */
  size = 4 + 3 + 2 + 1;

  /* count the size of the headers first and update the hash */
  length = 0;
  n_headers = 0;
  ident = fnv1_hash_32_new ();
  extralen = 1;
  for (walk = rtpvorbispay->headers; walk; walk = g_list_next (walk)) {
    GstBuffer *buf = GST_BUFFER_CAST (walk->data);
    GstMapInfo map;
    guint bsize;

    bsize = gst_buffer_get_size (buf);
    length += bsize;
    n_headers++;

    /* count number of bytes needed for length fields, we don't need this for
     * the last header. */
    if (g_list_next (walk)) {
      do {
        size++;
        extralen++;
        bsize >>= 7;
      } while (bsize);
    }
    /* update hash */
    gst_buffer_map (buf, &map, GST_MAP_READ);
    ident = fnv1_hash_32_update (ident, map.data, map.size);
    gst_buffer_unmap (buf, &map);
  }

  /* packet length is header size + packet length */
  configlen = size + length;
  config = data = g_malloc (configlen);

  /* number of packed headers, we only pack 1 header */
  data[0] = 0;
  data[1] = 0;
  data[2] = 0;
  data[3] = 1;

  ident = fnv1_hash_32_to_24 (ident);
  rtpvorbispay->payload_ident = ident;
  GST_DEBUG_OBJECT (rtpvorbispay, "ident 0x%08x", ident);

  /* take lower 3 bytes */
  data[4] = (ident >> 16) & 0xff;
  data[5] = (ident >> 8) & 0xff;
  data[6] = ident & 0xff;

  /* store length of all vorbis headers */
  data[7] = ((length) >> 8) & 0xff;
  data[8] = (length) & 0xff;

  /* store number of headers minus one. */
  data[9] = n_headers - 1;
  data += 10;

  /* store length for each header */
  for (walk = rtpvorbispay->headers; walk; walk = g_list_next (walk)) {
    GstBuffer *buf = GST_BUFFER_CAST (walk->data);
    guint bsize, size, temp;
    guint flag;

    /* only need to store the length when it's not the last header */
    if (!g_list_next (walk))
      break;

    bsize = gst_buffer_get_size (buf);

    /* calc size */
    size = 0;
    do {
      size++;
      bsize >>= 7;
    } while (bsize);
    temp = size;

    bsize = gst_buffer_get_size (buf);
    /* write the size backwards */
    flag = 0;
    while (size) {
      size--;
      data[size] = (bsize & 0x7f) | flag;
      bsize >>= 7;
      flag = 0x80;              /* Flag bit on all bytes of the length except the last */
    }
    data += temp;
  }

  /* copy header data */
  for (walk = rtpvorbispay->headers; walk; walk = g_list_next (walk)) {
    GstBuffer *buf = GST_BUFFER_CAST (walk->data);

    gst_buffer_extract (buf, 0, data, gst_buffer_get_size (buf));
    data += gst_buffer_get_size (buf);
  }
  rtpvorbispay->need_headers = FALSE;

  /* serialize to base64 */
  configuration = g_base64_encode (config, configlen);

  /* store for later re-sending */
  g_free (rtpvorbispay->config_data);
  rtpvorbispay->config_size = configlen - 4 - 3 - 2;
  rtpvorbispay->config_data = g_malloc (rtpvorbispay->config_size);
  rtpvorbispay->config_extra_len = extralen;
  memcpy (rtpvorbispay->config_data, config + 4 + 3 + 2,
      rtpvorbispay->config_size);

  g_free (config);

  /* configure payloader settings */
  cstr = g_strdup_printf ("%d", rtpvorbispay->channels);
  gst_rtp_base_payload_set_options (basepayload, "audio", TRUE, "VORBIS",
      rtpvorbispay->rate);
  res =
      gst_rtp_base_payload_set_outcaps (basepayload, "encoding-params",
      G_TYPE_STRING, cstr, "configuration", G_TYPE_STRING, configuration, NULL);
  g_free (cstr);
  g_free (configuration);

  return res;

  /* ERRORS */
no_headers:
  {
    GST_DEBUG_OBJECT (rtpvorbispay, "finish headers");
    return FALSE;
  }
}

static gboolean
gst_rtp_vorbis_pay_parse_id (GstRTPBasePayload * basepayload, guint8 * data,
    guint size)
{
  GstRtpVorbisPay *rtpvorbispay = GST_RTP_VORBIS_PAY (basepayload);
  guint8 channels;
  gint32 rate, version;

  if (G_UNLIKELY (size < 16))
    goto too_short;

  if (G_UNLIKELY (memcmp (data, "\001vorbis", 7)))
    goto invalid_start;
  data += 7;

  if (G_UNLIKELY ((version = GST_READ_UINT32_LE (data)) != 0))
    goto invalid_version;
  data += 4;

  if (G_UNLIKELY ((channels = *data++) < 1))
    goto invalid_channels;

  if (G_UNLIKELY ((rate = GST_READ_UINT32_LE (data)) < 1))
    goto invalid_rate;

  /* all fine, store the values */
  rtpvorbispay->channels = channels;
  rtpvorbispay->rate = rate;

  return TRUE;

  /* ERRORS */
too_short:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, DECODE,
        ("Identification packet is too short, need at least 16, got %d", size),
        (NULL));
    return FALSE;
  }
invalid_start:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, DECODE,
        ("Invalid header start in identification packet"), (NULL));
    return FALSE;
  }
invalid_version:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, DECODE,
        ("Invalid version, expected 0, got %d", version), (NULL));
    return FALSE;
  }
invalid_rate:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, DECODE,
        ("Invalid rate %d", rate), (NULL));
    return FALSE;
  }
invalid_channels:
  {
    GST_ELEMENT_ERROR (basepayload, STREAM, DECODE,
        ("Invalid channels %d", channels), (NULL));
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_vorbis_pay_payload_buffer (GstRtpVorbisPay * rtpvorbispay, guint8 VDT,
    GstBuffer * buffer, guint8 * data, guint size, GstClockTime timestamp,
    GstClockTime duration, guint not_in_length)
{
  GstFlowReturn ret = GST_FLOW_OK;
  guint newsize;
  guint packet_len;
  GstClockTime newduration;
  gboolean flush;
  guint plen;
  guint8 *ppos, *payload;
  gboolean fragmented;
  GstRTPBuffer rtp = { NULL };

  /* size increases with packet length and 2 bytes size eader. */
  newduration = rtpvorbispay->payload_duration;
  if (duration != GST_CLOCK_TIME_NONE)
    newduration += duration;

  newsize = rtpvorbispay->payload_pos + 2 + size;
  packet_len = gst_rtp_buffer_calc_packet_len (newsize, 0, 0);

  /* check buffer filled against length and max latency */
  flush = gst_rtp_base_payload_is_filled (GST_RTP_BASE_PAYLOAD (rtpvorbispay),
      packet_len, newduration);
  /* we can store up to 15 vorbis packets in one RTP packet. */
  flush |= (rtpvorbispay->payload_pkts == 15);
  /* flush if we have a new VDT */
  if (rtpvorbispay->packet)
    flush |= (rtpvorbispay->payload_VDT != VDT);
  if (flush)
    ret = gst_rtp_vorbis_pay_flush_packet (rtpvorbispay);

  if (ret != GST_FLOW_OK)
    goto done;

  /* create new packet if we must */
  if (!rtpvorbispay->packet) {
    gst_rtp_vorbis_pay_init_packet (rtpvorbispay, VDT, timestamp);
  }

  gst_rtp_buffer_map (rtpvorbispay->packet, GST_MAP_WRITE, &rtp);
  payload = gst_rtp_buffer_get_payload (&rtp);
  ppos = payload + rtpvorbispay->payload_pos;
  fragmented = FALSE;

  /* put buffer in packet, it either fits completely or needs to be fragmented
   * over multiple RTP packets. */
  do {
    plen = MIN (rtpvorbispay->payload_left - 2, size);

    GST_LOG_OBJECT (rtpvorbispay, "append %u bytes", plen);

    /* data is copied in the payload with a 2 byte length header */
    ppos[0] = ((plen - not_in_length) >> 8) & 0xff;
    ppos[1] = ((plen - not_in_length) & 0xff);
    if (plen)
      memcpy (&ppos[2], data, plen);

    if (buffer) {
      if (!rtpvorbispay->packet_buffers
          || rtpvorbispay->packet_buffers->data != (gpointer) buffer)
        rtpvorbispay->packet_buffers =
            g_list_prepend (rtpvorbispay->packet_buffers,
            gst_buffer_ref (buffer));
    } else {
      GList *l;

      for (l = rtpvorbispay->headers; l; l = l->next)
        rtpvorbispay->packet_buffers =
            g_list_prepend (rtpvorbispay->packet_buffers,
            gst_buffer_ref (l->data));
    }

    /* only first (only) configuration cuts length field */
    /* NOTE: spec (if any) is not clear on this ... */
    not_in_length = 0;

    size -= plen;
    data += plen;

    rtpvorbispay->payload_pos += plen + 2;
    rtpvorbispay->payload_left -= plen + 2;

    if (fragmented) {
      if (size == 0)
        /* last fragment, set F to 0x3. */
        rtpvorbispay->payload_F = 0x3;
      else
        /* fragment continues, set F to 0x2. */
        rtpvorbispay->payload_F = 0x2;
    } else {
      if (size > 0) {
        /* fragmented packet starts, set F to 0x1, mark ourselves as
         * fragmented. */
        rtpvorbispay->payload_F = 0x1;
        fragmented = TRUE;
      }
    }
    if (fragmented) {
      gst_rtp_buffer_unmap (&rtp);
      /* fragmented packets are always flushed and have ptks of 0 */
      rtpvorbispay->payload_pkts = 0;
      ret = gst_rtp_vorbis_pay_flush_packet (rtpvorbispay);

      if (size > 0) {
        /* start new packet and get pointers. VDT stays the same. */
        gst_rtp_vorbis_pay_init_packet (rtpvorbispay,
            rtpvorbispay->payload_VDT, timestamp);
        gst_rtp_buffer_map (rtpvorbispay->packet, GST_MAP_WRITE, &rtp);
        payload = gst_rtp_buffer_get_payload (&rtp);
        ppos = payload + rtpvorbispay->payload_pos;
      }
    } else {
      /* unfragmented packet, update stats for next packet, size == 0 and we
       * exit the while loop */
      rtpvorbispay->payload_pkts++;
      if (duration != GST_CLOCK_TIME_NONE)
        rtpvorbispay->payload_duration += duration;
    }
  } while (size && ret == GST_FLOW_OK);

  if (rtp.buffer)
    gst_rtp_buffer_unmap (&rtp);

done:

  return ret;
}

static GstFlowReturn
gst_rtp_vorbis_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstRtpVorbisPay *rtpvorbispay;
  GstFlowReturn ret;
  GstMapInfo map;
  gsize size;
  guint8 *data;
  GstClockTime duration, timestamp;
  guint8 VDT;

  rtpvorbispay = GST_RTP_VORBIS_PAY (basepayload);

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  data = map.data;
  size = map.size;
  duration = GST_BUFFER_DURATION (buffer);
  timestamp = GST_BUFFER_PTS (buffer);

  GST_LOG_OBJECT (rtpvorbispay, "size %" G_GSIZE_FORMAT
      ", duration %" GST_TIME_FORMAT, size, GST_TIME_ARGS (duration));

  if (G_UNLIKELY (size < 1))
    goto wrong_size;

  /* find packet type */
  if (data[0] & 1) {
    /* header */
    if (data[0] == 1) {
      /* identification, we need to parse this in order to get the clock rate. */
      if (G_UNLIKELY (!gst_rtp_vorbis_pay_parse_id (basepayload, data, size)))
        goto parse_id_failed;
      VDT = 1;
    } else if (data[0] == 3) {
      /* comment */
      VDT = 2;
    } else if (data[0] == 5) {
      /* setup */
      VDT = 1;
    } else
      goto unknown_header;
  } else
    /* data */
    VDT = 0;

  /* we need to collect the headers and construct a config string from them */
  if (VDT != 0) {
    rtpvorbispay->need_headers = TRUE;
    if (!rtpvorbispay->need_headers && VDT == 1) {
      GST_INFO_OBJECT (rtpvorbispay, "getting new headers, replace existing");
      g_list_free_full (rtpvorbispay->headers,
          (GDestroyNotify) gst_buffer_unref);
      rtpvorbispay->headers = NULL;
    }
    GST_DEBUG_OBJECT (rtpvorbispay, "collecting header");
    /* append header to the list of headers, or replace
     * if the same type of header was already in there.
     *
     * This prevents storing an infinite amount of e.g. comment headers, there
     * must only be one */
    gst_buffer_unmap (buffer, &map);

    if (rtpvorbispay->headers) {
      gboolean found = FALSE;
      GList *l;
      guint8 new_header_type;

      gst_buffer_extract (buffer, 0, &new_header_type, 1);

      for (l = rtpvorbispay->headers; l; l = l->next) {
        GstBuffer *header = l->data;
        guint8 header_type;

        if (gst_buffer_extract (header, 0, &header_type, 1)
            && header_type == new_header_type) {
          found = TRUE;
          gst_buffer_unref (header);
          l->data = buffer;
          break;
        }
      }
      if (!found)
        rtpvorbispay->headers = g_list_append (rtpvorbispay->headers, buffer);
    } else {
      rtpvorbispay->headers = g_list_append (rtpvorbispay->headers, buffer);
    }

    ret = GST_FLOW_OK;
    goto done;
  } else if (rtpvorbispay->headers && rtpvorbispay->need_headers) {
    if (!gst_rtp_vorbis_pay_finish_headers (basepayload))
      goto header_error;
  }

  /* there is a config request, see if we need to insert it */
  if (rtpvorbispay->config_interval > 0 && rtpvorbispay->config_data) {
    gboolean send_config = FALSE;
    GstClockTime running_time =
        gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
        timestamp);

    if (rtpvorbispay->last_config != -1) {
      guint64 diff;

      GST_LOG_OBJECT (rtpvorbispay,
          "now %" GST_TIME_FORMAT ", last config %" GST_TIME_FORMAT,
          GST_TIME_ARGS (running_time),
          GST_TIME_ARGS (rtpvorbispay->last_config));

      /* calculate diff between last config in milliseconds */
      if (running_time > rtpvorbispay->last_config) {
        diff = running_time - rtpvorbispay->last_config;
      } else {
        diff = 0;
      }

      GST_DEBUG_OBJECT (rtpvorbispay,
          "interval since last config %" GST_TIME_FORMAT, GST_TIME_ARGS (diff));

      /* bigger than interval, queue config */
      if (GST_TIME_AS_SECONDS (diff) >= rtpvorbispay->config_interval) {
        GST_DEBUG_OBJECT (rtpvorbispay, "time to send config");
        send_config = TRUE;
      }
    } else {
      /* no known previous config time, send now */
      GST_DEBUG_OBJECT (rtpvorbispay, "no previous config time, send now");
      send_config = TRUE;
    }

    if (send_config) {
      /* we need to send config now first */
      /* different TDT type forces flush */
      gst_rtp_vorbis_pay_payload_buffer (rtpvorbispay, 1,
          NULL, rtpvorbispay->config_data, rtpvorbispay->config_size,
          timestamp, GST_CLOCK_TIME_NONE, rtpvorbispay->config_extra_len);

      if (running_time != -1) {
        rtpvorbispay->last_config = running_time;
      }
    }
  }

  ret =
      gst_rtp_vorbis_pay_payload_buffer (rtpvorbispay, VDT, buffer, data, size,
      timestamp, duration, 0);

  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);

done:
  return ret;

  /* ERRORS */
wrong_size:
  {
    GST_ELEMENT_WARNING (rtpvorbispay, STREAM, DECODE,
        ("Invalid packet size (1 < %" G_GSIZE_FORMAT ")", size), (NULL));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }
parse_id_failed:
  {
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }
unknown_header:
  {
    GST_ELEMENT_WARNING (rtpvorbispay, STREAM, DECODE,
        (NULL), ("Ignoring unknown header received"));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }
header_error:
  {
    GST_ELEMENT_WARNING (rtpvorbispay, STREAM, DECODE,
        (NULL), ("Error initializing header config"));
    gst_buffer_unmap (buffer, &map);
    gst_buffer_unref (buffer);
    return GST_FLOW_OK;
  }
}

static gboolean
gst_rtp_vorbis_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  GstRtpVorbisPay *rtpvorbispay = GST_RTP_VORBIS_PAY (payload);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_vorbis_pay_clear_packet (rtpvorbispay);
      break;
    default:
      break;
  }
  /* false to let parent handle event as well */
  return GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload, event);
}

static GstStateChangeReturn
gst_rtp_vorbis_pay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpVorbisPay *rtpvorbispay;
  GstStateChangeReturn ret;

  rtpvorbispay = GST_RTP_VORBIS_PAY (element);

  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_vorbis_pay_cleanup (rtpvorbispay);
      break;
    default:
      break;
  }
  return ret;
}

static void
gst_rtp_vorbis_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpVorbisPay *rtpvorbispay;

  rtpvorbispay = GST_RTP_VORBIS_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      rtpvorbispay->config_interval = g_value_get_uint (value);
      break;
    default:
      break;
  }
}

static void
gst_rtp_vorbis_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpVorbisPay *rtpvorbispay;

  rtpvorbispay = GST_RTP_VORBIS_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      g_value_set_uint (value, rtpvorbispay->config_interval);
      break;
    default:
      break;
  }
}

gboolean
gst_rtp_vorbis_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvorbispay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_VORBIS_PAY);
}
