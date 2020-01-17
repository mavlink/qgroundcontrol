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

#include <string.h>

#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>

#include "gstrtpvrawpay.h"
#include "gstrtputils.h"

enum
{
  PROP_CHUNKS_PER_FRAME = 1
};

#define DEFAULT_CHUNKS_PER_FRAME 10

GST_DEBUG_CATEGORY_STATIC (rtpvrawpay_debug);
#define GST_CAT_DEFAULT (rtpvrawpay_debug)

static GstStaticPadTemplate gst_rtp_vraw_pay_sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) { RGB, RGBA, BGR, BGRA, AYUV, UYVY, I420, Y41B, UYVP }, "
        "width = (int) [ 1, 32767 ], " "height = (int) [ 1, 32767 ]; ")
    );

static GstStaticPadTemplate gst_rtp_vraw_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "media = (string) \"video\", "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ", "
        "clock-rate = (int) 90000, "
        "encoding-name = (string) \"RAW\","
        "sampling = (string) { \"RGB\", \"RGBA\", \"BGR\", \"BGRA\", "
        "\"YCbCr-4:4:4\", \"YCbCr-4:2:2\", \"YCbCr-4:2:0\", "
        "\"YCbCr-4:1:1\" },"
        /* we cannot express these as strings 
         * "width = (string) [1 32767],"
         * "height = (string) [1 32767],"
         */
        "depth = (string) { \"8\", \"10\", \"12\", \"16\" },"
        "colorimetry = (string) { \"BT601-5\", \"BT709-2\", \"SMPTE240M\" }"
        /* optional 
         * interlace = 
         * top-field-first = 
         * chroma-position = (string) 
         * gamma = (float)
         */
    )
    );

static gboolean gst_rtp_vraw_pay_setcaps (GstRTPBasePayload * payload,
    GstCaps * caps);
static GstFlowReturn gst_rtp_vraw_pay_handle_buffer (GstRTPBasePayload *
    payload, GstBuffer * buffer);
static void gst_rtp_vraw_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_vraw_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

G_DEFINE_TYPE (GstRtpVRawPay, gst_rtp_vraw_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static void
gst_rtp_vraw_pay_class_init (GstRtpVRawPayClass * klass)
{
  GstRTPBasePayloadClass *gstrtpbasepayload_class;
  GstElementClass *gstelement_class;
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstrtpbasepayload_class = (GstRTPBasePayloadClass *) klass;

  gobject_class->set_property = gst_rtp_vraw_pay_set_property;
  gobject_class->get_property = gst_rtp_vraw_pay_get_property;

  g_object_class_install_property (gobject_class,
      PROP_CHUNKS_PER_FRAME,
      g_param_spec_int ("chunks-per-frame", "Chunks per Frame",
          "Split and send out each frame in multiple chunks to reduce overhead",
          1, G_MAXINT, DEFAULT_CHUNKS_PER_FRAME,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
      );

  gstrtpbasepayload_class->set_caps = gst_rtp_vraw_pay_setcaps;
  gstrtpbasepayload_class->handle_buffer = gst_rtp_vraw_pay_handle_buffer;

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vraw_pay_src_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_rtp_vraw_pay_sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "RTP Raw Video payloader", "Codec/Payloader/Network/RTP",
      "Payload raw video as RTP packets (RFC 4175)",
      "Wim Taymans <wim.taymans@gmail.com>");

  GST_DEBUG_CATEGORY_INIT (rtpvrawpay_debug, "rtpvrawpay", 0,
      "Raw video RTP Payloader");
}

static void
gst_rtp_vraw_pay_init (GstRtpVRawPay * rtpvrawpay)
{
  rtpvrawpay->chunks_per_frame = DEFAULT_CHUNKS_PER_FRAME;
}

static gboolean
gst_rtp_vraw_pay_setcaps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstRtpVRawPay *rtpvrawpay;
  gboolean res;
  gint pgroup, xinc, yinc;
  const gchar *depthstr, *samplingstr, *colorimetrystr;
  gchar *wstr, *hstr;
  GstVideoInfo info;

  rtpvrawpay = GST_RTP_VRAW_PAY (payload);

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_caps;

  rtpvrawpay->vinfo = info;

  if (gst_video_colorimetry_matches (&info.colorimetry,
          GST_VIDEO_COLORIMETRY_BT601)) {
    colorimetrystr = "BT601-5";
  } else if (gst_video_colorimetry_matches (&info.colorimetry,
          GST_VIDEO_COLORIMETRY_BT709)) {
    colorimetrystr = "BT709-2";
  } else if (gst_video_colorimetry_matches (&info.colorimetry,
          GST_VIDEO_COLORIMETRY_SMPTE240M)) {
    colorimetrystr = "SMPTE240M";
  } else {
    colorimetrystr = "SMPTE240M";
  }

  xinc = yinc = 1;

  /* these values are the only thing we can do */
  depthstr = "8";

  switch (GST_VIDEO_INFO_FORMAT (&info)) {
    case GST_VIDEO_FORMAT_RGBA:
      samplingstr = "RGBA";
      pgroup = 4;
      break;
    case GST_VIDEO_FORMAT_BGRA:
      samplingstr = "BGRA";
      pgroup = 4;
      break;
    case GST_VIDEO_FORMAT_RGB:
      samplingstr = "RGB";
      pgroup = 3;
      break;
    case GST_VIDEO_FORMAT_BGR:
      samplingstr = "BGR";
      pgroup = 3;
      break;
    case GST_VIDEO_FORMAT_AYUV:
      samplingstr = "YCbCr-4:4:4";
      pgroup = 3;
      break;
    case GST_VIDEO_FORMAT_UYVY:
      samplingstr = "YCbCr-4:2:2";
      pgroup = 4;
      xinc = 2;
      break;
    case GST_VIDEO_FORMAT_Y41B:
      samplingstr = "YCbCr-4:1:1";
      pgroup = 6;
      xinc = 4;
      break;
    case GST_VIDEO_FORMAT_I420:
      samplingstr = "YCbCr-4:2:0";
      pgroup = 6;
      xinc = yinc = 2;
      break;
    case GST_VIDEO_FORMAT_UYVP:
      samplingstr = "YCbCr-4:2:2";
      pgroup = 5;
      xinc = 2;
      depthstr = "10";
      break;
    default:
      goto unknown_format;
      break;
  }

  if (GST_VIDEO_INFO_IS_INTERLACED (&info)) {
    yinc *= 2;
  }

  rtpvrawpay->pgroup = pgroup;
  rtpvrawpay->xinc = xinc;
  rtpvrawpay->yinc = yinc;

  GST_DEBUG_OBJECT (payload, "width %d, height %d, sampling %s",
      GST_VIDEO_INFO_WIDTH (&info), GST_VIDEO_INFO_HEIGHT (&info), samplingstr);
  GST_DEBUG_OBJECT (payload, "xinc %d, yinc %d, pgroup %d", xinc, yinc, pgroup);

  wstr = g_strdup_printf ("%d", GST_VIDEO_INFO_WIDTH (&info));
  hstr = g_strdup_printf ("%d", GST_VIDEO_INFO_HEIGHT (&info));

  gst_rtp_base_payload_set_options (payload, "video", TRUE, "RAW", 90000);
  if (GST_VIDEO_INFO_IS_INTERLACED (&info)) {
    res = gst_rtp_base_payload_set_outcaps (payload, "sampling", G_TYPE_STRING,
        samplingstr, "depth", G_TYPE_STRING, depthstr, "width", G_TYPE_STRING,
        wstr, "height", G_TYPE_STRING, hstr, "colorimetry", G_TYPE_STRING,
        colorimetrystr, "interlace", G_TYPE_STRING, "true", NULL);
  } else {
    res = gst_rtp_base_payload_set_outcaps (payload, "sampling", G_TYPE_STRING,
        samplingstr, "depth", G_TYPE_STRING, depthstr, "width", G_TYPE_STRING,
        wstr, "height", G_TYPE_STRING, hstr, "colorimetry", G_TYPE_STRING,
        colorimetrystr, NULL);
  }
  g_free (wstr);
  g_free (hstr);

  return res;

  /* ERRORS */
invalid_caps:
  {
    GST_ERROR_OBJECT (payload, "could not parse caps");
    return FALSE;
  }
unknown_format:
  {
    GST_ERROR_OBJECT (payload, "unknown caps format");
    return FALSE;
  }
}

static GstFlowReturn
gst_rtp_vraw_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buffer)
{
  GstRtpVRawPay *rtpvrawpay;
  GstFlowReturn ret = GST_FLOW_OK;
  gfloat packets_per_packline;
  guint pgroups_per_packet;
  guint packlines_per_list, buffers_per_list;
  guint lines_delay;            /* after how many packed lines we push out a buffer list */
  guint last_line;              /* last pack line number we pushed out a buffer list     */
  guint line, offset;
  guint8 *p0, *yp, *up, *vp;
  guint ystride, uvstride;
  guint xinc, yinc;
  guint pgroup;
  guint mtu;
  guint width, height;
  gint field, fields;
  GstVideoFormat format;
  GstVideoFrame frame;
  gint interlaced;
  gboolean use_buffer_lists;
  GstBufferList *list = NULL;
  GstRTPBuffer rtp = { NULL, };
  gboolean discont;

  rtpvrawpay = GST_RTP_VRAW_PAY (payload);

  if (!gst_video_frame_map (&frame, &rtpvrawpay->vinfo, buffer, GST_MAP_READ)) {
    gst_buffer_unref (buffer);
    return GST_FLOW_ERROR;
  }

  discont = GST_BUFFER_IS_DISCONT (buffer);

  GST_LOG_OBJECT (rtpvrawpay, "new frame of %" G_GSIZE_FORMAT " bytes",
      gst_buffer_get_size (buffer));

  /* get pointer and strides of the planes */
  p0 = GST_VIDEO_FRAME_PLANE_DATA (&frame, 0);
  yp = GST_VIDEO_FRAME_COMP_DATA (&frame, 0);
  up = GST_VIDEO_FRAME_COMP_DATA (&frame, 1);
  vp = GST_VIDEO_FRAME_COMP_DATA (&frame, 2);

  ystride = GST_VIDEO_FRAME_COMP_STRIDE (&frame, 0);
  uvstride = GST_VIDEO_FRAME_COMP_STRIDE (&frame, 1);

  mtu = GST_RTP_BASE_PAYLOAD_MTU (payload);

  /* amount of bytes for one pixel */
  pgroup = rtpvrawpay->pgroup;
  width = GST_VIDEO_INFO_WIDTH (&rtpvrawpay->vinfo);
  height = GST_VIDEO_INFO_HEIGHT (&rtpvrawpay->vinfo);

  interlaced = GST_VIDEO_INFO_IS_INTERLACED (&rtpvrawpay->vinfo);

  format = GST_VIDEO_INFO_FORMAT (&rtpvrawpay->vinfo);

  yinc = rtpvrawpay->yinc;
  xinc = rtpvrawpay->xinc;

  /* after how many packed lines we push out a buffer list */
  lines_delay = GST_ROUND_UP_4 (height / rtpvrawpay->chunks_per_frame);

  /* calculate how many buffers we expect to store in a single buffer list */
  pgroups_per_packet = (mtu - (12 + 14)) / pgroup;
  packets_per_packline = width / (xinc * pgroups_per_packet * 1.0);
  packlines_per_list = height / (yinc * rtpvrawpay->chunks_per_frame);
  buffers_per_list = packlines_per_list * packets_per_packline;
  buffers_per_list = GST_ROUND_UP_8 (buffers_per_list);

  use_buffer_lists = buffers_per_list > 1 &&
      (rtpvrawpay->chunks_per_frame < (height / yinc));

  fields = 1 + interlaced;

  /* start with line 0, offset 0 */
  for (field = 0; field < fields; field++) {
    line = field;
    offset = 0;
    last_line = 0;

    if (use_buffer_lists)
      list = gst_buffer_list_new_sized (buffers_per_list);

    /* write all lines */
    while (line < height) {
      guint left, pack_line;
      GstBuffer *out;
      guint8 *outdata, *headers;
      gboolean next_line, complete = FALSE;
      guint length, cont, pixels;

      /* get the max allowed payload length size, we try to fill the complete MTU */
      left = gst_rtp_buffer_calc_payload_len (mtu, 0, 0);
      out = gst_rtp_buffer_new_allocate (left, 0, 0);

      if (discont) {
        GST_BUFFER_FLAG_SET (out, GST_BUFFER_FLAG_DISCONT);
        /* Only the first outputted buffer has the DISCONT flag */
        discont = FALSE;
      }

      if (field == 0) {
        GST_BUFFER_PTS (out) = GST_BUFFER_PTS (buffer);
      } else {
        GST_BUFFER_PTS (out) = GST_BUFFER_PTS (buffer) +
            GST_BUFFER_DURATION (buffer) / 2;
      }

      gst_rtp_buffer_map (out, GST_MAP_WRITE, &rtp);
      outdata = gst_rtp_buffer_get_payload (&rtp);

      GST_LOG_OBJECT (rtpvrawpay, "created buffer of size %u for MTU %u", left,
          mtu);

      /*
       *   0                   1                   2                   3
       *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       *  |   Extended Sequence Number    |            Length             |
       *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       *  |F|          Line No            |C|           Offset            |
       *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       *  |            Length             |F|          Line No            |
       *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       *  |C|           Offset            |                               .
       *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               .
       *  .                                                               .
       *  .                 Two (partial) lines of video data             .
       *  .                                                               .
       *  +---------------------------------------------------------------+
       */

      /* need 2 bytes for the extended sequence number */
      *outdata++ = 0;
      *outdata++ = 0;
      left -= 2;

      /* the headers start here */
      headers = outdata;

      /* make sure we can fit at least *one* header and pixel */
      if (!(left > (6 + pgroup))) {
        gst_rtp_buffer_unmap (&rtp);
        gst_buffer_unref (out);
        goto too_small;
      }

      /* while we can fit at least one header and one pixel */
      while (left > (6 + pgroup)) {
        /* we need a 6 bytes header */
        left -= 6;

        /* get how may bytes we need for the remaining pixels */
        pixels = width - offset;
        length = (pixels * pgroup) / xinc;

        if (left >= length) {
          /* pixels and header fit completely, we will write them and skip to the
           * next line. */
          next_line = TRUE;
        } else {
          /* line does not fit completely, see how many pixels fit */
          pixels = (left / pgroup) * xinc;
          length = (pixels * pgroup) / xinc;
          next_line = FALSE;
        }
        GST_LOG_OBJECT (rtpvrawpay, "filling %u bytes in %u pixels", length,
            pixels);
        left -= length;

        /* write length */
        *outdata++ = (length >> 8) & 0xff;
        *outdata++ = length & 0xff;

        /* write line no */
        *outdata++ = ((line >> 8) & 0x7f) | ((field << 7) & 0x80);
        *outdata++ = line & 0xff;

        if (next_line) {
          /* go to next line we do this here to make the check below easier */
          line += yinc;
        }

        /* calculate continuation marker */
        cont = (left > (6 + pgroup) && line < height) ? 0x80 : 0x00;

        /* write offset and continuation marker */
        *outdata++ = ((offset >> 8) & 0x7f) | cont;
        *outdata++ = offset & 0xff;

        if (next_line) {
          /* reset offset */
          offset = 0;
          GST_LOG_OBJECT (rtpvrawpay, "go to next line %u", line);
        } else {
          offset += pixels;
          GST_LOG_OBJECT (rtpvrawpay, "next offset %u", offset);
        }

        if (!cont)
          break;
      }
      GST_LOG_OBJECT (rtpvrawpay, "consumed %u bytes",
          (guint) (outdata - headers));

      /* second pass, read headers and write the data */
      while (TRUE) {
        guint offs, lin;

        /* read length and cont */
        length = (headers[0] << 8) | headers[1];
        lin = ((headers[2] & 0x7f) << 8) | headers[3];
        offs = ((headers[4] & 0x7f) << 8) | headers[5];
        cont = headers[4] & 0x80;
        pixels = length / pgroup;
        headers += 6;

        GST_LOG_OBJECT (payload,
            "writing length %u, line %u, offset %u, cont %d", length, lin, offs,
            cont);

        switch (format) {
          case GST_VIDEO_FORMAT_RGB:
          case GST_VIDEO_FORMAT_RGBA:
          case GST_VIDEO_FORMAT_BGR:
          case GST_VIDEO_FORMAT_BGRA:
          case GST_VIDEO_FORMAT_UYVY:
          case GST_VIDEO_FORMAT_UYVP:
            offs /= xinc;
            memcpy (outdata, p0 + (lin * ystride) + (offs * pgroup), length);
            outdata += length;
            break;
          case GST_VIDEO_FORMAT_AYUV:
          {
            gint i;
            guint8 *datap;

            datap = p0 + (lin * ystride) + (offs * 4);

            for (i = 0; i < pixels; i++) {
              *outdata++ = datap[2];
              *outdata++ = datap[1];
              *outdata++ = datap[3];
              datap += 4;
            }
            break;
          }
          case GST_VIDEO_FORMAT_I420:
          {
            gint i;
            guint uvoff;
            guint8 *yd1p, *yd2p, *udp, *vdp;

            yd1p = yp + (lin * ystride) + (offs);
            yd2p = yd1p + ystride;
            uvoff = (lin / yinc * uvstride) + (offs / xinc);
            udp = up + uvoff;
            vdp = vp + uvoff;

            for (i = 0; i < pixels; i++) {
              *outdata++ = *yd1p++;
              *outdata++ = *yd1p++;
              *outdata++ = *yd2p++;
              *outdata++ = *yd2p++;
              *outdata++ = *udp++;
              *outdata++ = *vdp++;
            }
            break;
          }
          case GST_VIDEO_FORMAT_Y41B:
          {
            gint i;
            guint uvoff;
            guint8 *ydp, *udp, *vdp;

            ydp = yp + (lin * ystride) + offs;
            uvoff = (lin / yinc * uvstride) + (offs / xinc);
            udp = up + uvoff;
            vdp = vp + uvoff;

            for (i = 0; i < pixels; i++) {
              *outdata++ = *udp++;
              *outdata++ = *ydp++;
              *outdata++ = *ydp++;
              *outdata++ = *vdp++;
              *outdata++ = *ydp++;
              *outdata++ = *ydp++;
            }
            break;
          }
          default:
            gst_rtp_buffer_unmap (&rtp);
            gst_buffer_unref (out);
            goto unknown_sampling;
        }

        if (!cont)
          break;
      }

      if (line >= height) {
        GST_LOG_OBJECT (rtpvrawpay, "field/frame complete, set marker");
        gst_rtp_buffer_set_marker (&rtp, TRUE);
        complete = TRUE;
      }
      gst_rtp_buffer_unmap (&rtp);
      if (left > 0) {
        GST_LOG_OBJECT (rtpvrawpay, "we have %u bytes left", left);
        gst_buffer_resize (out, 0, gst_buffer_get_size (out) - left);
      }

      gst_rtp_copy_video_meta (rtpvrawpay, out, buffer);

      /* Now either push out the buffer directly */
      if (!use_buffer_lists) {
        ret = gst_rtp_base_payload_push (payload, out);
        continue;
      }

      /* or add the buffer to buffer list ... */
      gst_buffer_list_add (list, out);

      /* .. and check if we need to push out the list */
      pack_line = (line - field) / fields;
      if (complete || (pack_line > last_line && pack_line % lines_delay == 0)) {
        GST_LOG_OBJECT (rtpvrawpay, "pushing list of %u buffers up to pack "
            "line %u", gst_buffer_list_length (list), pack_line);
        ret = gst_rtp_base_payload_push_list (payload, list);
        list = NULL;
        if (!complete)
          list = gst_buffer_list_new_sized (buffers_per_list);
        last_line = pack_line;
      }
    }

  }

  gst_video_frame_unmap (&frame);
  gst_buffer_unref (buffer);

  return ret;

  /* ERRORS */
unknown_sampling:
  {
    GST_ELEMENT_ERROR (payload, STREAM, FORMAT,
        (NULL), ("unimplemented sampling"));
    gst_video_frame_unmap (&frame);
    gst_buffer_unref (buffer);
    return GST_FLOW_NOT_SUPPORTED;
  }
too_small:
  {
    GST_ELEMENT_ERROR (payload, RESOURCE, NO_SPACE_LEFT,
        (NULL), ("not enough space to send at least one pixel"));
    gst_video_frame_unmap (&frame);
    gst_buffer_unref (buffer);
    return GST_FLOW_NOT_SUPPORTED;
  }
}

static void
gst_rtp_vraw_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRtpVRawPay *rtpvrawpay;

  rtpvrawpay = GST_RTP_VRAW_PAY (object);

  switch (prop_id) {
    case PROP_CHUNKS_PER_FRAME:
      rtpvrawpay->chunks_per_frame = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_vraw_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRtpVRawPay *rtpvrawpay;

  rtpvrawpay = GST_RTP_VRAW_PAY (object);

  switch (prop_id) {
    case PROP_CHUNKS_PER_FRAME:
      g_value_set_int (value, rtpvrawpay->chunks_per_frame);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

gboolean
gst_rtp_vraw_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvrawpay",
      GST_RANK_SECONDARY, GST_TYPE_RTP_VRAW_PAY);
}
