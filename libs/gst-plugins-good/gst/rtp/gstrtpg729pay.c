/* GStreamer
 * Copyright (C) <2007> Nokia Corporation
 * Copyright (C) <2007> Collabora Ltd
 *  @author: Olivier Crete <olivier.crete@collabora.co.uk>
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

/*
 * This payloader assumes that the data will ALWAYS come as zero or more
 * 10 bytes frame of audio followed by 0 or 1 2 byte frame of silence.
 * Any other buffer format won't work
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/base/gstadapter.h>
#include <gst/audio/audio.h>

#include "gstrtpg729pay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpg729pay_debug);
#define GST_CAT_DEFAULT (rtpg729pay_debug)

#define G729_FRAME_SIZE 10
#define G729B_CN_FRAME_SIZE 2
#define G729_FRAME_DURATION (10 * GST_MSECOND)
#define G729_FRAME_DURATION_MS (10)

static gboolean
gst_rtp_g729_pay_set_caps (GstRTPBasePayload * payload, GstCaps * caps);
static GstFlowReturn
gst_rtp_g729_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buf);

static GstStateChangeReturn
gst_rtp_g729_pay_change_state (GstElement * element, GstStateChange transition);

static GstStaticPadTemplate gst_rtp_g729_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/G729, "     /* according to RFC 3555 */
        "channels = (int) 1, " "rate = (int) 8000")
    );

static GstStaticPadTemplate gst_rtp_g729_pay_src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_G729_STRING ", "
        "clock-rate = (int) 8000, "
        "encoding-name = (string) \"G729\"; "
        "application/x-rtp, "
        "media = (string) \"audio\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 8000, " "encoding-name = (string) \"G729\"")
    );

#define gst_rtp_g729_pay_parent_class parent_class
G_DEFINE_TYPE (GstRTPG729Pay, gst_rtp_g729_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_g729_pay_finalize (GObject * object)
{
  GstRTPG729Pay *pay = GST_RTP_G729_PAY (object);

  g_object_unref (pay->adapter);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_rtp_g729_pay_class_init (GstRTPG729PayClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstRTPBasePayloadClass *payload_class = GST_RTP_BASE_PAYLOAD_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (rtpg729pay_debug, "rtpg729pay", 0,
      "G.729 RTP Payloader");

  gobject_class->finalize = gst_rtp_g729_pay_finalize;

  gstelement_class->change_state = gst_rtp_g729_pay_change_state;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g729_pay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_g729_pay_src_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP G.729 payloader", "Codec/Payloader/Network/RTP",
      "Packetize G.729 audio into RTP packets",
      "Olivier Crete <olivier.crete@collabora.co.uk>");

  payload_class->set_caps = gst_rtp_g729_pay_set_caps;
  payload_class->handle_buffer = gst_rtp_g729_pay_handle_buffer;
}

static void
gst_rtp_g729_pay_init (GstRTPG729Pay * pay)
{
  GstRTPBasePayload *payload = GST_RTP_BASE_PAYLOAD (pay);

  payload->pt = GST_RTP_PAYLOAD_G729;

  pay->adapter = gst_adapter_new ();
}

static void
gst_rtp_g729_pay_reset (GstRTPG729Pay * pay)
{
  gst_adapter_clear (pay->adapter);
  pay->discont = FALSE;
  pay->next_rtp_time = 0;
  pay->first_ts = GST_CLOCK_TIME_NONE;
  pay->first_rtp_time = 0;
}

static gboolean
gst_rtp_g729_pay_set_caps (GstRTPBasePayload * payload, GstCaps * caps)
{
  gboolean res;

  gst_rtp_base_payload_set_options (payload, "audio",
      payload->pt != GST_RTP_PAYLOAD_G729, "G729", 8000);

  res = gst_rtp_base_payload_set_outcaps (payload, NULL);

  return res;
}

static GstFlowReturn
gst_rtp_g729_pay_push (GstRTPG729Pay * rtpg729pay, GstBuffer * buf)
{
  GstRTPBasePayload *basepayload;
  GstClockTime duration;
  guint frames;
  GstBuffer *outbuf;
  GstFlowReturn ret;
  GstRTPBuffer rtp = { NULL };
  guint payload_len = gst_buffer_get_size (buf);

  basepayload = GST_RTP_BASE_PAYLOAD (rtpg729pay);

  GST_DEBUG_OBJECT (rtpg729pay, "Pushing %d bytes ts %" GST_TIME_FORMAT,
      payload_len, GST_TIME_ARGS (rtpg729pay->next_ts));

  /* create buffer to hold the payload */
  outbuf = gst_rtp_buffer_new_allocate (0, 0, 0);

  gst_rtp_buffer_map (outbuf, GST_MAP_READWRITE, &rtp);

  /* set metadata */
  frames =
      (payload_len / G729_FRAME_SIZE) + ((payload_len % G729_FRAME_SIZE) >> 1);
  duration = frames * G729_FRAME_DURATION;
  GST_BUFFER_PTS (outbuf) = rtpg729pay->next_ts;
  GST_BUFFER_DURATION (outbuf) = duration;
  GST_BUFFER_OFFSET (outbuf) = rtpg729pay->next_rtp_time;
  rtpg729pay->next_ts += duration;
  rtpg729pay->next_rtp_time += frames * 80;

  if (G_UNLIKELY (rtpg729pay->discont)) {
    GST_DEBUG_OBJECT (basepayload, "discont, setting marker bit");
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
    gst_rtp_buffer_set_marker (&rtp, TRUE);
    rtpg729pay->discont = FALSE;
  }
  gst_rtp_buffer_unmap (&rtp);

  /* append payload */
  gst_rtp_copy_audio_meta (basepayload, outbuf, buf);
  outbuf = gst_buffer_append (outbuf, buf);

  ret = gst_rtp_base_payload_push (basepayload, outbuf);

  return ret;
}

static void
gst_rtp_g729_pay_recalc_rtp_time (GstRTPG729Pay * rtpg729pay, GstClockTime time)
{
  if (GST_CLOCK_TIME_IS_VALID (rtpg729pay->first_ts)
      && GST_CLOCK_TIME_IS_VALID (time) && time >= rtpg729pay->first_ts) {
    GstClockTime diff;
    guint32 rtpdiff;

    diff = time - rtpg729pay->first_ts;
    rtpdiff = (diff / GST_MSECOND) * 8;
    rtpg729pay->next_rtp_time = rtpg729pay->first_rtp_time + rtpdiff;
    GST_DEBUG_OBJECT (rtpg729pay,
        "elapsed time %" GST_TIME_FORMAT ", rtp %" G_GUINT32_FORMAT ", "
        "new offset %" G_GUINT32_FORMAT, GST_TIME_ARGS (diff), rtpdiff,
        rtpg729pay->next_rtp_time);
  }
}

static GstFlowReturn
gst_rtp_g729_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buf)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstRTPG729Pay *rtpg729pay = GST_RTP_G729_PAY (payload);
  GstAdapter *adapter = NULL;
  guint payload_len;
  guint available;
  guint maxptime_octets = G_MAXUINT;
  guint minptime_octets = 0;
  guint min_payload_len;
  guint max_payload_len;
  gsize size;
  GstClockTime timestamp;

  size = gst_buffer_get_size (buf);

  if (size % G729_FRAME_SIZE != 0 &&
      size % G729_FRAME_SIZE != G729B_CN_FRAME_SIZE)
    goto invalid_size;

  /* max number of bytes based on given ptime, has to be multiple of
   * frame_duration */
  if (payload->max_ptime != -1) {
    guint ptime_ms = payload->max_ptime / GST_MSECOND;

    maxptime_octets = G729_FRAME_SIZE *
        (int) (ptime_ms / G729_FRAME_DURATION_MS);

    if (maxptime_octets < G729_FRAME_SIZE) {
      GST_WARNING_OBJECT (payload, "Given ptime %" G_GINT64_FORMAT
          " is smaller than minimum %d ns, overwriting to minimum",
          payload->max_ptime, G729_FRAME_DURATION_MS);
      maxptime_octets = G729_FRAME_SIZE;
    }
  }

  max_payload_len = MIN (
      /* MTU max */
      (int) (gst_rtp_buffer_calc_payload_len (GST_RTP_BASE_PAYLOAD_MTU
              (payload), 0, 0) / G729_FRAME_SIZE)
      * G729_FRAME_SIZE,
      /* ptime max */
      maxptime_octets);

  /* min number of bytes based on a given ptime, has to be a multiple
     of frame duration */
  {
    guint64 min_ptime = payload->min_ptime;

    min_ptime = min_ptime / GST_MSECOND;
    minptime_octets = G729_FRAME_SIZE *
        (int) (min_ptime / G729_FRAME_DURATION_MS);
  }

  min_payload_len = MAX (minptime_octets, G729_FRAME_SIZE);

  if (min_payload_len > max_payload_len) {
    min_payload_len = max_payload_len;
  }

  /* If the ptime is specified in the caps, tried to adhere to it exactly */
  if (payload->ptime) {
    guint64 ptime = payload->ptime / GST_MSECOND;
    guint ptime_in_bytes = G729_FRAME_SIZE *
        (guint) (ptime / G729_FRAME_DURATION_MS);

    /* clip to computed min and max lengths */
    ptime_in_bytes = MAX (min_payload_len, ptime_in_bytes);
    ptime_in_bytes = MIN (max_payload_len, ptime_in_bytes);

    min_payload_len = max_payload_len = ptime_in_bytes;
  }

  GST_LOG_OBJECT (payload,
      "Calculated min_payload_len %u and max_payload_len %u",
      min_payload_len, max_payload_len);

  adapter = rtpg729pay->adapter;
  available = gst_adapter_available (adapter);

  timestamp = GST_BUFFER_PTS (buf);

  /* resync rtp time on discont or a discontinuous cn packet */
  if (GST_BUFFER_IS_DISCONT (buf)) {
    /* flush remainder */
    if (available > 0) {
      gst_rtp_g729_pay_push (rtpg729pay,
          gst_adapter_take_buffer_fast (adapter, available));
      available = 0;
    }
    rtpg729pay->discont = TRUE;
    gst_rtp_g729_pay_recalc_rtp_time (rtpg729pay, timestamp);
  }

  if (size < G729_FRAME_SIZE)
    gst_rtp_g729_pay_recalc_rtp_time (rtpg729pay, timestamp);

  if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (rtpg729pay->first_ts))) {
    rtpg729pay->first_ts = timestamp;
    rtpg729pay->first_rtp_time = rtpg729pay->next_rtp_time;
  }

  /* let's reset the base timestamp when the adapter is empty */
  if (available == 0)
    rtpg729pay->next_ts = timestamp;

  if (available == 0 && size >= min_payload_len && size <= max_payload_len) {
    ret = gst_rtp_g729_pay_push (rtpg729pay, buf);
    return ret;
  }

  gst_adapter_push (adapter, buf);
  available = gst_adapter_available (adapter);

  /* as long as we have full frames */
  /* this loop will push all available buffers till the last frame */
  while (available >= min_payload_len ||
      available % G729_FRAME_SIZE == G729B_CN_FRAME_SIZE) {
    /* We send as much as we can */
    if (available <= max_payload_len) {
      payload_len = available;
    } else {
      payload_len = MIN (max_payload_len,
          (available / G729_FRAME_SIZE) * G729_FRAME_SIZE);
    }

    ret = gst_rtp_g729_pay_push (rtpg729pay,
        gst_adapter_take_buffer_fast (adapter, payload_len));
    available -= payload_len;
  }

  return ret;

  /* ERRORS */
invalid_size:
  {
    GST_ELEMENT_ERROR (payload, STREAM, WRONG_TYPE,
        ("Invalid input buffer size"),
        ("Invalid buffer size, should be a multiple of"
            " G729_FRAME_SIZE(10) with an optional G729B_CN_FRAME_SIZE(2)"
            " added to it, but it is %" G_GSIZE_FORMAT, size));
    gst_buffer_unref (buf);
    return GST_FLOW_ERROR;
  }
}

static GstStateChangeReturn
gst_rtp_g729_pay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  /* handle upwards state changes here */
  switch (transition) {
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  /* handle downwards state changes */
  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_g729_pay_reset (GST_RTP_G729_PAY (element));
      break;
    default:
      break;
  }

  return ret;
}

gboolean
gst_rtp_g729_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpg729pay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_G729_PAY);
}
