/* gstrtpvp8depay.c - Source for GstRtpVP8Depay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2011 Collabora Ltd.
 *   Contact: Youness Alaoui <youness.alaoui@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "gstrtpvp8depay.h"
#include "gstrtputils.h"

#include <gst/video/video.h>

#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (gst_rtp_vp8_depay_debug);
#define GST_CAT_DEFAULT gst_rtp_vp8_depay_debug

static void gst_rtp_vp8_depay_dispose (GObject * object);
static void gst_rtp_vp8_depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_vp8_depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static GstBuffer *gst_rtp_vp8_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static GstStateChangeReturn gst_rtp_vp8_depay_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_rtp_vp8_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * event);

G_DEFINE_TYPE (GstRtpVP8Depay, gst_rtp_vp8_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static GstStaticPadTemplate gst_rtp_vp8_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp8"));

static GstStaticPadTemplate gst_rtp_vp8_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "clock-rate = (int) 90000,"
        "media = (string) \"video\","
        "encoding-name = (string) { \"VP8\", \"VP8-DRAFT-IETF-01\" }"));

#define DEFAULT_WAIT_FOR_KEYFRAME FALSE

enum
{
  PROP_0,
  PROP_WAIT_FOR_KEYFRAME
};

static void
gst_rtp_vp8_depay_init (GstRtpVP8Depay * self)
{
  self->adapter = gst_adapter_new ();
  self->started = FALSE;
  self->wait_for_keyframe = DEFAULT_WAIT_FOR_KEYFRAME;
}

static void
gst_rtp_vp8_depay_class_init (GstRtpVP8DepayClass * gst_rtp_vp8_depay_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (gst_rtp_vp8_depay_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (gst_rtp_vp8_depay_class);
  GstRTPBaseDepayloadClass *depay_class =
      (GstRTPBaseDepayloadClass *) (gst_rtp_vp8_depay_class);

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp8_depay_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp8_depay_src_template);

  gst_element_class_set_static_metadata (element_class, "RTP VP8 depayloader",
      "Codec/Depayloader/Network/RTP",
      "Extracts VP8 video from RTP packets)",
      "Sjoerd Simons <sjoerd@luon.net>");

  object_class->dispose = gst_rtp_vp8_depay_dispose;
  object_class->set_property = gst_rtp_vp8_depay_set_property;
  object_class->get_property = gst_rtp_vp8_depay_get_property;

  g_object_class_install_property (object_class, PROP_WAIT_FOR_KEYFRAME,
      g_param_spec_boolean ("wait-for-keyframe", "Wait for Keyframe",
          "Wait for the next keyframe after packet loss",
          DEFAULT_WAIT_FOR_KEYFRAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  element_class->change_state = gst_rtp_vp8_depay_change_state;

  depay_class->process_rtp_packet = gst_rtp_vp8_depay_process;
  depay_class->handle_event = gst_rtp_vp8_depay_handle_event;

  GST_DEBUG_CATEGORY_INIT (gst_rtp_vp8_depay_debug, "rtpvp8depay", 0,
      "VP8 Video RTP Depayloader");
}

static void
gst_rtp_vp8_depay_dispose (GObject * object)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (object);

  if (self->adapter != NULL)
    g_object_unref (self->adapter);
  self->adapter = NULL;

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (gst_rtp_vp8_depay_parent_class)->dispose)
    G_OBJECT_CLASS (gst_rtp_vp8_depay_parent_class)->dispose (object);
}

static void
gst_rtp_vp8_depay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (object);

  switch (prop_id) {
    case PROP_WAIT_FOR_KEYFRAME:
      self->wait_for_keyframe = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_vp8_depay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (object);

  switch (prop_id) {
    case PROP_WAIT_FOR_KEYFRAME:
      g_value_set_boolean (value, self->wait_for_keyframe);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstBuffer *
gst_rtp_vp8_depay_process (GstRTPBaseDepayload * depay, GstRTPBuffer * rtp)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (depay);
  GstBuffer *payload;
  guint8 *data;
  guint hdrsize;
  guint size;

  if (G_UNLIKELY (GST_BUFFER_IS_DISCONT (rtp->buffer))) {
    GST_LOG_OBJECT (self, "Discontinuity, flushing adapter");
    gst_adapter_clear (self->adapter);
    self->started = FALSE;

    if (self->wait_for_keyframe)
      self->waiting_for_keyframe = TRUE;
  }

  size = gst_rtp_buffer_get_payload_len (rtp);

  /* At least one header and one vp8 byte */
  if (G_UNLIKELY (size < 2))
    goto too_small;

  data = gst_rtp_buffer_get_payload (rtp);

  if (G_UNLIKELY (!self->started)) {
    /* Check if this is the start of a VP8 frame, otherwise bail */
    /* S=1 and PartID= 0 */
    if ((data[0] & 0x17) != 0x10)
      goto done;

    self->started = TRUE;
  }

  hdrsize = 1;
  /* Check X optional header */
  if ((data[0] & 0x80) != 0) {
    hdrsize++;
    /* Check I optional header */
    if ((data[1] & 0x80) != 0) {
      if (G_UNLIKELY (size < 3))
        goto too_small;
      hdrsize++;
      /* Check for 16 bits PictureID */
      if ((data[2] & 0x80) != 0)
        hdrsize++;
    }
    /* Check L optional header */
    if ((data[1] & 0x40) != 0)
      hdrsize++;
    /* Check T or K optional headers */
    if ((data[1] & 0x20) != 0 || (data[1] & 0x10) != 0)
      hdrsize++;
  }
  GST_DEBUG_OBJECT (depay, "hdrsize %u, size %u", hdrsize, size);

  if (G_UNLIKELY (hdrsize >= size))
    goto too_small;

  payload = gst_rtp_buffer_get_payload_subbuffer (rtp, hdrsize, -1);
  gst_adapter_push (self->adapter, payload);

  /* Marker indicates that it was the last rtp packet for this frame */
  if (gst_rtp_buffer_get_marker (rtp)) {
    GstBuffer *out;
    guint8 header[10];

    if (gst_adapter_available (self->adapter) < 10)
      goto too_small;
    gst_adapter_copy (self->adapter, &header, 0, 10);

    out = gst_adapter_take_buffer (self->adapter,
        gst_adapter_available (self->adapter));

    self->started = FALSE;

    /* mark keyframes */
    out = gst_buffer_make_writable (out);
    /* Filter away all metas that are not sensible to copy */
    gst_rtp_drop_non_video_meta (self, out);
    if ((header[0] & 0x01)) {
      GST_BUFFER_FLAG_SET (out, GST_BUFFER_FLAG_DELTA_UNIT);

      if (self->waiting_for_keyframe) {
        gst_buffer_unref (out);
        out = NULL;
        GST_INFO_OBJECT (self, "Dropping inter-frame before intra-frame");
        gst_pad_push_event (GST_RTP_BASE_DEPAYLOAD_SINKPAD (depay),
            gst_video_event_new_upstream_force_key_unit (GST_CLOCK_TIME_NONE,
                TRUE, 0));
      }
    } else {
      guint profile, width, height;

      GST_BUFFER_FLAG_UNSET (out, GST_BUFFER_FLAG_DELTA_UNIT);

      profile = (header[0] & 0x0e) >> 1;
      width = GST_READ_UINT16_LE (header + 6) & 0x3fff;
      height = GST_READ_UINT16_LE (header + 8) & 0x3fff;

      if (G_UNLIKELY (self->last_width != width ||
              self->last_height != height || self->last_profile != profile)) {
        gchar profile_str[3];
        GstCaps *srccaps;

        snprintf (profile_str, 3, "%u", profile);
        srccaps = gst_caps_new_simple ("video/x-vp8",
            "framerate", GST_TYPE_FRACTION, 0, 1,
            "height", G_TYPE_INT, height,
            "width", G_TYPE_INT, width,
            "profile", G_TYPE_STRING, profile_str, NULL);

        gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depay), srccaps);
        gst_caps_unref (srccaps);

        self->last_width = width;
        self->last_height = height;
        self->last_profile = profile;
      }
      self->waiting_for_keyframe = FALSE;
    }

    return out;
  }

done:
  return NULL;

too_small:
  GST_LOG_OBJECT (self, "Invalid rtp packet (too small), ignoring");
  gst_adapter_clear (self->adapter);
  self->started = FALSE;

  goto done;
}

static GstStateChangeReturn
gst_rtp_vp8_depay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      self->last_profile = -1;
      self->last_height = -1;
      self->last_width = -1;
      self->waiting_for_keyframe = TRUE;
      break;
    default:
      break;
  }

  return
      GST_ELEMENT_CLASS (gst_rtp_vp8_depay_parent_class)->change_state (element,
      transition);
}

static gboolean
gst_rtp_vp8_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * event)
{
  GstRtpVP8Depay *self = GST_RTP_VP8_DEPAY (depay);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      self->last_profile = -1;
      self->last_height = -1;
      self->last_width = -1;
      break;
    default:
      break;
  }

  return
      GST_RTP_BASE_DEPAYLOAD_CLASS
      (gst_rtp_vp8_depay_parent_class)->handle_event (depay, event);
}

gboolean
gst_rtp_vp8_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvp8depay",
      GST_RANK_MARGINAL, GST_TYPE_RTP_VP8_DEPAY);
}
