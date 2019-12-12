/*
 * gstrtpvp9pay.c - Source for GstRtpVP9Pay
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gst/base/gstbitreader.h>
#include <gst/rtp/gstrtppayloads.h>
#include <gst/rtp/gstrtpbuffer.h>
#include <gst/video/video.h>
#include "dboolhuff.h"
#include "gstrtpvp9pay.h"
#include "gstrtputils.h"

GST_DEBUG_CATEGORY_STATIC (gst_rtp_vp9_pay_debug);
#define GST_CAT_DEFAULT gst_rtp_vp9_pay_debug

#define DEFAULT_PICTURE_ID_MODE VP9_PAY_NO_PICTURE_ID

enum
{
  PROP_0,
  PROP_PICTURE_ID_MODE
};

#define GST_TYPE_RTP_VP9_PAY_PICTURE_ID_MODE (gst_rtp_vp9_pay_picture_id_mode_get_type())
static GType
gst_rtp_vp9_pay_picture_id_mode_get_type (void)
{
  static GType mode_type = 0;
  static const GEnumValue modes[] = {
    {VP9_PAY_NO_PICTURE_ID, "No Picture ID", "none"},
    {VP9_PAY_PICTURE_ID_7BITS, "7-bit Picture ID", "7-bit"},
    {VP9_PAY_PICTURE_ID_15BITS, "15-bit Picture ID", "15-bit"},
    {0, NULL, NULL},
  };

  if (!mode_type) {
    mode_type = g_enum_register_static ("GstVP9RTPPayMode", modes);
  }
  return mode_type;
}

static void gst_rtp_vp9_pay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_rtp_vp9_pay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static GstFlowReturn gst_rtp_vp9_pay_handle_buffer (GstRTPBasePayload * payload,
    GstBuffer * buffer);
static gboolean gst_rtp_vp9_pay_sink_event (GstRTPBasePayload * payload,
    GstEvent * event);
static gboolean gst_rtp_vp9_pay_set_caps (GstRTPBasePayload * payload,
    GstCaps * caps);

G_DEFINE_TYPE (GstRtpVP9Pay, gst_rtp_vp9_pay, GST_TYPE_RTP_BASE_PAYLOAD);

static GstStaticPadTemplate gst_rtp_vp9_pay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp, "
        "payload = (int) " GST_RTP_PAYLOAD_DYNAMIC_STRING ","
        "clock-rate = (int) 90000, encoding-name = (string) { \"VP9\", \"VP9-DRAFT-IETF-01\" }"));

static GstStaticPadTemplate gst_rtp_vp9_pay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp9"));

static void
gst_rtp_vp9_pay_init (GstRtpVP9Pay * obj)
{
  obj->picture_id_mode = DEFAULT_PICTURE_ID_MODE;
  if (obj->picture_id_mode == VP9_PAY_PICTURE_ID_7BITS)
    obj->picture_id = g_random_int_range (0, G_MAXUINT8) & 0x7F;
  else if (obj->picture_id_mode == VP9_PAY_PICTURE_ID_15BITS)
    obj->picture_id = g_random_int_range (0, G_MAXUINT16) & 0x7FFF;
}

static void
gst_rtp_vp9_pay_class_init (GstRtpVP9PayClass * gst_rtp_vp9_pay_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (gst_rtp_vp9_pay_class);
  GstElementClass *element_class = GST_ELEMENT_CLASS (gst_rtp_vp9_pay_class);
  GstRTPBasePayloadClass *pay_class =
      GST_RTP_BASE_PAYLOAD_CLASS (gst_rtp_vp9_pay_class);

  gobject_class->set_property = gst_rtp_vp9_pay_set_property;
  gobject_class->get_property = gst_rtp_vp9_pay_get_property;

  g_object_class_install_property (gobject_class, PROP_PICTURE_ID_MODE,
      g_param_spec_enum ("picture-id-mode", "Picture ID Mode",
          "The picture ID mode for payloading",
          GST_TYPE_RTP_VP9_PAY_PICTURE_ID_MODE, DEFAULT_PICTURE_ID_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp9_pay_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_rtp_vp9_pay_src_template);

  gst_element_class_set_static_metadata (element_class, "RTP VP9 payloader",
      "Codec/Payloader/Network/RTP",
      "Puts VP9 video in RTP packets)", "Stian Selnes <stian@pexip.com>");

  pay_class->handle_buffer = gst_rtp_vp9_pay_handle_buffer;
  pay_class->sink_event = gst_rtp_vp9_pay_sink_event;
  pay_class->set_caps = gst_rtp_vp9_pay_set_caps;

  GST_DEBUG_CATEGORY_INIT (gst_rtp_vp9_pay_debug, "rtpvp9pay", 0,
      "VP9 Video RTP Payloader");
}

static void
gst_rtp_vp9_pay_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstRtpVP9Pay *rtpvp9pay = GST_RTP_VP9_PAY (object);

  switch (prop_id) {
    case PROP_PICTURE_ID_MODE:
      rtpvp9pay->picture_id_mode = g_value_get_enum (value);
      if (rtpvp9pay->picture_id_mode == VP9_PAY_PICTURE_ID_7BITS)
        rtpvp9pay->picture_id = g_random_int_range (0, G_MAXUINT8) & 0x7F;
      else if (rtpvp9pay->picture_id_mode == VP9_PAY_PICTURE_ID_15BITS)
        rtpvp9pay->picture_id = g_random_int_range (0, G_MAXUINT16) & 0x7FFF;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_rtp_vp9_pay_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstRtpVP9Pay *rtpvp9pay = GST_RTP_VP9_PAY (object);

  switch (prop_id) {
    case PROP_PICTURE_ID_MODE:
      g_value_set_enum (value, rtpvp9pay->picture_id_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

#define VP9_PROFILE_0 0
#define VP9_PROFILE_1 1
#define VP9_PROFILE_2 2
#define VP9_PROFILE_3 3
#define VP9_FRAME_MARKER 0x2
#define VPX_CS_SRGB 7

static gboolean
gst_rtp_vp9_pay_parse_frame (GstRtpVP9Pay * self, GstBuffer * buffer,
    gsize buffer_size)
{
  GstMapInfo map = GST_MAP_INFO_INIT;
  GstBitReader reader;
  guint8 *data;
  gsize size;
  gboolean keyframe;
  guint32 tmp, profile;

  if (G_UNLIKELY (buffer_size < 3))
    goto error;

  if (!gst_buffer_map (buffer, &map, GST_MAP_READ) || !map.data)
    goto error;

  data = map.data;
  size = map.size;

  gst_bit_reader_init (&reader, data, size);


  /* frame marker */
  if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 2)
      || tmp != VP9_FRAME_MARKER)
    goto error;

  /* profile, variable length */
  if (!gst_bit_reader_get_bits_uint32 (&reader, &profile, 2))
    goto error;
  if (profile > 2) {
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 1))
      goto error;
    profile += tmp;
  }

  /* show existing frame */
  if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 1))
    goto error;
  if (tmp) {
    if (!gst_bit_reader_skip (&reader, 3))
      goto error;
    return TRUE;
  }

  /* frame type */
  if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 1))
    goto error;
  self->is_keyframe = keyframe = (tmp == 0);

  /* show frame and resilient mode */
  if (!gst_bit_reader_skip (&reader, 2))
    goto error;

  if (keyframe) {
    /* sync code */
    const guint32 sync_code = 0x498342;
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 24))
      goto error;
    if (tmp != sync_code)
      goto error;

    if (profile >= VP9_PROFILE_2) {
      /* bit depth */
      if (!gst_bit_reader_skip (&reader, 1))
        goto error;
    }

    /* color space */
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 3))
      goto error;
    if (tmp != VPX_CS_SRGB) {
      /* color range */
      if (!gst_bit_reader_skip (&reader, 1))
        goto error;
      if (profile == VP9_PROFILE_1 || profile == VP9_PROFILE_3) {
        /* subsampling + reserved bit */
        if (!gst_bit_reader_skip (&reader, 2 + 1))
          goto error;
      }
    } else {
      if (profile == VP9_PROFILE_1 || profile == VP9_PROFILE_3)
        /* reserved bit */
        if (!gst_bit_reader_skip (&reader, 1))
          goto error;
    }

    /* frame size */
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 16))
      goto error;
    self->width = tmp + 1;
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 16))
      goto error;
    self->height = tmp + 1;

    /* render size */
    if (!gst_bit_reader_get_bits_uint32 (&reader, &tmp, 1))
      goto error;
    if (tmp) {
      if (!gst_bit_reader_skip (&reader, 32))
        goto error;
    }

    GST_INFO_OBJECT (self, "parsed width=%d height=%d", self->width,
        self->height);
  }


  gst_buffer_unmap (buffer, &map);
  return TRUE;

error:
  GST_DEBUG ("Failed to parse frame");
  if (map.memory != NULL) {
    gst_buffer_unmap (buffer, &map);
  }
  return FALSE;
}

static gsize
gst_rtp_vp9_calc_header_len (GstRtpVP9Pay * self, gboolean start)
{
  gsize len = 1;

  switch (self->picture_id_mode) {
    case VP9_PAY_PICTURE_ID_7BITS:
      len += 1;
      break;
    case VP9_PAY_PICTURE_ID_15BITS:
      len += 2;
    default:
      break;
  }

  /* Assume non-flexible mode */
  /* Assume L-bit not set, no L header */

  if (self->is_keyframe && start) {
    /* Assume V-bit set */
    /* FIXME: SS depends on layers and prediction structure */
    /* For now assume 1 spatial and 1 temporal layer. */
    /* FIXME: Only for the first packet in the key frame */
    len += 8;
  }

  return len;
}

/* VP9 RTP header, non-flexible mode:

         0 1 2 3 4 5 6 7
        +-+-+-+-+-+-+-+-+
        |I|P|L|F|B|E|V|-| (REQUIRED)
        +-+-+-+-+-+-+-+-+
   I:   |M| PICTURE ID  | (RECOMMENDED)
        +-+-+-+-+-+-+-+-+
   M:   | EXTENDED PID  | (RECOMMENDED)
        +-+-+-+-+-+-+-+-+
   L:   |  T  |U|  S  |D| (CONDITIONALLY RECOMMENDED)
        +-+-+-+-+-+-+-+-+                             -\
   P,F: | P_DIFF    |X|N| (CONDITIONALLY RECOMMENDED)  .
        +-+-+-+-+-+-+-+-+                              . - up to 3 times
   X:   |EXTENDED P_DIFF| (OPTIONAL)                   .
        +-+-+-+-+-+-+-+-+                             -/
   V:   | SS            |
        | ..            |
        +-+-+-+-+-+-+-+-+

   Scalability structure (SS)
     (from https://chromium.googlesource.com/external/webrtc/+/HEAD/webrtc/modules/rtp_rtcp/source/rtp_format_vp9.cc
     since latest draft is not up to date with chromium)

        +-+-+-+-+-+-+-+-+
   V:   | N_S |Y|G|-|-|-|
        +-+-+-+-+-+-+-+-+              -|
   Y:   |     WIDTH     | (OPTIONAL)    .
        +               +               .
        |               | (OPTIONAL)    .
        +-+-+-+-+-+-+-+-+               . N_S + 1 times
        |     HEIGHT    | (OPTIONAL)    .
        +               +               .
        |               | (OPTIONAL)    .
        +-+-+-+-+-+-+-+-+              -|
   G:   |      N_G      | (OPTIONAL)
        +-+-+-+-+-+-+-+-+                           -|
   N_G: |  T  |U| R |-|-| (OPTIONAL)                 .
        +-+-+-+-+-+-+-+-+              -|            . N_G times
        |    P_DIFF     | (OPTIONAL)    . R times    .
        +-+-+-+-+-+-+-+-+              -|           -|

**/

/* When growing the vp9 header keep max payload len calculation in sync */
static GstBuffer *
gst_rtp_vp9_create_header_buffer (GstRtpVP9Pay * self,
    gboolean start, gboolean mark, GstBuffer * in)
{
  GstBuffer *out;
  guint8 *p;
  GstRTPBuffer rtpbuffer = GST_RTP_BUFFER_INIT;
  guint off = 1;
  guint hdrlen = gst_rtp_vp9_calc_header_len (self, start);

  out = gst_rtp_buffer_new_allocate (hdrlen, 0, 0);
  gst_rtp_buffer_map (out, GST_MAP_READWRITE, &rtpbuffer);
  p = gst_rtp_buffer_get_payload (&rtpbuffer);
  p[0] = 0x0;

  if (self->picture_id_mode != VP9_PAY_NO_PICTURE_ID) {
    p[0] |= 0x80;
    if (self->picture_id_mode == VP9_PAY_PICTURE_ID_7BITS) {
      /* M=0 */
      p[off++] = self->picture_id & 0x7F;
    } else {
      /* M=1 */
      p[off++] = 0x80 | ((self->picture_id & 0x7FFF) >> 8);
      p[off++] = self->picture_id & 0xFF;
    }
  }

  if (!self->is_keyframe)
    p[0] |= 0x40;
  if (start)
    p[0] |= 0x08;
  if (mark)
    p[0] |= 0x04;

  if (self->is_keyframe && start) {
    p[0] |= 0x02;
    /* scalability structure, hard coded for now to be similar to chromium for
     * quick and dirty interop */
    p[off++] = 0x18;            /* N_S=0 Y=1 G=1 */
    p[off++] = self->width >> 8;
    p[off++] = self->width & 0xFF;
    p[off++] = self->height >> 8;
    p[off++] = self->height & 0xFF;
    p[off++] = 0x01;            /* N_G=1 */
    p[off++] = 0x04;            /* T=0, U=0, R=1 */
    p[off++] = 0x01;            /* P_DIFF=1 */
  }

  g_assert_cmpint (off, ==, hdrlen);

  gst_rtp_buffer_set_marker (&rtpbuffer, mark);

  gst_rtp_buffer_unmap (&rtpbuffer);

  GST_BUFFER_DURATION (out) = GST_BUFFER_DURATION (in);
  GST_BUFFER_PTS (out) = GST_BUFFER_PTS (in);

  return out;
}

static guint
gst_rtp_vp9_payload_next (GstRtpVP9Pay * self, GstBufferList * list,
    guint offset, GstBuffer * buffer, gsize buffer_size, gsize max_payload_len)
{
  GstBuffer *header;
  GstBuffer *sub;
  GstBuffer *out;
  gboolean mark;
  gsize remaining;
  gsize available;

  remaining = buffer_size - offset;
  available = max_payload_len;
  if (available > remaining)
    available = remaining;

  mark = (remaining == available);
  header = gst_rtp_vp9_create_header_buffer (self, offset == 0, mark, buffer);
  sub = gst_buffer_copy_region (buffer, GST_BUFFER_COPY_ALL, offset, available);

  gst_rtp_copy_video_meta (self, header, buffer);

  out = gst_buffer_append (header, sub);

  gst_buffer_list_insert (list, -1, out);

  return available;
}


static GstFlowReturn
gst_rtp_vp9_pay_handle_buffer (GstRTPBasePayload * payload, GstBuffer * buffer)
{
  GstRtpVP9Pay *self = GST_RTP_VP9_PAY (payload);
  GstFlowReturn ret;
  GstBufferList *list;
  gsize size, max_paylen;
  guint offset, mtu, vp9_hdr_len;

  size = gst_buffer_get_size (buffer);

  if (G_UNLIKELY (!gst_rtp_vp9_pay_parse_frame (self, buffer, size))) {
    GST_ELEMENT_ERROR (self, STREAM, ENCODE, (NULL),
        ("Failed to parse VP9 frame"));
    return GST_FLOW_ERROR;
  }

  mtu = GST_RTP_BASE_PAYLOAD_MTU (payload);
  vp9_hdr_len = gst_rtp_vp9_calc_header_len (self, TRUE);
  max_paylen = gst_rtp_buffer_calc_payload_len (mtu - vp9_hdr_len, 0, 0);

  list = gst_buffer_list_new_sized ((size / max_paylen) + 1);

  offset = 0;
  while (offset < size) {
    offset +=
        gst_rtp_vp9_payload_next (self, list, offset, buffer, size, max_paylen);
  }

  ret = gst_rtp_base_payload_push_list (payload, list);

  /* Incremenent and wrap the picture id if it overflows */
  if ((self->picture_id_mode == VP9_PAY_PICTURE_ID_7BITS &&
          ++self->picture_id >= 0x80) ||
      (self->picture_id_mode == VP9_PAY_PICTURE_ID_15BITS &&
          ++self->picture_id >= 0x8000))
    self->picture_id = 0;

  gst_buffer_unref (buffer);

  return ret;
}

static gboolean
gst_rtp_vp9_pay_sink_event (GstRTPBasePayload * payload, GstEvent * event)
{
  GstRtpVP9Pay *self = GST_RTP_VP9_PAY (payload);

  if (GST_EVENT_TYPE (event) == GST_EVENT_FLUSH_START) {
    if (self->picture_id_mode == VP9_PAY_PICTURE_ID_7BITS)
      self->picture_id = g_random_int_range (0, G_MAXUINT8) & 0x7F;
    else if (self->picture_id_mode == VP9_PAY_PICTURE_ID_15BITS)
      self->picture_id = g_random_int_range (0, G_MAXUINT16) & 0x7FFF;
  }

  return GST_RTP_BASE_PAYLOAD_CLASS (gst_rtp_vp9_pay_parent_class)->sink_event
      (payload, event);
}

static gboolean
gst_rtp_vp9_pay_set_caps (GstRTPBasePayload * payload, GstCaps * caps)
{
  GstCaps *src_caps;
  const char *encoding_name = "VP9";

  src_caps = gst_pad_get_allowed_caps (GST_RTP_BASE_PAYLOAD_SRCPAD (payload));
  if (src_caps) {
    GstStructure *s;
    const GValue *value;

    s = gst_caps_get_structure (src_caps, 0);

    if (gst_structure_has_field (s, "encoding-name")) {
      GValue default_value = G_VALUE_INIT;

      g_value_init (&default_value, G_TYPE_STRING);
      g_value_set_static_string (&default_value, encoding_name);

      value = gst_structure_get_value (s, "encoding-name");
      if (!gst_value_can_intersect (&default_value, value))
        encoding_name = "VP9-DRAFT-IETF-01";
    }
  }

  gst_rtp_base_payload_set_options (payload, "video", TRUE,
      encoding_name, 90000);

  return gst_rtp_base_payload_set_outcaps (payload, NULL);
}

gboolean
gst_rtp_vp9_pay_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "rtpvp9pay",
      GST_RANK_MARGINAL, GST_TYPE_RTP_VP9_PAY);
}
