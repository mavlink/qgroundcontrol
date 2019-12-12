/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2006> Mark Nauwelaerts <mnauw@users.sourceforge.net>
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
/**
 * SECTION:element-y4menc
 * @title: y4menc
 *
 * Creates a YU4MPEG2 raw video stream as defined by the mjpegtools project.
 *
 * ## Example launch line
 *
 * (write everything in one line, without the backslash characters)
 * |[
 * gst-launch-1.0 videotestsrc num-buffers=250 \
 * ! 'video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)25/1' \
 * ! y4menc ! filesink location=test.yuv
 * ]|
 *
 */

/* see mjpegtools/yuv4mpeg.h for yuv4mpeg format */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "gsty4mencode.h"

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

static GstStaticPadTemplate y4mencode_src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-yuv4mpeg, " "y4mversion = (int) 2")
    );

static GstStaticPadTemplate y4mencode_sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ IYUV, I420, Y42B, Y41B, Y444 }"))
    );


static void gst_y4m_encode_reset (GstY4mEncode * filter);

static GstStateChangeReturn gst_y4m_encode_change_state (GstElement * element,
    GstStateChange transition);

static GstFlowReturn
gst_y4m_encode_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);
static gboolean gst_y4m_encode_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);

#define gst_y4m_encode_parent_class parent_class
G_DEFINE_TYPE (GstY4mEncode, gst_y4m_encode, GST_TYPE_VIDEO_ENCODER);


static void
gst_y4m_encode_class_init (GstY4mEncodeClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstVideoEncoderClass *venc_class = GST_VIDEO_ENCODER_CLASS (klass);

  element_class->change_state = GST_DEBUG_FUNCPTR (gst_y4m_encode_change_state);

  gst_element_class_add_static_pad_template (element_class,
      &y4mencode_src_factory);
  gst_element_class_add_static_pad_template (element_class,
      &y4mencode_sink_factory);

  gst_element_class_set_static_metadata (element_class,
      "YUV4MPEG video encoder", "Codec/Encoder/Video",
      "Encodes a YUV frame into the yuv4mpeg format (mjpegtools)",
      "Wim Taymans <wim.taymans@gmail.com>");
  venc_class->set_format = gst_y4m_encode_set_format;
  venc_class->handle_frame = gst_y4m_encode_handle_frame;

}

static void
gst_y4m_encode_init (GstY4mEncode * filter)
{
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (filter));

  /* init properties */
  gst_y4m_encode_reset (filter);
}

static void
gst_y4m_encode_reset (GstY4mEncode * filter)
{
  filter->header = FALSE;
}

static gboolean
gst_y4m_encode_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  GstY4mEncode *y4menc;
  GstVideoInfo *info;
  GstVideoCodecState *output_state;

  y4menc = GST_Y4M_ENCODE (encoder);
  info = &state->info;

  switch (GST_VIDEO_INFO_FORMAT (info)) {
    case GST_VIDEO_FORMAT_I420:
      y4menc->colorspace = "420";
      break;
    case GST_VIDEO_FORMAT_Y42B:
      y4menc->colorspace = "422";
      break;
    case GST_VIDEO_FORMAT_Y41B:
      y4menc->colorspace = "411";
      break;
    case GST_VIDEO_FORMAT_Y444:
      y4menc->colorspace = "444";
      break;
    default:
      goto invalid_format;
  }

  y4menc->info = *info;

  output_state =
      gst_video_encoder_set_output_state (encoder,
      gst_static_pad_template_get_caps (&y4mencode_src_factory), state);
  gst_video_codec_state_unref (output_state);

  return TRUE;

invalid_format:
  {
    GST_ERROR_OBJECT (y4menc, "Invalid format");
    return FALSE;
  }

}

static inline GstBuffer *
gst_y4m_encode_get_stream_header (GstY4mEncode * filter, gboolean tff)
{
  gpointer header;
  GstBuffer *buf;
  gchar interlaced;
  gsize len;

  if (GST_VIDEO_INFO_IS_INTERLACED (&filter->info)) {
    if (tff)
      interlaced = 't';
    else
      interlaced = 'b';
  } else {
    interlaced = 'p';
  }

  header = g_strdup_printf ("YUV4MPEG2 C%s W%d H%d I%c F%d:%d A%d:%d\n",
      filter->colorspace, GST_VIDEO_INFO_WIDTH (&filter->info),
      GST_VIDEO_INFO_HEIGHT (&filter->info), interlaced,
      GST_VIDEO_INFO_FPS_N (&filter->info),
      GST_VIDEO_INFO_FPS_D (&filter->info),
      GST_VIDEO_INFO_PAR_N (&filter->info),
      GST_VIDEO_INFO_PAR_D (&filter->info));
  len = strlen (header);

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (0, header, len, 0, len, header, g_free));

  return buf;
}

static inline GstBuffer *
gst_y4m_encode_get_frame_header (GstY4mEncode * filter)
{
  gpointer header;
  GstBuffer *buf;
  gsize len;

  header = g_strdup_printf ("FRAME\n");
  len = strlen (header);

  buf = gst_buffer_new ();
  gst_buffer_append_memory (buf,
      gst_memory_new_wrapped (0, header, len, 0, len, header, g_free));

  return buf;
}


static GstFlowReturn
gst_y4m_encode_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstY4mEncode *filter = GST_Y4M_ENCODE (encoder);
  GstClockTime timestamp;

  /* check we got some decent info from caps */
  if (GST_VIDEO_INFO_FORMAT (&filter->info) == GST_VIDEO_FORMAT_UNKNOWN)
    goto not_negotiated;

  timestamp = GST_BUFFER_TIMESTAMP (frame->input_buffer);

  if (G_UNLIKELY (!filter->header)) {
    gboolean tff = FALSE;

    if (GST_VIDEO_INFO_IS_INTERLACED (&filter->info)) {
      tff =
          GST_BUFFER_FLAG_IS_SET (frame->input_buffer,
          GST_VIDEO_BUFFER_FLAG_TFF);
    }
    frame->output_buffer = gst_y4m_encode_get_stream_header (filter, tff);
    filter->header = TRUE;
    frame->output_buffer =
        gst_buffer_append (frame->output_buffer,
        gst_y4m_encode_get_frame_header (filter));
  } else {
    frame->output_buffer = gst_y4m_encode_get_frame_header (filter);
  }

  frame->output_buffer =
      gst_buffer_append (frame->output_buffer,
      gst_buffer_copy (frame->input_buffer));

  /* decorate */
  frame->output_buffer = gst_buffer_make_writable (frame->output_buffer);
  GST_BUFFER_TIMESTAMP (frame->output_buffer) = timestamp;

  return gst_video_encoder_finish_frame (encoder, frame);

not_negotiated:
  {
    GST_ELEMENT_ERROR (filter, CORE, NEGOTIATION, (NULL),
        ("format wasn't negotiated"));

    return GST_FLOW_NOT_NEGOTIATED;
  }
}

static GstStateChangeReturn
gst_y4m_encode_change_state (GstElement * element, GstStateChange transition)
{
  GstY4mEncode *filter = GST_Y4M_ENCODE (element);
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    default:
      break;
  }

  ret = GST_CALL_PARENT_WITH_DEFAULT (GST_ELEMENT_CLASS, change_state,
      (element, transition), GST_STATE_CHANGE_SUCCESS);
  if (ret != GST_STATE_CHANGE_SUCCESS)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_y4m_encode_reset (filter);
      break;
    default:
      break;
  }

  return GST_STATE_CHANGE_SUCCESS;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "y4menc", GST_RANK_PRIMARY,
      GST_TYPE_Y4M_ENCODE);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    y4menc,
    "Encodes a YUV frame into the yuv4mpeg format (mjpegtools)",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
