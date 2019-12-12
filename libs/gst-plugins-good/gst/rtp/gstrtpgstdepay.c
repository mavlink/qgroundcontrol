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
#include <stdlib.h>

#include "gstrtpgstdepay.h"
#include "gstrtputils.h"

#include <gst/video/video.h>

GST_DEBUG_CATEGORY_STATIC (rtpgstdepay_debug);
#define GST_CAT_DEFAULT (rtpgstdepay_debug)

static GstStaticPadTemplate gst_rtp_gst_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate gst_rtp_gst_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"application\", "
        "clock-rate = (int) 90000, " "encoding-name = (string) \"X-GST\"")
    );

#define gst_rtp_gst_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpGSTDepay, gst_rtp_gst_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static void gst_rtp_gst_depay_finalize (GObject * object);

static gboolean gst_rtp_gst_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * event);
static GstStateChangeReturn gst_rtp_gst_depay_change_state (GstElement *
    element, GstStateChange transition);

static void gst_rtp_gst_depay_reset (GstRtpGSTDepay * rtpgstdepay, gboolean
    full);
static gboolean gst_rtp_gst_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_gst_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);

static void
gst_rtp_gst_depay_class_init (GstRtpGSTDepayClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  GST_DEBUG_CATEGORY_INIT (rtpgstdepay_debug, "rtpgstdepay", 0,
      "Gstreamer RTP Depayloader");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gobject_class->finalize = gst_rtp_gst_depay_finalize;

  gstelement_class->change_state = gst_rtp_gst_depay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gst_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_gst_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "GStreamer depayloader", "Codec/Depayloader/Network",
      "Extracts GStreamer buffers from RTP packets",
      "Wim Taymans <wim.taymans@gmail.com>");

  gstrtpbasedepayload_class->handle_event = gst_rtp_gst_depay_handle_event;
  gstrtpbasedepayload_class->set_caps = gst_rtp_gst_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet = gst_rtp_gst_depay_process;
}

static void
gst_rtp_gst_depay_init (GstRtpGSTDepay * rtpgstdepay)
{
  rtpgstdepay->adapter = gst_adapter_new ();
}

static void
gst_rtp_gst_depay_finalize (GObject * object)
{
  GstRtpGSTDepay *rtpgstdepay;

  rtpgstdepay = GST_RTP_GST_DEPAY (object);

  gst_rtp_gst_depay_reset (rtpgstdepay, TRUE);
  g_object_unref (rtpgstdepay->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_gst_depay_reset (GstRtpGSTDepay * rtpgstdepay, gboolean full)
{
  gst_adapter_clear (rtpgstdepay->adapter);
  if (full) {
    rtpgstdepay->current_CV = 0;
    gst_caps_replace (&rtpgstdepay->current_caps, NULL);
    g_free (rtpgstdepay->stream_id);
    rtpgstdepay->stream_id = NULL;
    if (rtpgstdepay->tags)
      gst_tag_list_unref (rtpgstdepay->tags);
    rtpgstdepay->tags = NULL;
  }
}

static gboolean
gst_rtp_gst_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstRtpGSTDepay *rtpgstdepay;
  GstStructure *structure;
  gint clock_rate;
  gboolean res;
  const gchar *capsenc;

  rtpgstdepay = GST_RTP_GST_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;
  depayload->clock_rate = clock_rate;

  capsenc = gst_structure_get_string (structure, "caps");
  if (capsenc) {
    GstCaps *outcaps;
    gsize out_len;
    gchar *capsstr;
    const gchar *capsver;
    guint CV;

    /* decode caps */
    capsstr = (gchar *) g_base64_decode (capsenc, &out_len);
    outcaps = gst_caps_from_string (capsstr);
    g_free (capsstr);

    /* parse version */
    capsver = gst_structure_get_string (structure, "capsversion");
    if (capsver) {
      CV = atoi (capsver);
    } else {
      /* no version, assume 0 */
      CV = 0;
    }
    /* store in cache */
    rtpgstdepay->current_CV = CV;
    gst_caps_replace (&rtpgstdepay->current_caps, outcaps);

    res = gst_pad_set_caps (depayload->srcpad, outcaps);
    gst_caps_unref (outcaps);
  } else {
    GST_WARNING_OBJECT (depayload, "no caps given");
    rtpgstdepay->current_CV = -1;
    gst_caps_replace (&rtpgstdepay->current_caps, NULL);
    res = TRUE;
  }

  return res;
}

static gboolean
read_length (GstRtpGSTDepay * rtpgstdepay, guint8 * data, guint size,
    guint * length, guint * skip)
{
  guint b, len, offset;

  /* start reading the length, we need this to skip to the data later */
  len = offset = 0;
  do {
    if (offset >= size)
      return FALSE;
    b = data[offset++];
    len = (len << 7) | (b & 0x7f);
  } while (b & 0x80);

  /* check remaining buffer size */
  if (size - offset < len)
    return FALSE;

  *length = len;
  *skip = offset;

  return TRUE;
}

static GstCaps *
read_caps (GstRtpGSTDepay * rtpgstdepay, GstBuffer * buf, guint * skip)
{
  guint offset, length;
  GstCaps *caps;
  GstMapInfo map;

  gst_buffer_map (buf, &map, GST_MAP_READ);

  GST_DEBUG_OBJECT (rtpgstdepay, "buffer size %" G_GSIZE_FORMAT, map.size);

  if (!read_length (rtpgstdepay, map.data, map.size, &length, &offset))
    goto too_small;

  if (length == 0 || map.data[offset + length - 1] != '\0')
    goto invalid_buffer;

  GST_DEBUG_OBJECT (rtpgstdepay, "parsing caps %s", &map.data[offset]);

  /* parse and store in cache */
  caps = gst_caps_from_string ((gchar *) & map.data[offset]);
  gst_buffer_unmap (buf, &map);

  *skip = length + offset;

  return caps;

too_small:
  {
    GST_ELEMENT_WARNING (rtpgstdepay, STREAM, DECODE,
        ("Buffer too small."), (NULL));
    gst_buffer_unmap (buf, &map);
    return NULL;
  }
invalid_buffer:
  {
    GST_ELEMENT_WARNING (rtpgstdepay, STREAM, DECODE,
        ("caps string not 0-terminated."), (NULL));
    gst_buffer_unmap (buf, &map);
    return NULL;
  }
}

static GstEvent *
read_event (GstRtpGSTDepay * rtpgstdepay, guint type,
    GstBuffer * buf, guint * skip)
{
  guint offset, length;
  GstStructure *s;
  GstEvent *event;
  GstEventType etype;
  gchar *end;
  GstMapInfo map;

  gst_buffer_map (buf, &map, GST_MAP_READ);

  GST_DEBUG_OBJECT (rtpgstdepay, "buffer size %" G_GSIZE_FORMAT, map.size);

  if (!read_length (rtpgstdepay, map.data, map.size, &length, &offset))
    goto too_small;

  if (length == 0)
    goto invalid_buffer;
  /* backward compat, old payloader did not put 0-byte at the end */
  if (map.data[offset + length - 1] != '\0'
      && map.data[offset + length - 1] != ';')
    goto invalid_buffer;

  GST_DEBUG_OBJECT (rtpgstdepay, "parsing event %s", &map.data[offset]);

  /* parse */
  s = gst_structure_from_string ((gchar *) & map.data[offset], &end);
  gst_buffer_unmap (buf, &map);

  if (s == NULL)
    goto parse_failed;

  switch (type) {
    case 1:
      etype = GST_EVENT_TAG;
      break;
    case 2:
      etype = GST_EVENT_CUSTOM_DOWNSTREAM;
      break;
    case 3:
      etype = GST_EVENT_CUSTOM_BOTH;
      break;
    case 4:
      etype = GST_EVENT_STREAM_START;
      break;
    default:
      goto unknown_event;
  }
  event = gst_event_new_custom (etype, s);

  *skip = length + offset;

  return event;

too_small:
  {
    GST_ELEMENT_WARNING (rtpgstdepay, STREAM, DECODE,
        ("Buffer too small."), (NULL));
    gst_buffer_unmap (buf, &map);
    return NULL;
  }
invalid_buffer:
  {
    GST_ELEMENT_WARNING (rtpgstdepay, STREAM, DECODE,
        ("event string not 0-terminated."), (NULL));
    gst_buffer_unmap (buf, &map);
    return NULL;
  }
parse_failed:
  {
    GST_WARNING_OBJECT (rtpgstdepay, "could not parse event");
    return NULL;
  }
unknown_event:
  {
    GST_DEBUG_OBJECT (rtpgstdepay, "unknown event type");
    gst_structure_free (s);
    return NULL;
  }
}

static void
store_event (GstRtpGSTDepay * rtpgstdepay, GstEvent * event)
{
  gboolean do_push = FALSE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG:
    {
      GstTagList *old, *tags;

      gst_event_parse_tag (event, &tags);

      old = rtpgstdepay->tags;
      if (!old || !gst_tag_list_is_equal (old, tags)) {
        do_push = TRUE;
        if (old)
          gst_tag_list_unref (old);
        rtpgstdepay->tags = gst_tag_list_ref (tags);
      }
      break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM:
    case GST_EVENT_CUSTOM_BOTH:
      /* always push custom events */
      do_push = TRUE;
      break;
    case GST_EVENT_STREAM_START:
    {
      gchar *old;
      const gchar *stream_id = NULL;

      gst_event_parse_stream_start (event, &stream_id);

      old = rtpgstdepay->stream_id;
      if (!old || g_strcmp0 (old, stream_id)) {
        do_push = TRUE;
        g_free (old);
        rtpgstdepay->stream_id = g_strdup (stream_id);
      }
      break;
    }
    default:
      /* unknown event, don't push */
      break;
  }
  if (do_push)
    gst_pad_push_event (GST_RTP_BASE_DEPAYLOAD (rtpgstdepay)->srcpad, event);
  else
    gst_event_unref (event);
}

static GstBuffer *
gst_rtp_gst_depay_process (GstRTPBaseDepayload * depayload, GstRTPBuffer * rtp)
{
  GstRtpGSTDepay *rtpgstdepay;
  GstBuffer *subbuf, *outbuf = NULL;
  gint payload_len;
  guint8 *payload;
  guint CV, frag_offset, avail, offset;

  rtpgstdepay = GST_RTP_GST_DEPAY (depayload);

  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  if (payload_len <= 8)
    goto empty_packet;

  if (GST_BUFFER_IS_DISCONT (rtp->buffer)) {
    GST_WARNING_OBJECT (rtpgstdepay, "DISCONT, clear adapter");
    gst_adapter_clear (rtpgstdepay->adapter);
  }

  payload = gst_rtp_buffer_get_payload (rtp);

  /* strip off header
   *
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |C| CV  |D|0|0|0|     ETYPE     |  MBZ                          |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Frag_offset                          |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
  frag_offset =
      (payload[4] << 24) | (payload[5] << 16) | (payload[6] << 8) | payload[7];

  avail = gst_adapter_available (rtpgstdepay->adapter);
  if (avail != frag_offset)
    goto wrong_frag;

  /* subbuffer skipping the 8 header bytes */
  subbuf = gst_rtp_buffer_get_payload_subbuffer (rtp, 8, -1);
  gst_adapter_push (rtpgstdepay->adapter, subbuf);

  offset = 0;
  if (gst_rtp_buffer_get_marker (rtp)) {
    guint avail;
    GstCaps *outcaps;

    /* take the buffer */
    avail = gst_adapter_available (rtpgstdepay->adapter);
    outbuf = gst_adapter_take_buffer (rtpgstdepay->adapter, avail);

    CV = (payload[0] >> 4) & 0x7;

    if (payload[0] & 0x80) {
      guint size;

      /* C bit, we have inline caps */
      outcaps = read_caps (rtpgstdepay, outbuf, &size);
      if (outcaps == NULL)
        goto no_caps;

      GST_DEBUG_OBJECT (rtpgstdepay,
          "inline caps %u, length %u, %" GST_PTR_FORMAT, CV, size, outcaps);

      if (!rtpgstdepay->current_caps
          || !gst_caps_is_strictly_equal (rtpgstdepay->current_caps, outcaps))
        gst_pad_set_caps (depayload->srcpad, outcaps);
      gst_caps_replace (&rtpgstdepay->current_caps, outcaps);
      gst_caps_unref (outcaps);
      rtpgstdepay->current_CV = CV;

      /* skip caps */
      offset += size;
      avail -= size;
    }
    if (payload[1]) {
      guint size;
      GstEvent *event;

      /* we have an event */
      event = read_event (rtpgstdepay, payload[1], outbuf, &size);
      if (event == NULL)
        goto no_event;

      GST_DEBUG_OBJECT (rtpgstdepay,
          "inline event, length %u, %" GST_PTR_FORMAT, size, event);

      store_event (rtpgstdepay, event);

      /* no buffer after event */
      avail = 0;
    }

    if (avail) {
      if (offset != 0) {
        GstBuffer *temp;

        GST_DEBUG_OBJECT (rtpgstdepay, "sub buffer: offset %u, size %u", offset,
            avail);

        temp =
            gst_buffer_copy_region (outbuf, GST_BUFFER_COPY_ALL, offset, avail);

        gst_buffer_unref (outbuf);
        outbuf = temp;
      }

      /* see what caps we need */
      if (CV != rtpgstdepay->current_CV) {
        /* we need to switch caps but didn't receive the new caps yet */
        gst_caps_replace (&rtpgstdepay->current_caps, NULL);
        goto missing_caps;
      }

      if (payload[0] & 0x8)
        GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DELTA_UNIT);
    } else {
      gst_buffer_unref (outbuf);
      outbuf = NULL;
    }
  }

  if (outbuf) {
    gst_rtp_drop_meta (GST_ELEMENT_CAST (rtpgstdepay), outbuf, 0);
  }

  return outbuf;

  /* ERRORS */
empty_packet:
  {
    GST_ELEMENT_WARNING (rtpgstdepay, STREAM, DECODE,
        ("Empty Payload."), (NULL));
    return NULL;
  }
wrong_frag:
  {
    gst_adapter_clear (rtpgstdepay->adapter);
    GST_LOG_OBJECT (rtpgstdepay, "wrong fragment, skipping");
    return NULL;
  }
no_caps:
  {
    GST_WARNING_OBJECT (rtpgstdepay, "failed to parse caps");
    gst_buffer_unref (outbuf);
    return NULL;
  }
no_event:
  {
    GST_WARNING_OBJECT (rtpgstdepay, "failed to parse event");
    gst_buffer_unref (outbuf);
    return NULL;
  }
missing_caps:
  {
    GST_INFO_OBJECT (rtpgstdepay, "No caps received yet %u", CV);
    gst_buffer_unref (outbuf);

    gst_pad_push_event (GST_RTP_BASE_DEPAYLOAD_SINKPAD (rtpgstdepay),
        gst_video_event_new_upstream_force_key_unit (GST_CLOCK_TIME_NONE,
            TRUE, 0));

    return NULL;
  }
}

static gboolean
gst_rtp_gst_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * event)
{
  GstRtpGSTDepay *rtpgstdepay;

  rtpgstdepay = GST_RTP_GST_DEPAY (depay);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_gst_depay_reset (rtpgstdepay, FALSE);
      break;
    default:
      break;
  }

  return
      GST_RTP_BASE_DEPAYLOAD_CLASS (parent_class)->handle_event (depay, event);
}


static GstStateChangeReturn
gst_rtp_gst_depay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpGSTDepay *rtpgstdepay;
  GstStateChangeReturn ret;

  rtpgstdepay = GST_RTP_GST_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_gst_depay_reset (rtpgstdepay, TRUE);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_gst_depay_reset (rtpgstdepay, TRUE);
      break;
    default:
      break;
  }
  return ret;
}


gboolean
gst_rtp_gst_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpgstdepay",
      GST_RANK_MARGINAL, GST_TYPE_RTP_GST_DEPAY);
}
