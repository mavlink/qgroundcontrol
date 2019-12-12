/* GStreamer
 * Copyright (C) <2010> Wim Taymans <wim.taymans@gmail.com>
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

#include "gstrtpgstpay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_pay_debug);
#define GST_CAT_DEFAULT gst_rtp_pay_debug

/*
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |C| CV  |D|0|0|0|     ETYPE     |  MBZ                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                          Frag_offset                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * C: caps inlined flag
 *   When C set, first part of payload contains caps definition. Caps definition
 *   starts with variable-length length prefix and then a string of that length.
 *   the length is encoded in big endian 7 bit chunks, the top 1 bit of a byte
 *   is the continuation marker and the 7 next bits the data. A continuation
 *   marker of 1 means that the next byte contains more data.
 *
 * CV: caps version, 0 = caps from SDP, 1 - 7 inlined caps
 * D: delta unit buffer
 * ETYPE: type of event. Payload contains the event, prefixed with a
 *        variable length field.
 *   0 = NO event
 *   1 = GST_EVENT_TAG
 *   2 = GST_EVENT_CUSTOM_DOWNSTREAM
 *   3 = GST_EVENT_CUSTOM_BOTH
 *   4 = GST_EVENT_STREAM_START
 */

static GstStaticPadTemplate gst_rtp_gst_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_rtp_gst_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"application\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"X-GST\"")
    );

enum
{
  PROP_0,
  PROP_CONFIG_INTERVAL
};

#define DEFAULT_CONFIG_INTERVAL		      0

static void gst_rtp_gst_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rtp_gst_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_gst_pay_finalize (GObject * obj);
static GstStateChangeReturn gst_rtp_gst_pay_change_state (GstElement * element,
    GstStateChange transition);

static gboolean gst_rtp_gst_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_gst_pay_handle_buffer (GstRTPBasePayload * payload,
    GstBuffer * buffer);
static gboolean gst_rtp_gst_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);
static gboolean gst_rtp_gst_pay_src_event (GstRTPBasePayload * payload,
    GstEvent * event);

#define gst_rtp_gst_pay_parent_class parent_class
G_DEFINE_TYPE (GstRtpGSTPay, gst_rtp_gst_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_gst_pay_class_init (GstRtpGSTPayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBasePayloadClass *gstrtpbasepayload_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->set_property = gst_rtp_gst_pay_set_property;
  gobject_class->get_property = gst_rtp_gst_pay_get_property;
  gobject_class->finalize = gst_rtp_gst_pay_finalize;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_CONFIG_INTERVAL,
      g_param_spec_uint ("config-interval",
          "Caps/Tags Send Interval",
          "Interval for sending caps and TAG events in seconds (0 = disabled)",
          0, 3600, DEFAULT_CONFIG_INTERVAL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  gstelement_class->change_state = gst_rtp_gst_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gst_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gst_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP GStreamer payloader", "Codec/Payloader/Network/RTP",
      "Payload GStreamer buffers as RTP packets",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasepayload_class->set_caps = gst_rtp_gst_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_gst_pay_handle_buffer;
  gstrtpbasepayload_class->sink_event = gst_rtp_gst_pay_sink_event;
  gstrtpbasepayload_class->src_event = gst_rtp_gst_pay_src_event;

  GST_DEBUG_CATEGORY_INIT (gst_rtp_pay_debug, "rtpgstpay", 0,
      "rtpgstpay element");
}

static void
gst_rtp_gst_pay_init (GstRtpGSTPay * rtpgstpay)
{
  rtpgstpay->adapter = gst_adapter_new ();
  rtpgstpay->pending_buffers = NULL;
  gst_rtp_base_payload_set_options (GST_RTP_BASE_PAYLOAD (rtpgstpay),
      "application", TRUE, "X-GST", 90000);
  rtpgstpay->last_config = GST_CLOCK_TIME_NONE;
  rtpgstpay->taglist = NULL;
  rtpgstpay->config_interval = DEFAULT_CONFIG_INTERVAL;
}

static void
gst_rtp_gst_pay_reset (GstRtpGSTPay * rtpgstpay, gboolean full)
{
  rtpgstpay->last_config = GST_CLOCK_TIME_NONE;
  gst_adapter_clear (rtpgstpay->adapter);
  rtpgstpay->flags &= 0x70;
  rtpgstpay->etype = 0;
  if (rtpgstpay->pending_buffers)
    g_list_free_full (rtpgstpay->pending_buffers,
        (GDestroyNotify) gst_buffer_list_unref);
  rtpgstpay->pending_buffers = NULL;
  if (full) {
    if (rtpgstpay->taglist)
      gst_tag_list_unref (rtpgstpay->taglist);
    rtpgstpay->taglist = NULL;
    g_free (rtpgstpay->stream_id);
    rtpgstpay->stream_id = NULL;
    rtpgstpay->current_CV = 0;
    rtpgstpay->next_CV = 0;
  }
}

static void
gst_rtp_gst_pay_finalize (GObject * obj)
{
  GstRtpGSTPay *rtpgstpay;

  rtpgstpay = GST_RTP_GST_PAY (obj);

  gst_rtp_gst_pay_reset (rtpgstpay, TRUE);

  g_object_unref (rtpgstpay->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_rtp_gst_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpGSTPay *rtpgstpay;

  rtpgstpay = GST_RTP_GST_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      rtpgstpay->config_interval = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_gst_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpGSTPay *rtpgstpay;

  rtpgstpay = GST_RTP_GST_PAY (object);

  switch (prop_id) {
    case PROP_CONFIG_INTERVAL:
      g_value_set_uint (value, rtpgstpay->config_interval);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_rtp_gst_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpGSTPay *rtpgstpay;
  GstStateChangeReturn ret;

  rtpgstpay = GST_RTP_GST_PAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_gst_pay_reset (rtpgstpay, TRUE);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_gst_pay_reset (rtpgstpay, TRUE);
      break;
    default:
      break;
  }
  return ret;
}

#define RTP_HEADER_LEN 12

static gboolean
gst_rtp_gst_pay_create_from_adapter (GstRtpGSTPay * rtpgstpay,
    GstClockTime timestamp)
{
  guint avail, mtu;
  guint frag_offset;
  GstBufferList *list;

  avail = gst_adapter_available (rtpgstpay->adapter);
  if (avail == 0)
    return FALSE;

  mtu = GST_RTP_BASE_PAYLOAD_MTU (rtpgstpay);

  list = gst_buffer_list_new_sized ((avail / (mtu - (RTP_HEADER_LEN + 8))) + 1);
  frag_offset = 0;

  while (avail) {
    guint towrite;
    guint8 *payload;
    guint payload_len;
    guint packet_len;
    GstBuffer *outbuf;
    GstRTPBuffer rtp = { NULL };
    GstBuffer *paybuf;


    /* this will be the total length of the packet */
    packet_len = gst_rtp_buffer_calc_packet_len (8 + avail, 0, 0);

    /* fill one MTU or all available bytes */
    towrite = MIN (packet_len, mtu);

    /* this is the payload length */
    payload_len = gst_rtp_buffer_calc_payload_len (towrite, 0, 0);

    /* create buffer to hold the header */
    outbuf = gst_rtp_buffer_new_allocate (8, 0, 0);

    gst_rtp_buffer_map (outbuf, GST_MAP_WRITE, &rtp);
    payload = gst_rtp_buffer_get_payload (&rtp);

    GST_DEBUG_OBJECT (rtpgstpay, "new packet len %u, frag %u", packet_len,
        frag_offset);

    /*
     *  0                   1                   2                   3
     *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |C| CV  |D|0|0|0|     ETYPE     |  MBZ                          |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                          Frag_offset                          |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    payload[0] = rtpgstpay->flags;
    payload[1] = rtpgstpay->etype;
    payload[2] = payload[3] = 0;
    payload[4] = frag_offset >> 24;
    payload[5] = frag_offset >> 16;
    payload[6] = frag_offset >> 8;
    payload[7] = frag_offset & 0xff;

    payload += 8;
    payload_len -= 8;

    frag_offset += payload_len;
    avail -= payload_len;

    if (avail == 0)
      gst_rtp_buffer_set_marker (&rtp, TRUE);

    gst_rtp_buffer_unmap (&rtp);

    /* create a new buf to hold the payload */
    GST_DEBUG_OBJECT (rtpgstpay, "take %u bytes from adapter", payload_len);
    paybuf = gst_adapter_take_buffer_fast (rtpgstpay->adapter, payload_len);

    if (GST_BUFFER_FLAG_IS_SET (paybuf, GST_BUFFER_FLAG_DELTA_UNIT))
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);

    /* create a new group to hold the rtp header and the payload */
    gst_rtp_copy_meta (GST_ELEMENT_CAST (rtpgstpay), outbuf, paybuf, 0);
    outbuf = gst_buffer_append (outbuf, paybuf);

    GST_BUFFER_PTS (outbuf) = timestamp;

    /* and add to list */
    gst_buffer_list_insert (list, -1, outbuf);
  }

  rtpgstpay->flags &= 0x70;
  rtpgstpay->etype = 0;
  rtpgstpay->pending_buffers = g_list_append (rtpgstpay->pending_buffers, list);

  return TRUE;
}

static GstFlowReturn
gst_rtp_gst_pay_flush (GstRtpGSTPay * rtpgstpay, GstClockTime timestamp)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GList *iter;

  gst_rtp_gst_pay_create_from_adapter (rtpgstpay, timestamp);

  iter = rtpgstpay->pending_buffers;
  while (iter) {
    GstBufferList *list = iter->data;

    rtpgstpay->pending_buffers = iter =
        g_list_delete_link (rtpgstpay->pending_buffers, iter);

    /* push the whole buffer list at once */
    ret = gst_rtp_base_payload_push_list (GST_RTP_BASE_PAYLOAD (rtpgstpay),
        list);
    if (ret != GST_FLOW_OK)
      break;
  }

  return ret;
}

static GstBuffer *
make_data_buffer (GstRtpGSTPay * rtpgstpay, gchar * data, guint size)
{
  guint plen;
  guint8 *ptr;
  GstBuffer *outbuf;
  GstMapInfo map;

  /* calculate length */
  plen = 1;
  while (size >> (7 * plen))
    plen++;

  outbuf = gst_buffer_new_allocate (NULL, plen + size, NULL);

  gst_buffer_map (outbuf, &map, GST_MAP_WRITE);
  ptr = map.data;

  /* write length */
  while (plen) {
    plen--;
    *ptr++ = ((plen > 0) ? 0x80 : 0) | ((size >> (7 * plen)) & 0x7f);
  }
  /* copy data */
  memcpy (ptr, data, size);
  gst_buffer_unmap (outbuf, &map);

  return outbuf;
}

static void
gst_rtp_gst_pay_send_caps (GstRtpGSTPay * rtpgstpay, guint8 cv, GstCaps * caps)
{
  gchar *capsstr;
  guint capslen;
  GstBuffer *outbuf;

  if (rtpgstpay->flags == ((1 << 7) | (cv << 4))) {
    /* If caps for the current CV are pending in the adapter already, do
     * nothing at all here
     */
    return;
  } else if (rtpgstpay->flags & (1 << 7)) {
    /* Create a new standalone caps packet if caps were already pending.
     * The next caps are going to be merged with the following buffer or
     * sent standalone if another event is sent first */
    gst_rtp_gst_pay_create_from_adapter (rtpgstpay, GST_CLOCK_TIME_NONE);
  }

  capsstr = gst_caps_to_string (caps);
  capslen = strlen (capsstr);
  /* for 0 byte */
  capslen++;

  GST_DEBUG_OBJECT (rtpgstpay, "sending caps=%s", capsstr);

  /* make a data buffer of it */
  outbuf = make_data_buffer (rtpgstpay, capsstr, capslen);
  g_free (capsstr);

  /* store in adapter, we don't flush yet, buffer might follow */
  rtpgstpay->flags = (1 << 7) | (cv << 4);
  gst_adapter_push (rtpgstpay->adapter, outbuf);
}

static gboolean
gst_rtp_gst_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstRtpGSTPay *rtpgstpay;
  gboolean res;
  gchar *capsstr, *capsenc, *capsver;
  guint capslen;

  rtpgstpay = GST_RTP_GST_PAY (payload);

  capsstr = gst_caps_to_string (caps);
  capslen = strlen (capsstr);

  /* encode without 0 byte */
  capsenc = g_base64_encode ((guchar *) capsstr, capslen);
  GST_DEBUG_OBJECT (payload, "caps=%s, caps(base64)=%s", capsstr, capsenc);
  g_free (capsstr);

  /* Send the new caps */
  rtpgstpay->current_CV = rtpgstpay->next_CV;
  rtpgstpay->next_CV = (rtpgstpay->next_CV + 1) & 0x7;
  gst_rtp_gst_pay_send_caps (rtpgstpay, rtpgstpay->current_CV, caps);

  /* make caps for SDP */
  capsver = g_strdup_printf ("%d", rtpgstpay->current_CV);
  res =
      gst_rtp_base_payload_set_outcaps (payload, "caps", G_TYPE_STRING, capsenc,
      "capsversion", G_TYPE_STRING, capsver, NULL);
  g_free (capsenc);
  g_free (capsver);

  return res;
}

static void
gst_rtp_gst_pay_send_event (GstRtpGSTPay * rtpgstpay, guint etype,
    GstEvent * event)
{
  const GstStructure *s;
  gchar *estr;
  guint elen;
  GstBuffer *outbuf;

  /* Create the standalone caps packet if an inlined caps was pending */
  gst_rtp_gst_pay_create_from_adapter (rtpgstpay, GST_CLOCK_TIME_NONE);

  s = gst_event_get_structure (event);

  estr = gst_structure_to_string (s);
  elen = strlen (estr);
  /* for 0 byte */
  elen++;
  outbuf = make_data_buffer (rtpgstpay, estr, elen);
  GST_DEBUG_OBJECT (rtpgstpay, "sending event=%s", estr);
  g_free (estr);

  rtpgstpay->etype = etype;
  gst_adapter_push (rtpgstpay->adapter, outbuf);
  /* Create the event packet now to avoid conflict with data/caps packets */
  gst_rtp_gst_pay_create_from_adapter (rtpgstpay, GST_CLOCK_TIME_NONE);
}

static gboolean
gst_rtp_gst_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  gboolean ret;
  GstRtpGSTPay *rtpgstpay;
  guint etype = 0;

  rtpgstpay = GST_RTP_GST_PAY (payload);

  if (gst_video_event_is_force_key_unit (event)) {
    g_atomic_int_set (&rtpgstpay->force_config, TRUE);
  }

  ret =
      GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->sink_event (payload,
      gst_event_ref (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_gst_pay_reset (rtpgstpay, FALSE);
      break;
    case GST_EVENT_TAG:{
      GstTagList *tags;

      gst_event_parse_tag (event, &tags);

      if (gst_tag_list_get_scope (tags) == GST_TAG_SCOPE_STREAM) {
        GstTagList *old;

        GST_DEBUG_OBJECT (rtpgstpay, "storing stream tags %" GST_PTR_FORMAT,
            tags);
        if ((old = rtpgstpay->taglist))
          gst_tag_list_unref (old);
        rtpgstpay->taglist = gst_tag_list_ref (tags);
      }
      etype = 1;
      break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM:
      etype = 2;
      break;
    case GST_EVENT_CUSTOM_BOTH:
      etype = 3;
      break;
    case GST_EVENT_STREAM_START:{
      const gchar *stream_id = NULL;

      if (rtpgstpay->taglist)
        gst_tag_list_unref (rtpgstpay->taglist);
      rtpgstpay->taglist = NULL;

      gst_event_parse_stream_start (event, &stream_id);
      if (stream_id) {
        g_free (rtpgstpay->stream_id);
        rtpgstpay->stream_id = g_strdup (stream_id);
      }
      etype = 4;
      break;
    }
    default:
      GST_LOG_OBJECT (rtpgstpay, "no event for %s",
          GST_EVENT_TYPE_NAME (event));
      break;
  }
  if (etype) {
    GST_DEBUG_OBJECT (rtpgstpay, "make event type %d for %s",
        etype, GST_EVENT_TYPE_NAME (event));
    gst_rtp_gst_pay_send_event (rtpgstpay, etype, event);
    /* Do not send stream-start right away since caps/new-segment were not yet
       sent, so our data would be considered invalid */
    if (etype != 4) {
      /* flush the adapter immediately */
      gst_rtp_gst_pay_flush (rtpgstpay, GST_CLOCK_TIME_NONE);
    }
  }

  gst_event_unref (event);

  return ret;
}

static gboolean
gst_rtp_gst_pay_src_event (GstRTPBasePayload * payload, GstEvent * event)
{
  GstRtpGSTPay *rtpgstpay;

  rtpgstpay = GST_RTP_GST_PAY (payload);

  if (gst_video_event_is_force_key_unit (event)) {
    g_atomic_int_set (&rtpgstpay->force_config, TRUE);
  }

  return GST_RTP_BASE_PAYLOAD_CLASS (parent_class)->src_event (payload, event);
}

static void
gst_rtp_gst_pay_send_config (GstRtpGSTPay * rtpgstpay,
    GstClockTime running_time)
{
  GstPad *pad = GST_RTP_BASE_PAYLOAD_SINKPAD (rtpgstpay);
  GstCaps *caps = NULL;
  GstEvent *tag = NULL;
  GstEvent *stream_start = NULL;

  GST_DEBUG_OBJECT (rtpgstpay, "time to send config");
  /* Send tags */
  if (rtpgstpay->taglist && !gst_tag_list_is_empty (rtpgstpay->taglist))
    tag = gst_event_new_tag (gst_tag_list_ref (rtpgstpay->taglist));
  if (tag) {
    /* Send start-stream to clear tags */
    if (rtpgstpay->stream_id)
      stream_start = gst_event_new_stream_start (rtpgstpay->stream_id);
    if (stream_start) {
      gst_rtp_gst_pay_send_event (rtpgstpay, 4, stream_start);
      gst_event_unref (stream_start);
    }
    gst_rtp_gst_pay_send_event (rtpgstpay, 1, tag);
    gst_event_unref (tag);
  }
  /* send caps */
  caps = gst_pad_get_current_caps (pad);
  if (caps) {
    gst_rtp_gst_pay_send_caps (rtpgstpay, rtpgstpay->current_CV, caps);
    gst_caps_unref (caps);
  }
  rtpgstpay->last_config = running_time;
}

static GstFlowReturn
gst_rtp_gst_pay_handle_buffer (GstRTPBasePayload * basepayload,
    GstBuffer * buffer)
{
  GstFlowReturn ret;
  GstRtpGSTPay *rtpgstpay;
  GstClockTime timestamp, running_time;

  rtpgstpay = GST_RTP_GST_PAY (basepayload);

  timestamp = GST_BUFFER_PTS (buffer);
  running_time =
      gst_segment_to_running_time (&basepayload->segment, GST_FORMAT_TIME,
      timestamp);

  /* check if we need to send the caps and taglist now */
  if (rtpgstpay->config_interval > 0
      || g_atomic_int_compare_and_exchange (&rtpgstpay->force_config, TRUE,
          FALSE)) {
    GST_DEBUG_OBJECT (rtpgstpay,
        "running time %" GST_TIME_FORMAT ", last config %" GST_TIME_FORMAT,
        GST_TIME_ARGS (running_time), GST_TIME_ARGS (rtpgstpay->last_config));

    if (running_time != GST_CLOCK_TIME_NONE &&
        rtpgstpay->last_config != GST_CLOCK_TIME_NONE) {
      guint64 diff;

      /* calculate diff between last SPS/PPS in milliseconds */
      if (running_time > rtpgstpay->last_config)
        diff = running_time - rtpgstpay->last_config;
      else
        diff = 0;

      GST_DEBUG_OBJECT (rtpgstpay,
          "interval since last config %" GST_TIME_FORMAT, GST_TIME_ARGS (diff));

      /* bigger than interval, queue SPS/PPS */
      if (GST_TIME_AS_SECONDS (diff) >= rtpgstpay->config_interval)
        gst_rtp_gst_pay_send_config (rtpgstpay, running_time);
    } else {
      gst_rtp_gst_pay_send_config (rtpgstpay, running_time);
    }
  }

  /* caps always from SDP for now */
  if (GST_BUFFER_FLAG_IS_SET (buffer, GST_BUFFER_FLAG_DELTA_UNIT))
    rtpgstpay->flags |= (1 << 3);

  gst_adapter_push (rtpgstpay->adapter, buffer);
  ret = gst_rtp_gst_pay_flush (rtpgstpay, timestamp);

  return ret;
}

gboolean
gst_rtp_gst_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpgstpay",
      GST_RANK_NONE, GST_TYPE_RTP_GST_PAY);
}
