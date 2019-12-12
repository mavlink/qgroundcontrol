/* GStreamer WebM muxer
 * Copyright (c) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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
 * SECTION:element-webmmux
 * @title: webmmux
 *
 * webmmux muxes VP8 video and Vorbis audio streams into a WebM file.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 webmmux name=mux ! filesink location=newfile.webm         \
 *   uridecodebin uri=file:///path/to/somefile.ogv name=demux                \
 *   demux. ! videoconvert ! vp8enc ! queue ! mux.video_0    \
 *   demux. ! progressreport ! audioconvert ! audiorate ! vorbisenc ! queue ! mux.audio_0
 * ]| This pipeline re-encodes a video file of any format into a WebM file.
 * |[
 * gst-launch-1.0 webmmux name=mux ! filesink location=test.webm            \
 *   videotestsrc num-buffers=250 ! video/x-raw,framerate=25/1 ! videoconvert ! vp8enc ! queue ! mux.video_0 \
 *   audiotestsrc samplesperbuffer=44100 num-buffers=10 ! audio/x-raw,rate=44100 ! vorbisenc ! queue ! mux.audio_0
 * ]| This pipeline muxes a test video and a sine wave into a WebM file.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "webm-mux.h"

#define COMMON_VIDEO_CAPS \
  "width = (int) [ 16, MAX ], " \
  "height = (int) [ 16, MAX ], " \
  "framerate = (fraction) [ 0, MAX ]"

#define COMMON_AUDIO_CAPS \
  "channels = (int) [ 1, MAX ], " \
  "rate = (int) [ 1, MAX ]"

G_DEFINE_TYPE (GstWebMMux, gst_webm_mux, GST_TYPE_MATROSKA_MUX);

static GstStaticPadTemplate webm_src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/webm; audio/webm")
    );

static GstStaticPadTemplate webm_videosink_templ =
    GST_STATIC_PAD_TEMPLATE ("video_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/x-vp8, " COMMON_VIDEO_CAPS ";"
        "video/x-vp9, " COMMON_VIDEO_CAPS ";" "video/x-av1, " COMMON_VIDEO_CAPS)
    );

static GstStaticPadTemplate webm_audiosink_templ =
    GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("audio/x-vorbis, " COMMON_AUDIO_CAPS ";"
        "audio/x-opus, " COMMON_AUDIO_CAPS)
    );

static void
gst_webm_mux_class_init (GstWebMMuxClass * klass)
{
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gst_element_class_add_static_pad_template (gstelement_class,
      &webm_videosink_templ);
  gst_element_class_add_static_pad_template (gstelement_class,
      &webm_audiosink_templ);
  gst_element_class_add_static_pad_template (gstelement_class, &webm_src_templ);
  gst_element_class_set_static_metadata (gstelement_class, "WebM muxer",
      "Codec/Muxer",
      "Muxes video and audio streams into a WebM stream",
      "GStreamer maintainers <gstreamer-devel@lists.freedesktop.org>");
}

static void
gst_webm_mux_init (GstWebMMux * mux)
{
  GST_MATROSKA_MUX (mux)->doctype = GST_MATROSKA_DOCTYPE_WEBM;
  GST_MATROSKA_MUX (mux)->is_webm = TRUE;
}
