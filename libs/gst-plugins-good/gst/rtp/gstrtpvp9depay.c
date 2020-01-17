/* gstrtpvp9depay.c - Source for GstRtpVP9Depay
 * Copyright (C) 2011 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2011 Collabora Ltd.
 *   Contact: Youness Alaoui <youness.alaoui@collabora.co.uk>
 * Copyright (C) 2015 Stian Selnes <stian@pexip.com>
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

#include "gstrtpvp9depay.h"
#include "gstrtputils.h"

#include <gst/video/video.h>

#include <stdio.h>

GST_DEBUG_CATEGORY_STATIC (gst_rtp_vp9_depay_debug);
#define GST_CAT_DEFAULT gst_rtp_vp9_depay_debug

static void gst_rtp_vp9_depay_dispose (GObject * object);
static GstBuffer *gst_rtp_vp9_depay_process (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp);
static GstStateChangeReturn gst_rtp_vp9_depay_change_state (GstElement *
    element, GstStateChange transition);
static gboolean gst_rtp_vp9_depay_handle_event (GstRTPBaseDepayload * depay,
    GstEvent * event);

G_DEFINE_TYPE (GstRtpVP9Depay, gst_rtp_vp9_depay, GST_TYPE_RTP_BASE_DEPAYLOAD);

static GstStaticPadTemplate gst_rtp_vp9_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp9"));

static GstStaticPadTemplate gst_rtp_vp9_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "clock-rate = (int) 90000,"
        "media = (string) \"video\","
        "encoding-name = (string) { \"VP9\", \"VP9-DRAFT-IETF-01\" }"));

static void
gst_rtp_vp9_depay_init (GstRtpVP9Depay * self)
{
  self->adapter = gst_adapter_new ();
  self->started = FALSE;
}

static void
gst_rtp_vp9_depay_class_init (GstRtpVP9DepayClass * gst_rtp_vp9_depay_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (gst_rtp_vp9_depay_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (gst_rtp_vp9_depay_class);
  GstRTPBaseDepayloadClass *depay_class =
      (GstRTPBaseDepayloadClass *) (gst_rtp_vp9_depay_class);


  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp9_depay_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp9_depay_src_template);

  gst_element_class_set_static_metadata (element_class, "RTP VP9 depayloader",
      "Codec/Depayloader/Network/RTP",
      "Extracts VP9 video from RTP packets)", "Stian Selnes <stian@pexip.com>");

  object_class->dispose = gst_rtp_vp9_depay_dispose;

  element_class->change_state = gst_rtp_vp9_depay_change_state;

  depay_class->process_rtp_packet = gst_rtp_vp9_depay_process;
  depay_class->handle_event = gst_rtp_vp9_depay_handle_event;

  GST_DEBUG_CATEGORY_INIT (gst_rtp_vp9_depay_debug, "rtpvp9depay", 0,
      "VP9 Video RTP Depayloader");
}

static void
gst_rtp_vp9_depay_dispose (GObject * object)
{
  GstRtpVP9Depay *self = GST_RTP_VP9_DEPAY (object);

  if (self->adapter != NULL)
    g_object_unref (self->adapter);
  self->adapter = NULL;

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (gst_rtp_vp9_depay_parent_class)->dispose)
    G_OBJECT_CLASS (gst_rtp_vp9_depay_parent_class)->dispose (object);
}

static GstBuffer *
gst_rtp_vp9_depay_process (GstRTPBaseDepayload * depay, GstRTPBuffer * rtp)
{
  GstRtpVP9Depay *self = GST_RTP_VP9_DEPAY (depay);
  GstBuffer *payload;
  guint8 *data;
  guint hdrsize = 1;
  guint size;
  gint spatial_layer = 0;
  gboolean i_bit, p_bit, l_bit, f_bit, b_bit, e_bit, v_bit;

  if (G_UNLIKELY (GST_BUFFER_IS_DISCONT (rtp->buffer))) {
    GST_LOG_OBJECT (self, "Discontinuity, flushing adapter");
    gst_adapter_clear (self->adapter);
    self->started = FALSE;
  }

  size = gst_rtp_buffer_get_payload_len (rtp);

  /* Mandatory with at least one header and one vp9 byte */
  if (G_UNLIKELY (size < hdrsize + 1))
    goto too_small;

  data = gst_rtp_buffer_get_payload (rtp);
  i_bit = (data[0] & 0x80) != 0;
  p_bit = (data[0] & 0x40) != 0;
  l_bit = (data[0] & 0x20) != 0;
  f_bit = (data[0] & 0x10) != 0;
  b_bit = (data[0] & 0x08) != 0;
  e_bit = (data[0] & 0x04) != 0;
  v_bit = (data[0] & 0x02) != 0;

  if (G_UNLIKELY (!self->started)) {
    /* Check if this is the start of a VP9 layer frame, otherwise bail */
    if (!b_bit)
      goto done;

    self->started = TRUE;
  }

  GST_TRACE_OBJECT (self, "IPLFBEV : %d%d%d%d%d%d%d", i_bit, p_bit, l_bit,
      f_bit, b_bit, e_bit, v_bit);

  /* Check I optional header Picture ID */
  if (i_bit) {
    hdrsize++;
    if (G_UNLIKELY (size < hdrsize + 1))
      goto too_small;
    /* Check M for 15 bits PictureID */
    if ((data[1] & 0x80) != 0) {
      hdrsize++;
      if (G_UNLIKELY (size < hdrsize + 1))
        goto too_small;
    }
  }

  /* Check L optional header layer indices */
  if (l_bit) {
    hdrsize++;
    /* Check TL0PICIDX temporal layer zero index (non-flexible mode) */
    if (!f_bit)
      hdrsize++;
  }

  if (p_bit && f_bit) {
    gint i;

    /* At least one P_DIFF|N, up to three times */
    for (i = 0; i < 3; i++) {
      guint p_diff, n_bit;

      if (G_UNLIKELY (size < hdrsize + 1))
        goto too_small;

      p_diff = data[hdrsize] >> 1;
      n_bit = data[hdrsize] & 0x1;
      GST_TRACE_OBJECT (self, "P_DIFF[%d]=%d", i, p_diff);
      hdrsize++;

      if (!n_bit)
        break;
    }
  }

  /* Check V optional Scalability Structure */
  if (v_bit) {
    guint n_s, y_bit, g_bit;
    guint8 *ss = &data[hdrsize];
    guint sssize = 1;

    if (G_UNLIKELY (size < hdrsize + sssize + 1))
      goto too_small;

    n_s = (ss[0] & 0xe0) >> 5;
    y_bit = (ss[0] & 0x10) != 0;
    g_bit = (ss[0] & 0x08) != 0;

    GST_TRACE_OBJECT (self, "SS header: N_S=%u, Y=%u, G=%u", n_s, y_bit, g_bit);

    sssize += y_bit ? (n_s + 1) * 4 : 0;
    if (G_UNLIKELY (size < hdrsize + sssize + 1))
      goto too_small;

    if (y_bit) {
      guint i;
      for (i = 0; i <= n_s; i++) {
        /* For now, simply use the last layer specified for width and height */
        self->ss_width = ss[1 + i * 4] * 256 + ss[2 + i * 4];
        self->ss_height = ss[3 + i * 4] * 256 + ss[4 + i * 4];
        GST_TRACE_OBJECT (self, "N_S[%d]: WIDTH=%u, HEIGHT=%u", i,
            self->ss_width, self->ss_height);
      }
    }

    if (g_bit) {
      guint i, j;
      guint n_g = ss[sssize];
      sssize++;
      if (G_UNLIKELY (size < hdrsize + sssize + 1))
        goto too_small;
      for (i = 0; i < n_g; i++) {
        guint t = (ss[sssize] & 0xe0) >> 5;
        guint u = (ss[sssize] & 0x10) >> 4;
        guint r = (ss[sssize] & 0x0c) >> 2;
        GST_TRACE_OBJECT (self, "N_G[%u]: 0x%02x -> T=%u, U=%u, R=%u", i,
            ss[sssize], t, u, r);
        for (j = 0; j < r; j++)
          GST_TRACE_OBJECT (self, "  R[%u]: P_DIFF=%u", j, ss[sssize + 1 + j]);
        sssize += 1 + r;
        if (G_UNLIKELY (size < hdrsize + sssize + 1))
          goto too_small;
      }
    }
    hdrsize += sssize;
  }

  GST_DEBUG_OBJECT (depay, "hdrsize %u, size %u", hdrsize, size);

  if (G_UNLIKELY (hdrsize >= size))
    goto too_small;

  payload = gst_rtp_buffer_get_payload_subbuffer (rtp, hdrsize, -1);
  {
    GstMapInfo map;
    gst_buffer_map (payload, &map, GST_MAP_READ);
    GST_MEMDUMP_OBJECT (self, "vp9 payload", map.data, 16);
    gst_buffer_unmap (payload, &map);
  }
  gst_adapter_push (self->adapter, payload);

  /* Marker indicates that it was the last rtp packet for this frame */
  if (gst_rtp_buffer_get_marker (rtp)) {
    GstBuffer *out;
    gboolean key_frame_first_layer = !p_bit && spatial_layer == 0;

    if (gst_adapter_available (self->adapter) < 10)
      goto too_small;

    out = gst_adapter_take_buffer (self->adapter,
        gst_adapter_available (self->adapter));

    self->started = FALSE;

    /* mark keyframes */
    out = gst_buffer_make_writable (out);
    /* Filter away all metas that are not sensible to copy */
    gst_rtp_drop_non_video_meta (self, out);
    if (!key_frame_first_layer) {
      GST_BUFFER_FLAG_SET (out, GST_BUFFER_FLAG_DELTA_UNIT);

      if (!self->caps_sent) {
        gst_buffer_unref (out);
        out = NULL;
        GST_INFO_OBJECT (self, "Dropping inter-frame before intra-frame");
        gst_pad_push_event (GST_RTP_BASE_DEPAYLOAD_SINKPAD (depay),
            gst_video_event_new_upstream_force_key_unit (GST_CLOCK_TIME_NONE,
                TRUE, 0));
      }
    } else {
      GST_BUFFER_FLAG_UNSET (out, GST_BUFFER_FLAG_DELTA_UNIT);

      if (self->last_width != self->ss_width ||
          self->last_height != self->ss_height) {
        GstCaps *srccaps;

        /* Width and height are optional in the RTP header. Consider to parse
         * the frame header in addition if missing from RTP header */
        if (self->ss_width != 0 && self->ss_height != 0) {
          srccaps = gst_caps_new_simple ("video/x-vp9",
              "framerate", GST_TYPE_FRACTION, 0, 1,
              "width", G_TYPE_INT, self->ss_width,
              "height", G_TYPE_INT, self->ss_height, NULL);
        } else {
          srccaps = gst_caps_new_simple ("video/x-vp9",
              "framerate", GST_TYPE_FRACTION, 0, 1, NULL);
        }

        gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depay), srccaps);
        gst_caps_unref (srccaps);

        self->caps_sent = TRUE;
        self->last_width = self->ss_width;
        self->last_height = self->ss_height;
        self->ss_width = 0;
        self->ss_height = 0;
      }
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
gst_rtp_vp9_depay_change_state (GstElement * element, GstStateChange transition)
{
  GstRtpVP9Depay *self = GST_RTP_VP9_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      self->last_width = -1;
      self->last_height = -1;
      self->caps_sent = FALSE;
      break;
    default:
      break;
  }

  return
      GST_ELEMENT_CLASS (gst_rtp_vp9_depay_parent_class)->change_state (element,
      transition);
}

static gboolean
gst_rtp_vp9_depay_handle_event (GstRTPBaseDepayload * depay, GstEvent * event)
{
  GstRtpVP9Depay *self = GST_RTP_VP9_DEPAY (depay);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      self->last_width = -1;
      self->last_height = -1;
      break;
    default:
      break;
  }

  return
      GST_RTP_BASE_DEPAYLOAD_CLASS
      (gst_rtp_vp9_depay_parent_class)->handle_event (depay, event);
}

gboolean
gst_rtp_vp9_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvp9depay",
      GST_RANK_MARGINAL, GST_TYPE_RTP_VP9_DEPAY);
}
