/* GStreamer
 * Copyright (C) <2008> Wim Taymans <wim.taymans@gmail.com>
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

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

#include <string.h>
#include <stdlib.h>
#include "gstrtpvrawdepay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (rtpvrawdepay_debug);
#define GST_CAT_DEFAULT (rtpvrawdepay_debug)

static GstStaticPadTemplate gst_rtp_vraw_depay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw")
    );

static GstStaticPadTemplate gst_rtp_vraw_depay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "clock-rate = (int) 90000, "
        "encoding-name = (string) \"RAW\", "
        "sampling = (string) { \"RGB\", \"RGBA\", \"BGR\", \"BGRA\", "
        "\"YCbCr-4:4:4\", \"YCbCr-4:2:2\", \"YCbCr-4:2:0\", "
        "\"YCbCr-4:1:1\" },"
        /* we cannot express these as strings 
         * "width = (string) [1 32767],"
         * "height = (string) [1 32767],"
         */
        "depth = (string) { \"8\", \"10\", \"12\", \"16\" }")
    );

#define gst_rtp_vraw_depay_parent_class parent_class
G_DEFINE_TYPE (GstRtpVRawDepay, gst_rtp_vraw_depay,
    GST_TYPE_RTP_BASE_DEPAYLOAD);

static gboolean gst_rtp_vraw_depay_setcaps (GstRTPBaseDepayload * depayload,
    GstCaps * caps);
static GstBuffer *gst_rtp_vraw_depay_process_packet (GstRTPBaseDepayload *
    depay, GstRTPBuffer * rtp);

static GstStateChangeReturn gst_rtp_vraw_depay_change_state (GstElement *
    element, GstStateChange transition);

static gboolean gst_rtp_vraw_depay_handle_event (GstRTPBaseDepayload * filter,
    GstEvent * event);

static void
gst_rtp_vraw_depay_class_init (GstRtpVRawDepayClass * klass)
{
  GstElementClass *gstelement_class;
  GstRTPBaseDepayloadClass *gstrtpbasedepayload_class;

  gstelement_class = (GstElementClass *) klass;
  gstrtpbasedepayload_class = (GstRTPBaseDepayloadClass *) klass;

  gstelement_class->change_state = gst_rtp_vraw_depay_change_state;

  gstrtpbasedepayload_class->set_caps = gst_rtp_vraw_depay_setcaps;
  gstrtpbasedepayload_class->process_rtp_packet =
      gst_rtp_vraw_depay_process_packet;
  gstrtpbasedepayload_class->handle_event = gst_rtp_vraw_depay_handle_event;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vraw_depay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vraw_depay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Raw Video depayloader", "Codec/Depayloader/Network/RTP",
      "Extracts raw video from RTP packets (RFC 4175)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpvrawdepay_debug, "rtpvrawdepay", 0,
      "raw video RTP Depayloader");
}

static void
gst_rtp_vraw_depay_init (GstRtpVRawDepay * rtpvrawdepay)
{
}

static void
gst_rtp_vraw_depay_reset (GstRtpVRawDepay * rtpvrawdepay, gboolean full)
{
  if (rtpvrawdepay->outbuf) {
    gst_video_frame_unmap (&rtpvrawdepay->frame);
    gst_buffer_unref (rtpvrawdepay->outbuf);
    rtpvrawdepay->outbuf = NULL;
  }
  rtpvrawdepay->timestamp = -1;

  if (full && rtpvrawdepay->pool) {
    gst_buffer_pool_set_active (rtpvrawdepay->pool, FALSE);
    gst_object_unref (rtpvrawdepay->pool);
    rtpvrawdepay->pool = NULL;
  }
}

static GstFlowReturn
gst_rtp_vraw_depay_negotiate_pool (GstRtpVRawDepay * depay, GstCaps * caps,
    GstVideoInfo * info)
{
  GstQuery *query;
  GstBufferPool *pool = NULL;
  guint size, min, max;
  GstStructure *config;

  /* find a pool for the negotiated caps now */
  query = gst_query_new_allocation (caps, TRUE);

  if (!gst_pad_peer_query (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depay), query)) {
    /* not a problem, we use the defaults of query */
    GST_DEBUG_OBJECT (depay, "could not get downstream ALLOCATION hints");
  }

  if (gst_query_get_n_allocation_pools (query) > 0) {
    /* we got configuration from our peer, parse them */
    gst_query_parse_nth_allocation_pool (query, 0, &pool, &size, &min, &max);
  } else {
    GST_DEBUG_OBJECT (depay, "didn't get downstream pool hints");
    size = info->size;
    min = max = 0;
  }

  if (pool == NULL) {
    /* we did not get a pool, make one ourselves then */
    pool = gst_video_buffer_pool_new ();
  }

  if (depay->pool)
    gst_object_unref (depay->pool);
  depay->pool = pool;

  config = gst_buffer_pool_get_config (pool);
  gst_buffer_pool_config_set_params (config, caps, size, min, max);
  if (gst_query_find_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL)) {
    /* just set the metadata, if the pool can support it we will transparently use
     * it through the video info API. We could also see if the pool support this
     * metadata and only activate it then. */
    gst_buffer_pool_config_add_option (config,
        GST_BUFFER_POOL_OPTION_VIDEO_META);
  }

  gst_buffer_pool_set_config (pool, config);
  /* and activate */
  gst_buffer_pool_set_active (pool, TRUE);

  gst_query_unref (query);

  return GST_FLOW_OK;
}

static gboolean
gst_rtp_vraw_depay_setcaps (GstRTPBaseDepayload * depayload, GstCaps * caps)
{
  GstStructure *structure;
  GstRtpVRawDepay *rtpvrawdepay;
  gint clock_rate;
  const gchar *str;
  gint format, width, height, depth, pgroup, xinc, yinc;
  GstCaps *srccaps;
  gboolean res;
  GstFlowReturn ret;

  rtpvrawdepay = GST_RTP_VRAW_DEPAY (depayload);

  structure = gst_caps_get_structure (caps, 0);

  xinc = yinc = 1;

  if (!gst_structure_get_int (structure, "clock-rate", &clock_rate))
    clock_rate = 90000;         /* default */
  depayload->clock_rate = clock_rate;

  if (!(str = gst_structure_get_string (structure, "width")))
    goto no_width;
  width = atoi (str);

  if (!(str = gst_structure_get_string (structure, "height")))
    goto no_height;
  height = atoi (str);

  if (!(str = gst_structure_get_string (structure, "depth")))
    goto no_depth;
  depth = atoi (str);

  /* optional interlace value but we don't handle interlaced
   * formats yet */
  if (gst_structure_get_string (structure, "interlace"))
    goto interlaced;

  if (!(str = gst_structure_get_string (structure, "sampling")))
    goto no_sampling;

  if (!strcmp (str, "RGB")) {
    format = GST_VIDEO_FORMAT_RGB;
    pgroup = 3;
  } else if (!strcmp (str, "RGBA")) {
    format = GST_VIDEO_FORMAT_RGBA;
    pgroup = 4;
  } else if (!strcmp (str, "BGR")) {
    format = GST_VIDEO_FORMAT_BGR;
    pgroup = 3;
  } else if (!strcmp (str, "BGRA")) {
    format = GST_VIDEO_FORMAT_BGRA;
    pgroup = 4;
  } else if (!strcmp (str, "YCbCr-4:4:4")) {
    format = GST_VIDEO_FORMAT_AYUV;
    pgroup = 3;
  } else if (!strcmp (str, "YCbCr-4:2:2")) {
    if (depth == 8) {
      format = GST_VIDEO_FORMAT_UYVY;
      pgroup = 4;
    } else if (depth == 10) {
      format = GST_VIDEO_FORMAT_UYVP;
      pgroup = 5;
    } else
      goto unknown_format;
    xinc = 2;
  } else if (!strcmp (str, "YCbCr-4:2:0")) {
    format = GST_VIDEO_FORMAT_I420;
    pgroup = 6;
    xinc = yinc = 2;
  } else if (!strcmp (str, "YCbCr-4:1:1")) {
    format = GST_VIDEO_FORMAT_Y41B;
    pgroup = 6;
    xinc = 4;
  } else {
    goto unknown_format;
  }

  gst_video_info_init (&rtpvrawdepay->vinfo);
  gst_video_info_set_format (&rtpvrawdepay->vinfo, format, width, height);
  GST_VIDEO_INFO_FPS_N (&rtpvrawdepay->vinfo) = 0;
  GST_VIDEO_INFO_FPS_D (&rtpvrawdepay->vinfo) = 1;

  rtpvrawdepay->pgroup = pgroup;
  rtpvrawdepay->xinc = xinc;
  rtpvrawdepay->yinc = yinc;

  srccaps = gst_video_info_to_caps (&rtpvrawdepay->vinfo);
  res = gst_pad_set_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload), srccaps);
  gst_caps_unref (srccaps);

  GST_DEBUG_OBJECT (depayload, "width %d, height %d, format %d", width, height,
      format);
  GST_DEBUG_OBJECT (depayload, "xinc %d, yinc %d, pgroup %d",
      xinc, yinc, pgroup);

  /* negotiate a bufferpool */
  if ((ret = gst_rtp_vraw_depay_negotiate_pool (rtpvrawdepay, srccaps,
              &rtpvrawdepay->vinfo)) != GST_FLOW_OK)
    goto no_bufferpool;

  return res;

  /* ERRORS */
no_width:
  {
    GST_ERROR_OBJECT (depayload, "no width specified");
    return FALSE;
  }
no_height:
  {
    GST_ERROR_OBJECT (depayload, "no height specified");
    return FALSE;
  }
no_depth:
  {
    GST_ERROR_OBJECT (depayload, "no depth specified");
    return FALSE;
  }
interlaced:
  {
    GST_ERROR_OBJECT (depayload, "interlaced formats not supported yet");
    return FALSE;
  }
no_sampling:
  {
    GST_ERROR_OBJECT (depayload, "no sampling specified");
    return FALSE;
  }
unknown_format:
  {
    GST_ERROR_OBJECT (depayload, "unknown sampling format '%s'", str);
    return FALSE;
  }
no_bufferpool:
  {
    GST_DEBUG_OBJECT (depayload, "no bufferpool");
    return FALSE;
  }
}

static GstBuffer *
gst_rtp_vraw_depay_process_packet (GstRTPBaseDepayload * depayload,
    GstRTPBuffer * rtp)
{
  GstRtpVRawDepay *rtpvrawdepay;
  guint8 *payload, *p0, *yp, *up, *vp, *headers;
  guint32 timestamp;
  guint cont, ystride, uvstride, pgroup, payload_len;
  gint width, height, xinc, yinc;
  GstVideoFrame *frame;
  gboolean marker;
  GstBuffer *outbuf = NULL;

  rtpvrawdepay = GST_RTP_VRAW_DEPAY (depayload);

  timestamp = gst_rtp_buffer_get_timestamp (rtp);

  if (timestamp != rtpvrawdepay->timestamp || rtpvrawdepay->outbuf == NULL) {
    GstBuffer *new_buffer;
    GstFlowReturn ret;

    GST_LOG_OBJECT (depayload, "new frame with timestamp %u", timestamp);
    /* new timestamp, flush old buffer and create new output buffer */
    if (rtpvrawdepay->outbuf) {
      gst_video_frame_unmap (&rtpvrawdepay->frame);
      gst_rtp_base_depayload_push (depayload, rtpvrawdepay->outbuf);
      rtpvrawdepay->outbuf = NULL;
    }

    if (gst_pad_check_reconfigure (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload))) {
      GstCaps *caps;

      caps =
          gst_pad_get_current_caps (GST_RTP_BASE_DEPAYLOAD_SRCPAD (depayload));
      gst_rtp_vraw_depay_negotiate_pool (rtpvrawdepay, caps,
          &rtpvrawdepay->vinfo);
      gst_caps_unref (caps);
    }

    ret =
        gst_buffer_pool_acquire_buffer (rtpvrawdepay->pool, &new_buffer, NULL);

    if (G_UNLIKELY (ret != GST_FLOW_OK))
      goto alloc_failed;

    /* clear timestamp from alloc... */
    GST_BUFFER_PTS (new_buffer) = -1;

    if (!gst_video_frame_map (&rtpvrawdepay->frame, &rtpvrawdepay->vinfo,
            new_buffer, GST_MAP_WRITE | GST_VIDEO_FRAME_MAP_FLAG_NO_REF)) {
      gst_buffer_unref (new_buffer);
      goto invalid_frame;
    }

    rtpvrawdepay->outbuf = new_buffer;
    rtpvrawdepay->timestamp = timestamp;
  }

  frame = &rtpvrawdepay->frame;

  g_assert (frame->buffer != NULL);

  /* get pointer and strides of the planes */
  p0 = GST_VIDEO_FRAME_PLANE_DATA (frame, 0);
  yp = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  up = GST_VIDEO_FRAME_COMP_DATA (frame, 1);
  vp = GST_VIDEO_FRAME_COMP_DATA (frame, 2);

  ystride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  uvstride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 1);

  pgroup = rtpvrawdepay->pgroup;
  width = GST_VIDEO_INFO_WIDTH (&rtpvrawdepay->vinfo);
  height = GST_VIDEO_INFO_HEIGHT (&rtpvrawdepay->vinfo);
  xinc = rtpvrawdepay->xinc;
  yinc = rtpvrawdepay->yinc;

  payload = gst_rtp_buffer_get_payload (rtp);
  payload_len = gst_rtp_buffer_get_payload_len (rtp);

  if (payload_len < 3)
    goto short_packet;

  /* skip extended seqnum */
  payload += 2;
  payload_len -= 2;

  /* remember header position */
  headers = payload;

  gst_rtp_copy_video_meta (rtpvrawdepay, frame->buffer, rtp->buffer);

  /* find data start */
  do {
    if (payload_len < 6)
      goto short_packet;

    cont = payload[4] & 0x80;

    payload += 6;
    payload_len -= 6;
  } while (cont);

  while (TRUE) {
    guint length, line, offs, plen;
    guint8 *datap;

    /* stop when we run out of data */
    if (payload_len == 0)
      break;

    /* read length and cont. This should work because we iterated the headers
     * above. */
    length = (headers[0] << 8) | headers[1];
    line = ((headers[2] & 0x7f) << 8) | headers[3];
    offs = ((headers[4] & 0x7f) << 8) | headers[5];
    cont = headers[4] & 0x80;
    headers += 6;

    /* length must be a multiple of pgroup */
    if (length % pgroup != 0)
      goto wrong_length;

    if (length > payload_len)
      length = payload_len;

    /* sanity check */
    if (line > (height - yinc)) {
      GST_WARNING_OBJECT (depayload, "skipping line %d: out of range", line);
      goto next;
    }
    if (offs > (width - xinc)) {
      GST_WARNING_OBJECT (depayload, "skipping offset %d: out of range", offs);
      goto next;
    }

    /* calculate the maximum amount of bytes we can use per line */
    if (offs + ((length / pgroup) * xinc) > width) {
      plen = ((width - offs) * pgroup) / xinc;
      GST_WARNING_OBJECT (depayload, "clipping length %d, offset %d, plen %d",
          length, offs, plen);
    } else
      plen = length;

    GST_LOG_OBJECT (depayload,
        "writing length %u/%u, line %u, offset %u, remaining %u", plen, length,
        line, offs, payload_len);

    switch (GST_VIDEO_INFO_FORMAT (&rtpvrawdepay->vinfo)) {
      case GST_VIDEO_FORMAT_RGB:
      case GST_VIDEO_FORMAT_RGBA:
      case GST_VIDEO_FORMAT_BGR:
      case GST_VIDEO_FORMAT_BGRA:
      case GST_VIDEO_FORMAT_UYVY:
      case GST_VIDEO_FORMAT_UYVP:
        /* samples are packed just like gstreamer packs them */
        offs /= xinc;
        datap = p0 + (line * ystride) + (offs * pgroup);

        memcpy (datap, payload, plen);
        break;
      case GST_VIDEO_FORMAT_AYUV:
      {
        gint i;
        guint8 *p;

        datap = p0 + (line * ystride) + (offs * 4);
        p = payload;

        /* samples are packed in order Cb-Y-Cr for both interlaced and
         * progressive frames */
        for (i = 0; i < plen; i += pgroup) {
          *datap++ = 0;
          *datap++ = p[1];
          *datap++ = p[0];
          *datap++ = p[2];
          p += pgroup;
        }
        break;
      }
      case GST_VIDEO_FORMAT_I420:
      {
        gint i;
        guint uvoff;
        guint8 *yd1p, *yd2p, *udp, *vdp, *p;

        yd1p = yp + (line * ystride) + (offs);
        yd2p = yd1p + ystride;
        uvoff = (line / yinc * uvstride) + (offs / xinc);

        udp = up + uvoff;
        vdp = vp + uvoff;
        p = payload;

        /* line 0/1: Y00-Y01-Y10-Y11-Cb00-Cr00 Y02-Y03-Y12-Y13-Cb01-Cr01 ...  */
        for (i = 0; i < plen; i += pgroup) {
          *yd1p++ = p[0];
          *yd1p++ = p[1];
          *yd2p++ = p[2];
          *yd2p++ = p[3];
          *udp++ = p[4];
          *vdp++ = p[5];
          p += pgroup;
        }
        break;
      }
      case GST_VIDEO_FORMAT_Y41B:
      {
        gint i;
        guint uvoff;
        guint8 *ydp, *udp, *vdp, *p;

        ydp = yp + (line * ystride) + (offs);
        uvoff = (line / yinc * uvstride) + (offs / xinc);

        udp = up + uvoff;
        vdp = vp + uvoff;
        p = payload;

        /* Samples are packed in order Cb0-Y0-Y1-Cr0-Y2-Y3 for both interlaced
         * and progressive scan lines */
        for (i = 0; i < plen; i += pgroup) {
          *udp++ = p[0];
          *ydp++ = p[1];
          *ydp++ = p[2];
          *vdp++ = p[3];
          *ydp++ = p[4];
          *ydp++ = p[5];
          p += pgroup;
        }
        break;
      }
      default:
        goto unknown_sampling;
    }

  next:
    if (!cont)
      break;

    payload += length;
    payload_len -= length;
  }

  marker = gst_rtp_buffer_get_marker (rtp);

  if (marker) {
    GST_LOG_OBJECT (depayload, "marker, flushing frame");
    gst_video_frame_unmap (&rtpvrawdepay->frame);
    outbuf = rtpvrawdepay->outbuf;
    rtpvrawdepay->outbuf = NULL;
    rtpvrawdepay->timestamp = -1;
  }
  return outbuf;

  /* ERRORS */
unknown_sampling:
  {
    GST_ELEMENT_ERROR (depayload, STREAM, FORMAT,
        (NULL), ("unimplemented sampling"));
    return NULL;
  }
alloc_failed:
  {
    GST_WARNING_OBJECT (depayload, "failed to alloc output buffer");
    return NULL;
  }
invalid_frame:
  {
    GST_ERROR_OBJECT (depayload, "could not map video frame");
    return NULL;
  }
wrong_length:
  {
    GST_WARNING_OBJECT (depayload, "length not multiple of pgroup");
    return NULL;
  }
short_packet:
  {
    GST_WARNING_OBJECT (depayload, "short packet");
    return NULL;
  }
}

static gboolean
gst_rtp_vraw_depay_handle_event (GstRTPBaseDepayload * filter, GstEvent * event)
{
  gboolean ret;
  GstRtpVRawDepay *rtpvrawdepay;

  rtpvrawdepay = GST_RTP_VRAW_DEPAY (filter);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_FLUSH_STOP:
      gst_rtp_vraw_depay_reset (rtpvrawdepay, FALSE);
      break;
    default:
      break;
  }

  ret =
      GST_RTP_BASE_DEPAYLOAD_CLASS (parent_class)->handle_event (filter, event);

  return ret;
}

static GstStateChangeReturn
gst_rtp_vraw_depay_change_state (GstElement * element,
    GstStateChange transition)
{
  GstRtpVRawDepay *rtpvrawdepay;
  GstStateChangeReturn ret;

  rtpvrawdepay = GST_RTP_VRAW_DEPAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_rtp_vraw_depay_reset (rtpvrawdepay, TRUE);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_rtp_vraw_depay_reset (rtpvrawdepay, TRUE);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_rtp_vraw_depay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvrawdepay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_VRAW_DEPAY);
}
