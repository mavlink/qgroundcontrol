/* VP8
 * Copyright (C) 2006 David Schleef <ds@schleef.org>
 * Copyright (C) 2008,2009,2010 Entropy Wave Inc
 * Copyright (C) 2010-2012 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 *
 */
/**
 * SECTION:element-vp8dec
 * @title: vp8dec
 * @see_also: vp8enc, matroskademux
 *
 * This element decodes VP8 streams into raw video.
 * [VP8](http://www.webmproject.org) is a royalty-free video codec maintained by
 * [Google](http://www.google.com/). It's the successor of On2 VP3, which was
 * the base of the Theora video codec.
 *
 * ## Example pipeline
 * |[
 * gst-launch-1.0 -v filesrc location=videotestsrc.webm ! matroskademux ! vp8dec ! videoconvert ! videoscale ! autovideosink
 * ]| This example pipeline will decode a WebM stream and decodes the VP8 video.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_VP8_DECODER

#include <string.h>

#include "gstvp8dec.h"
#include "gstvp8utils.h"

#include <gst/video/gstvideometa.h>
#include <gst/video/gstvideopool.h>

GST_DEBUG_CATEGORY_STATIC (gst_vp8dec_debug);
#define GST_CAT_DEFAULT gst_vp8dec_debug

#define VP8_DECODER_VIDEO_TAG "VP8 video"

static void gst_vp8_dec_set_default_format (GstVPXDec * dec, GstVideoFormat fmt,
    int width, int height);
static void gst_vp8_dec_handle_resolution_change (GstVPXDec * dec,
    vpx_image_t * img, GstVideoFormat fmt);

static GstStaticPadTemplate gst_vp8_dec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-vp8")
    );

static GstStaticPadTemplate gst_vp8_dec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420"))
    );

#define parent_class gst_vp8_dec_parent_class
G_DEFINE_TYPE (GstVP8Dec, gst_vp8_dec, GST_TYPE_VPX_DEC);

static void
gst_vp8_dec_class_init (GstVP8DecClass * klass)
{
  GstElementClass *element_class;
  GstVPXDecClass *vpx_class;

  element_class = GST_ELEMENT_CLASS (klass);
  vpx_class = GST_VPX_DEC_CLASS (klass);

  gst_element_class_add_static_pad_template (element_class,
      &gst_vp8_dec_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_vp8_dec_src_template);

  gst_element_class_set_static_metadata (element_class,
      "On2 VP8 Decoder",
      "Codec/Decoder/Video",
      "Decode VP8 video streams", "David Schleef <ds@entropywave.com>, "
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  vpx_class->video_codec_tag = VP8_DECODER_VIDEO_TAG;
  vpx_class->codec_algo = &vpx_codec_vp8_dx_algo;
  vpx_class->set_default_format =
      GST_DEBUG_FUNCPTR (gst_vp8_dec_set_default_format);
  vpx_class->handle_resolution_change =
      GST_DEBUG_FUNCPTR (gst_vp8_dec_handle_resolution_change);

  GST_DEBUG_CATEGORY_INIT (gst_vp8dec_debug, "vp8dec", 0, "VP8 Decoder");
}

static void
gst_vp8_dec_init (GstVP8Dec * gst_vp8_dec)
{
  GST_DEBUG_OBJECT (gst_vp8_dec, "gst_vp8_dec_init");
}

static void
gst_vp8_dec_set_default_format (GstVPXDec * dec, GstVideoFormat fmt, int width,
    int height)
{
  GstVPXDecClass *vpxclass = GST_VPX_DEC_GET_CLASS (dec);
  g_assert (dec->output_state == NULL);
  dec->output_state =
      gst_video_decoder_set_output_state (GST_VIDEO_DECODER (dec),
      GST_VIDEO_FORMAT_I420, width, height, dec->input_state);
  gst_video_decoder_negotiate (GST_VIDEO_DECODER (dec));
  vpxclass->send_tags (dec);
}

static void
gst_vp8_dec_handle_resolution_change (GstVPXDec * dec, vpx_image_t * img,
    GstVideoFormat fmt)
{
  GstVideoInfo *info;
  GstVideoCodecState *new_output_state;

  info = &dec->output_state->info;
  if (GST_VIDEO_INFO_WIDTH (info) != img->d_w
      || GST_VIDEO_INFO_HEIGHT (info) != img->d_h) {
    GST_DEBUG_OBJECT (dec,
        "Changed output resolution was %d x %d now is got %u x %u (display %u x %u)",
        GST_VIDEO_INFO_WIDTH (info), GST_VIDEO_INFO_HEIGHT (info), img->w,
        img->h, img->d_w, img->d_h);

    new_output_state =
        gst_video_decoder_set_output_state (GST_VIDEO_DECODER (dec),
        GST_VIDEO_FORMAT_I420, img->d_w, img->d_h, dec->output_state);
    if (dec->output_state) {
      gst_video_codec_state_unref (dec->output_state);
    }
    dec->output_state = new_output_state;
    /* No need to call negotiate() here, it will be automatically called
     * by allocate_output_frame()*/
  }
}

#endif /* HAVE_VP8_DECODER */
