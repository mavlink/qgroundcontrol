/* AVI muxer plugin for GStreamer
 * Copyright (C) 2002 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *           (C) 2006 Mark Nauwelaerts <manauw@skynet.be>
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

/* based on:
 * - the old avimuxer (by Wim Taymans)
 * - xawtv's aviwriter (by Gerd Knorr)
 * - mjpegtools' avilib (by Rainer Johanni)
 * - openDML large-AVI docs
 */

/**
 * SECTION:element-avimux
 * @title: avimux
 *
 * Muxes raw or compressed audio and/or video streams into an AVI file.
 *
 * ## Example launch lines
 * (write everything in one line, without the backslash characters)
 * |[
 * gst-launch-1.0 videotestsrc num-buffers=250 \
 * ! 'video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)25/1' \
 * ! queue ! mux. \
 * audiotestsrc num-buffers=440 ! audioconvert \
 * ! 'audio/x-raw,rate=44100,channels=2' ! queue ! mux. \
 * avimux name=mux ! filesink location=test.avi
 * ]| This will create an .AVI file containing an uncompressed video stream
 * with a test picture and an uncompressed audio stream containing a
 * test sound.
 * |[
 * gst-launch-1.0 videotestsrc num-buffers=250 \
 * ! 'video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)25/1' \
 * ! xvidenc ! queue ! mux. \
 * audiotestsrc num-buffers=440 ! audioconvert ! 'audio/x-raw,rate=44100,channels=2' \
 * ! lame ! queue ! mux. \
 * avimux name=mux ! filesink location=test.avi
 * ]| This will create an .AVI file containing the same test video and sound
 * as above, only that both streams will be compressed this time. This will
 * only work if you have the necessary encoder elements installed of course.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gst/gst-i18n-plugin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gst/video/video.h>
#include <gst/audio/audio.h>
#include <gst/base/gstbytewriter.h>

#include "gstavimux.h"

GST_DEBUG_CATEGORY_STATIC (avimux_debug);
#define GST_CAT_DEFAULT avimux_debug

enum
{
  PROP_0,
  PROP_BIGFILE
};

#define DEFAULT_BIGFILE TRUE

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-msvideo")
    );

static GstStaticPadTemplate video_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("video_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("video/x-raw, "
        "format = (string) { YUY2, I420, BGR, BGRx, BGRA, GRAY8, UYVY }, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ]; "
        "image/jpeg, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ]; "
        "video/x-divx, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ], "
        "divxversion = (int) [ 3, 5 ]; "
        "video/x-msmpeg, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ], "
        "msmpegversion = (int) [ 41, 43 ]; "
        "video/mpeg, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ], "
        "mpegversion = (int) { 1, 2, 4}, "
        "systemstream = (boolean) FALSE; "
        "video/x-h263, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ]; "
        "video/x-h264, "
        "stream-format = (string) byte-stream, "
        "alignment = (string) au, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], "
        "framerate = (fraction) [ 0, MAX ]; "
        "video/x-dv, "
        "width = (int) 720, "
        "height = (int) { 576, 480 }, "
        "framerate = (fraction) [ 0, MAX ], "
        "systemstream = (boolean) FALSE; "
        "video/x-huffyuv, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], " "framerate = (fraction) [ 0, MAX ];"
        "video/x-wmv, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], " "framerate = (fraction) [ 0, MAX ], "
        "wmvversion = (int) [ 1, 3];"
        "image/x-jpc, "
        "width = (int) [ 1, 2147483647 ], "
        "height = (int) [ 1, 2147483647 ], "
        "framerate = (fraction) [ 0, MAX ];"
        "video/x-vp8, "
        "width = (int) [ 1, 2147483647 ], "
        "height = (int) [ 1, 2147483647 ], "
        "framerate = (fraction) [ 0, MAX ];"
        "image/png, "
        "width = (int) [ 16, 4096 ], "
        "height = (int) [ 16, 4096 ], framerate = (fraction) [ 0, MAX ]")
    );

static GstStaticPadTemplate audio_sink_factory =
    GST_STATIC_PAD_TEMPLATE ("audio_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) { U8, S16LE }, "
        "rate = (int) [ 1000, 96000 ], "
        "channels = (int) [ 1, 2 ]; "
        "audio/mpeg, "
        "mpegversion = (int) 1, "
        "layer = (int) [ 1, 3 ], "
        "rate = (int) [ 1000, 96000 ], " "channels = (int) [ 1, 2 ]; "
        "audio/mpeg, "
        "mpegversion = (int) 4, "
        "stream-format = (string) raw, "
        "rate = (int) [ 1000, 96000 ], " "channels = (int) [ 1, 2 ]; "
/*#if 0 VC6 doesn't support #if here ...
        "audio/x-vorbis, "
        "rate = (int) [ 1000, 96000 ], " "channels = (int) [ 1, 2 ]; "
#endif*/
        "audio/x-ac3, "
        "rate = (int) [ 1000, 96000 ], " "channels = (int) [ 1, 6 ]; "
        "audio/x-alaw, "
        "rate = (int) [ 1000, 48000 ], " "channels = (int) [ 1, 2 ]; "
        "audio/x-mulaw, "
        "rate = (int) [ 1000, 48000 ], " "channels = (int) [ 1, 2 ]; "
        "audio/x-wma, "
        "rate = (int) [ 1000, 96000 ], " "channels = (int) [ 1, 2 ], "
        "wmaversion = (int) [ 1, 2 ] ")
    );

static void gst_avi_mux_pad_reset (GstAviPad * avipad, gboolean free);

static GstFlowReturn gst_avi_mux_collect_pads (GstCollectPads * pads,
    GstAviMux * avimux);
static gboolean gst_avi_mux_handle_event (GstCollectPads * pad,
    GstCollectData * data, GstEvent * event, gpointer user_data);
static GstPad *gst_avi_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * name, const GstCaps * caps);
static void gst_avi_mux_release_pad (GstElement * element, GstPad * pad);
static void gst_avi_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_avi_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_avi_mux_change_state (GstElement * element,
    GstStateChange transition);

#define gst_avi_mux_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstAviMux, gst_avi_mux, GST_TYPE_ELEMENT,
    G_IMPLEMENT_INTERFACE (GST_TYPE_TAG_SETTER, NULL));

static void
gst_avi_mux_finalize (GObject * object)
{
  GstAviMux *mux = GST_AVI_MUX (object);
  GSList *node;

  /* completely free each sinkpad */
  node = mux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    node = node->next;

    gst_avi_mux_pad_reset (avipad, TRUE);
    g_free (avipad);
  }
  g_slist_free (mux->sinkpads);
  mux->sinkpads = NULL;

  g_free (mux->idx);
  mux->idx = NULL;

  gst_object_unref (mux->collect);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_avi_mux_class_init (GstAviMuxClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  GST_DEBUG_CATEGORY_INIT (avimux_debug, "avimux", 0, "Muxer for AVI streams");

  gobject_class->get_property = gst_avi_mux_get_property;
  gobject_class->set_property = gst_avi_mux_set_property;
  gobject_class->finalize = gst_avi_mux_finalize;

  g_object_class_install_property (gobject_class, PROP_BIGFILE,
      g_param_spec_boolean ("bigfile", "Bigfile Support (>2GB)",
          "Support for openDML-2.0 (big) AVI files", DEFAULT_BIGFILE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_avi_mux_request_new_pad);
  gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_avi_mux_release_pad);
  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_avi_mux_change_state);

  gst_element_class_add_static_pad_template (gstelement_class, &src_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &audio_sink_factory);
  gst_element_class_add_static_pad_template (gstelement_class,
      &video_sink_factory);

  gst_element_class_set_static_metadata (gstelement_class, "Avi muxer",
      "Codec/Muxer",
      "Muxes audio and video into an avi stream",
      "GStreamer maintainers <gstreamer-devel@lists.freedesktop.org>");
}

/* reset pad to initial state
 * free - if true, release all, not only stream related, data */
static void
gst_avi_mux_pad_reset (GstAviPad * avipad, gboolean free)
{
  /* generic part */
  memset (&(avipad->hdr), 0, sizeof (gst_riff_strh));

  memset (&(avipad->idx[0]), 0, sizeof (avipad->idx));

  if (free) {
    g_free (avipad->tag);
    avipad->tag = NULL;
    g_free (avipad->idx_tag);
    avipad->idx_tag = NULL;
  }

  if (avipad->is_video) {
    GstAviVideoPad *vidpad = (GstAviVideoPad *) avipad;

    avipad->hdr.type = GST_MAKE_FOURCC ('v', 'i', 'd', 's');
    if (vidpad->vids_codec_data) {
      gst_buffer_unref (vidpad->vids_codec_data);
      vidpad->vids_codec_data = NULL;
    }

    if (vidpad->prepend_buffer) {
      gst_buffer_unref (vidpad->prepend_buffer);
      vidpad->prepend_buffer = NULL;
    }

    memset (&(vidpad->vids), 0, sizeof (gst_riff_strf_vids));
    memset (&(vidpad->vprp), 0, sizeof (gst_riff_vprp));
  } else {
    GstAviAudioPad *audpad = (GstAviAudioPad *) avipad;

    audpad->samples = 0;

    avipad->hdr.type = GST_MAKE_FOURCC ('a', 'u', 'd', 's');
    if (audpad->auds_codec_data) {
      gst_buffer_unref (audpad->auds_codec_data);
      audpad->auds_codec_data = NULL;
    }

    memset (&(audpad->auds), 0, sizeof (gst_riff_strf_auds));

    audpad->audio_size = 0;
    audpad->audio_time = 0;
    audpad->max_audio_chunk = 0;
  }
}

static void
gst_avi_mux_reset (GstAviMux * avimux)
{
  GSList *node, *newlist = NULL;

  /* free and reset each sinkpad */
  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    node = node->next;

    gst_avi_mux_pad_reset (avipad, FALSE);
    /* if this pad has collectdata, keep it, otherwise dump it completely */
    if (avipad->collect)
      newlist = g_slist_append (newlist, avipad);
    else {
      gst_avi_mux_pad_reset (avipad, TRUE);
      g_free (avipad);
    }
  }

  /* free the old list of sinkpads, only keep the real collecting ones */
  g_slist_free (avimux->sinkpads);
  avimux->sinkpads = newlist;

  /* avi data */
  avimux->num_frames = 0;
  memset (&(avimux->avi_hdr), 0, sizeof (gst_riff_avih));
  avimux->avi_hdr.max_bps = 10000000;
  avimux->codec_data_size = 0;

  if (avimux->tags_snap) {
    gst_tag_list_unref (avimux->tags_snap);
    avimux->tags_snap = NULL;
  }

  g_free (avimux->idx);
  avimux->idx = NULL;

  /* state info */
  avimux->write_header = TRUE;

  /* tags */
  gst_tag_setter_reset_tags (GST_TAG_SETTER (avimux));
}

static void
gst_avi_mux_init (GstAviMux * avimux)
{
  avimux->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_use_fixed_caps (avimux->srcpad);
  gst_element_add_pad (GST_ELEMENT (avimux), avimux->srcpad);

  /* property */
  avimux->enable_large_avi = DEFAULT_BIGFILE;

  avimux->collect = gst_collect_pads_new ();
  gst_collect_pads_set_function (avimux->collect,
      (GstCollectPadsFunction) (GST_DEBUG_FUNCPTR (gst_avi_mux_collect_pads)),
      avimux);
  gst_collect_pads_set_event_function (avimux->collect,
      (GstCollectPadsEventFunction) (GST_DEBUG_FUNCPTR
          (gst_avi_mux_handle_event)), avimux);

  /* set to clean state */
  gst_avi_mux_reset (avimux);
}

static gboolean
gst_avi_mux_vidsink_set_caps (GstPad * pad, GstCaps * vscaps)
{
  GstAviMux *avimux;
  GstAviVideoPad *avipad;
  GstAviCollectData *collect_pad;
  GstStructure *structure;
  const gchar *mimetype;
  const GValue *fps, *par;
  const GValue *codec_data;
  gint width, height;
  gint par_n, par_d;
  gboolean codec_data_in_headers = TRUE;
  gboolean valid_caps = TRUE;

  avimux = GST_AVI_MUX (gst_pad_get_parent (pad));

  /* find stream data */
  collect_pad = (GstAviCollectData *) gst_pad_get_element_private (pad);
  g_assert (collect_pad);
  avipad = (GstAviVideoPad *) collect_pad->avipad;
  g_assert (avipad);
  g_assert (avipad->parent.is_video);
  g_assert (avipad->parent.hdr.type == GST_MAKE_FOURCC ('v', 'i', 'd', 's'));

  GST_DEBUG_OBJECT (avimux, "%s:%s, caps=%" GST_PTR_FORMAT,
      GST_DEBUG_PAD_NAME (pad), vscaps);

  structure = gst_caps_get_structure (vscaps, 0);
  mimetype = gst_structure_get_name (structure);

  /* global */
  avipad->vids.size = sizeof (gst_riff_strf_vids);
  avipad->vids.planes = 1;
  if (!gst_structure_get_int (structure, "width", &width) ||
      !gst_structure_get_int (structure, "height", &height)) {
    goto refuse_caps;
  }

  avipad->vids.width = width;
  avipad->vids.height = height;

  fps = gst_structure_get_value (structure, "framerate");
  if (fps == NULL || !GST_VALUE_HOLDS_FRACTION (fps))
    goto refuse_caps;

  avipad->parent.hdr.rate = gst_value_get_fraction_numerator (fps);
  avipad->parent.hdr.scale = gst_value_get_fraction_denominator (fps);
  if (avipad->parent.hdr.rate <= 0 || avipad->parent.hdr.scale <= 0)
    goto refuse_caps;

  /* (pixel) aspect ratio data, if any */
  par = gst_structure_get_value (structure, "pixel-aspect-ratio");
  /* only use video properties header if there is non-trivial aspect info */
  if (par && GST_VALUE_HOLDS_FRACTION (par) &&
      ((par_n = gst_value_get_fraction_numerator (par)) !=
          (par_d = gst_value_get_fraction_denominator (par)))) {
    GValue to_ratio = { 0, };
    guint ratio_n, ratio_d;

    /* some fraction voodoo to obtain simplest possible ratio */
    g_value_init (&to_ratio, GST_TYPE_FRACTION);
    gst_value_set_fraction (&to_ratio, width * par_n, height * par_d);
    ratio_n = gst_value_get_fraction_numerator (&to_ratio);
    ratio_d = gst_value_get_fraction_denominator (&to_ratio);
    GST_DEBUG_OBJECT (avimux, "generating vprp data with aspect ratio %d/%d",
        ratio_n, ratio_d);
    /* simply fill in */
    avipad->vprp.vert_rate = avipad->parent.hdr.rate / avipad->parent.hdr.scale;
    avipad->vprp.hor_t_total = width;
    avipad->vprp.vert_lines = height;
    avipad->vprp.aspect = (ratio_n) << 16 | (ratio_d & 0xffff);
    avipad->vprp.width = width;
    avipad->vprp.height = height;
    avipad->vprp.fields = 1;
    avipad->vprp.field_info[0].compressed_bm_height = height;
    avipad->vprp.field_info[0].compressed_bm_width = width;
    avipad->vprp.field_info[0].valid_bm_height = height;
    avipad->vprp.field_info[0].valid_bm_width = width;
  }

  if (!strcmp (mimetype, "video/x-raw")) {
    const gchar *format;
    GstVideoFormat fmt;

    format = gst_structure_get_string (structure, "format");
    fmt = gst_video_format_from_string (format);

    switch (fmt) {
      case GST_VIDEO_FORMAT_YUY2:
        avipad->vids.compression = GST_MAKE_FOURCC ('Y', 'U', 'Y', '2');
        avipad->vids.bit_cnt = 16;
        break;
      case GST_VIDEO_FORMAT_UYVY:
        avipad->vids.compression = GST_MAKE_FOURCC ('U', 'Y', 'V', 'Y');
        avipad->vids.bit_cnt = 16;
        break;
      case GST_VIDEO_FORMAT_I420:
        avipad->vids.compression = GST_MAKE_FOURCC ('I', '4', '2', '0');
        avipad->vids.bit_cnt = 12;
        break;
      case GST_VIDEO_FORMAT_GRAY8:
        avipad->vids.compression = GST_MAKE_FOURCC ('Y', '8', '0', '0');
        avipad->vids.bit_cnt = 8;
        break;
      case GST_VIDEO_FORMAT_BGR:
        avipad->vids.compression = GST_MAKE_FOURCC (0x00, 0x00, 0x00, 0x00);
        avipad->vids.bit_cnt = 24;
        break;
      case GST_VIDEO_FORMAT_BGRx:
      case GST_VIDEO_FORMAT_BGRA:
        avipad->vids.compression = GST_MAKE_FOURCC (0x00, 0x00, 0x00, 0x00);
        avipad->vids.bit_cnt = 32;
        break;
      default:
        valid_caps = FALSE;
        break;
    }
  } else {
    avipad->vids.bit_cnt = 24;
    avipad->vids.compression = 0;

    /* find format */
    if (!strcmp (mimetype, "video/x-huffyuv")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('H', 'F', 'Y', 'U');
    } else if (!strcmp (mimetype, "image/jpeg")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('M', 'J', 'P', 'G');
    } else if (!strcmp (mimetype, "video/x-divx")) {
      gint divxversion;

      gst_structure_get_int (structure, "divxversion", &divxversion);
      switch (divxversion) {
        case 3:
          avipad->vids.compression = GST_MAKE_FOURCC ('D', 'I', 'V', '3');
          break;
        case 4:
          avipad->vids.compression = GST_MAKE_FOURCC ('D', 'I', 'V', 'X');
          break;
        case 5:
          avipad->vids.compression = GST_MAKE_FOURCC ('D', 'X', '5', '0');
          break;
        default:
          valid_caps = FALSE;
      }
    } else if (gst_structure_has_name (structure, "video/x-msmpeg")) {
      gint msmpegversion;

      gst_structure_get_int (structure, "msmpegversion", &msmpegversion);
      switch (msmpegversion) {
        case 41:
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', 'G', '4');
          break;
        case 42:
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', '4', '2');
          break;
        case 43:
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', '4', '3');
          break;
        default:
          GST_INFO ("unhandled msmpegversion : %d, fall back to fourcc=MPEG",
              msmpegversion);
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', 'E', 'G');
          break;
      }
    } else if (!strcmp (mimetype, "video/x-dv")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('D', 'V', 'S', 'D');
    } else if (!strcmp (mimetype, "video/x-h263")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('H', '2', '6', '3');
    } else if (!strcmp (mimetype, "video/x-h264")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('H', '2', '6', '4');
    } else if (!strcmp (mimetype, "video/mpeg")) {
      gint mpegversion;

      gst_structure_get_int (structure, "mpegversion", &mpegversion);

      switch (mpegversion) {
        case 2:
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', 'G', '2');
          break;
        case 4:
          /* mplayer/ffmpeg might not work with DIVX, but with FMP4 */
          avipad->vids.compression = GST_MAKE_FOURCC ('D', 'I', 'V', 'X');

          /* DIVX/XVID in AVI store the codec_data chunk as part of the
             first data buffer. So for this case, we prepend the codec_data
             blob (if any) to that first buffer */
          codec_data_in_headers = FALSE;
          break;
        default:
          GST_INFO ("unhandled mpegversion : %d, fall back to fourcc=MPEG",
              mpegversion);
          avipad->vids.compression = GST_MAKE_FOURCC ('M', 'P', 'E', 'G');
          break;
      }
    } else if (!strcmp (mimetype, "video/x-wmv")) {
      gint wmvversion;

      if (gst_structure_get_int (structure, "wmvversion", &wmvversion)) {
        switch (wmvversion) {
          case 1:
            avipad->vids.compression = GST_MAKE_FOURCC ('W', 'M', 'V', '1');
            break;
          case 2:
            avipad->vids.compression = GST_MAKE_FOURCC ('W', 'M', 'V', '2');
            break;
          case 3:
            avipad->vids.compression = GST_MAKE_FOURCC ('W', 'M', 'V', '3');
            break;
          default:
            valid_caps = FALSE;
            break;
        }
      }
    } else if (!strcmp (mimetype, "image/x-jpc")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('M', 'J', '2', 'C');
    } else if (!strcmp (mimetype, "video/x-vp8")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('V', 'P', '8', '0');
    } else if (!strcmp (mimetype, "image/png")) {
      avipad->vids.compression = GST_MAKE_FOURCC ('p', 'n', 'g', ' ');
    } else {
      valid_caps = FALSE;
    }

    if (!valid_caps)
      goto refuse_caps;
  }

  /* codec initialization data, if any */
  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    if (codec_data_in_headers) {
      avipad->vids_codec_data = gst_value_get_buffer (codec_data);
      gst_buffer_ref (avipad->vids_codec_data);
      /* keep global track of size */
      avimux->codec_data_size += gst_buffer_get_size (avipad->vids_codec_data);
    } else {
      avipad->prepend_buffer =
          gst_buffer_ref (gst_value_get_buffer (codec_data));
    }
  }

  avipad->parent.hdr.fcc_handler = avipad->vids.compression;
  avipad->vids.image_size = avipad->vids.height * avipad->vids.width;
  /* hm, maybe why avi only handles one stream well ... */
  avimux->avi_hdr.width = avipad->vids.width;
  avimux->avi_hdr.height = avipad->vids.height;
  avimux->avi_hdr.us_frame = 1000000. * avipad->parent.hdr.scale /
      avipad->parent.hdr.rate;

  gst_object_unref (avimux);
  return TRUE;

refuse_caps:
  {
    GST_WARNING_OBJECT (avimux, "refused caps %" GST_PTR_FORMAT, vscaps);
    gst_object_unref (avimux);
    return FALSE;
  }
}

static void gst_avi_mux_audsink_set_fields (GstAviMux * avimux,
    GstAviAudioPad * avipad);

static GstFlowReturn
gst_avi_mux_audsink_scan_mpeg_audio (GstAviMux * avimux, GstAviPad * avipad,
    GstBuffer * buffer)
{
  GstMapInfo map;
  guint spf;
  guint32 header;
  gulong layer;
  gulong version;
  gint lsf, mpg25;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  if (map.size < 4)
    goto not_parsed;

  header = GST_READ_UINT32_BE (map.data);

  if ((header & 0xffe00000) != 0xffe00000)
    goto not_parsed;

  /* thanks go to mp3parse */
  if (header & (1 << 20)) {
    lsf = (header & (1 << 19)) ? 0 : 1;
    mpg25 = 0;
  } else {
    lsf = 1;
    mpg25 = 1;
  }

  version = 1 + lsf + mpg25;
  layer = 4 - ((header >> 17) & 0x3);

  /* see http://www.codeproject.com/audio/MPEGAudioInfo.asp */
  if (layer == 1)
    spf = 384;
  else if (layer == 2)
    spf = 1152;
  else if (version == 1) {
    spf = 1152;
  } else {
    /* MPEG-2 or "2.5" */
    spf = 576;
  }

  if (G_UNLIKELY (avipad->hdr.scale <= 1)) {
    avipad->hdr.scale = spf;
    gst_avi_mux_audsink_set_fields (avimux, (GstAviAudioPad *) avipad);
  } else if (G_UNLIKELY (avipad->hdr.scale != spf)) {
    GST_WARNING_OBJECT (avimux, "input mpeg audio has varying frame size");
    goto cbr_fallback;
  }
done:
  gst_buffer_unmap (buffer, &map);

  return GST_FLOW_OK;

  /* EXITS */
not_parsed:
  {
    GST_WARNING_OBJECT (avimux, "input mpeg audio is not parsed");
    /* fall-through */
  }
cbr_fallback:
  {
    GST_WARNING_OBJECT (avimux, "falling back to CBR muxing");
    avipad->hdr.scale = 1;
    gst_avi_mux_audsink_set_fields (avimux, (GstAviAudioPad *) avipad);
    /* no need to check further */
    avipad->hook = NULL;
    goto done;
  }
}

static void
gst_avi_mux_audsink_set_fields (GstAviMux * avimux, GstAviAudioPad * avipad)
{
  if (avipad->parent.hdr.scale > 1) {
    /* vbr case: fixed duration per frame/chunk */
    avipad->parent.hdr.rate = avipad->auds.rate;
    avipad->parent.hdr.samplesize = 0;
    /* this triggers determining largest audio chunk size to write at end */
    avipad->max_audio_chunk = avipad->auds.blockalign =
        avipad->parent.hdr.scale;
  } else {
    /* by spec, hdr.rate is av_bps related, is calculated that way in stop_file,
     * and reduces to sample rate in PCM like cases */
    avipad->parent.hdr.rate = avipad->auds.av_bps / avipad->auds.blockalign;
    avipad->parent.hdr.samplesize = avipad->auds.blockalign;
    avipad->parent.hdr.scale = 1;
  }
}

static gboolean
gst_avi_mux_audsink_set_caps (GstPad * pad, GstCaps * vscaps)
{
  GstAviMux *avimux;
  GstAviAudioPad *avipad;
  GstAviCollectData *collect_pad;
  GstStructure *structure;
  const gchar *mimetype;
  const GValue *codec_data;
  gint channels, rate;

  avimux = GST_AVI_MUX (gst_pad_get_parent (pad));

  /* find stream data */
  collect_pad = (GstAviCollectData *) gst_pad_get_element_private (pad);
  g_assert (collect_pad);
  avipad = (GstAviAudioPad *) collect_pad->avipad;
  g_assert (avipad);
  g_assert (!avipad->parent.is_video);
  g_assert (avipad->parent.hdr.type == GST_MAKE_FOURCC ('a', 'u', 'd', 's'));

  GST_DEBUG_OBJECT (avimux, "%s:%s, caps=%" GST_PTR_FORMAT,
      GST_DEBUG_PAD_NAME (pad), vscaps);

  structure = gst_caps_get_structure (vscaps, 0);
  mimetype = gst_structure_get_name (structure);

  /* we want these for all */
  if (!gst_structure_get_int (structure, "channels", &channels) ||
      !gst_structure_get_int (structure, "rate", &rate)) {
    goto refuse_caps;
  }

  avipad->auds.channels = channels;
  avipad->auds.rate = rate;

  /* codec initialization data, if any */
  codec_data = gst_structure_get_value (structure, "codec_data");
  if (codec_data) {
    avipad->auds_codec_data = gst_value_get_buffer (codec_data);
    gst_buffer_ref (avipad->auds_codec_data);
    /* keep global track of size */
    avimux->codec_data_size += gst_buffer_get_size (avipad->auds_codec_data);
  }

  if (!strcmp (mimetype, "audio/x-raw")) {
    const gchar *format;
    GstAudioFormat fmt;

    format = gst_structure_get_string (structure, "format");
    fmt = gst_audio_format_from_string (format);

    switch (fmt) {
      case GST_AUDIO_FORMAT_U8:
        avipad->auds.blockalign = 8;
        avipad->auds.bits_per_sample = 8;
        break;
      case GST_AUDIO_FORMAT_S16:
        avipad->auds.blockalign = 16;
        avipad->auds.bits_per_sample = 16;
        break;
      default:
        goto refuse_caps;
    }

    avipad->auds.format = GST_RIFF_WAVE_FORMAT_PCM;
    /* set some more info straight */
    avipad->auds.blockalign /= 8;
    avipad->auds.blockalign *= avipad->auds.channels;
    avipad->auds.av_bps = avipad->auds.blockalign * avipad->auds.rate;
  } else {
    avipad->auds.format = 0;
    /* set some defaults */
    avipad->auds.blockalign = 1;
    avipad->auds.av_bps = 0;
    avipad->auds.bits_per_sample = 16;

    if (!strcmp (mimetype, "audio/mpeg")) {
      gint mpegversion;

      gst_structure_get_int (structure, "mpegversion", &mpegversion);
      switch (mpegversion) {
        case 1:{
          gint layer = 3;
          gboolean parsed = FALSE;

          gst_structure_get_int (structure, "layer", &layer);
          gst_structure_get_boolean (structure, "parsed", &parsed);
          switch (layer) {
            case 3:
              avipad->auds.format = GST_RIFF_WAVE_FORMAT_MPEGL3;
              break;
            case 1:
            case 2:
              avipad->auds.format = GST_RIFF_WAVE_FORMAT_MPEGL12;
              break;
          }
          if (parsed) {
            /* treat as VBR, should also cover CBR case;
             * setup hook to parse frame header and determine spf */
            avipad->parent.hook = gst_avi_mux_audsink_scan_mpeg_audio;
          } else {
            GST_WARNING_OBJECT (avimux, "unparsed MPEG audio input (?), "
                "doing CBR muxing");
          }
          break;
        }
        case 4:
        {
          GstBuffer *codec_data_buf = avipad->auds_codec_data;
          const gchar *stream_format;
          guint codec;
          guint8 data[2];

          stream_format = gst_structure_get_string (structure, "stream-format");
          if (stream_format) {
            if (strcmp (stream_format, "raw") != 0) {
              GST_WARNING_OBJECT (avimux, "AAC's stream format '%s' is not "
                  "supported, please use 'raw'", stream_format);
              break;
            }
          } else {
            GST_WARNING_OBJECT (avimux, "AAC's stream-format not specified, "
                "assuming 'raw'");
          }

          /* vbr case needs some special handling */
          if (!codec_data_buf || gst_buffer_get_size (codec_data_buf) < 2) {
            GST_WARNING_OBJECT (avimux, "no (valid) codec_data for AAC audio");
            break;
          }
          avipad->auds.format = GST_RIFF_WAVE_FORMAT_AAC;
          /* need to determine frame length */
          gst_buffer_extract (codec_data_buf, 0, data, 2);
          codec = GST_READ_UINT16_BE (data);
          avipad->parent.hdr.scale = (codec & 0x4) ? 960 : 1024;
          break;
        }
      }
    } else if (!strcmp (mimetype, "audio/x-vorbis")) {
      avipad->auds.format = GST_RIFF_WAVE_FORMAT_VORBIS3;
    } else if (!strcmp (mimetype, "audio/x-ac3")) {
      avipad->auds.format = GST_RIFF_WAVE_FORMAT_A52;
    } else if (!strcmp (mimetype, "audio/x-alaw")) {
      avipad->auds.format = GST_RIFF_WAVE_FORMAT_ALAW;
      avipad->auds.bits_per_sample = 8;
      avipad->auds.blockalign = avipad->auds.channels;
      avipad->auds.av_bps = avipad->auds.blockalign * avipad->auds.rate;
    } else if (!strcmp (mimetype, "audio/x-mulaw")) {
      avipad->auds.format = GST_RIFF_WAVE_FORMAT_MULAW;
      avipad->auds.bits_per_sample = 8;
      avipad->auds.blockalign = avipad->auds.channels;
      avipad->auds.av_bps = avipad->auds.blockalign * avipad->auds.rate;
    } else if (!strcmp (mimetype, "audio/x-wma")) {
      gint version;
      gint bitrate;
      gint block_align;

      if (gst_structure_get_int (structure, "wmaversion", &version)) {
        switch (version) {
          case 1:
            avipad->auds.format = GST_RIFF_WAVE_FORMAT_WMAV1;
            break;
          case 2:
            avipad->auds.format = GST_RIFF_WAVE_FORMAT_WMAV2;
            break;
          default:
            break;
        }
      }

      if (avipad->auds.format != 0) {
        if (gst_structure_get_int (structure, "block_align", &block_align)) {
          avipad->auds.blockalign = block_align;
        }
        if (gst_structure_get_int (structure, "bitrate", &bitrate)) {
          avipad->auds.av_bps = bitrate / 8;
        }
      }
    }
  }

  if (!avipad->auds.format)
    goto refuse_caps;

  avipad->parent.hdr.fcc_handler = avipad->auds.format;
  gst_avi_mux_audsink_set_fields (avimux, avipad);

  gst_object_unref (avimux);
  return TRUE;

refuse_caps:
  {
    GST_WARNING_OBJECT (avimux, "refused caps %" GST_PTR_FORMAT, vscaps);
    gst_object_unref (avimux);
    return FALSE;
  }
}


static GstPad *
gst_avi_mux_request_new_pad (GstElement * element,
    GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstAviMux *avimux;
  GstPad *newpad;
  GstAviPad *avipad;
  GstElementClass *klass;
  gchar *name = NULL;
  const gchar *pad_name = NULL;
  gint pad_id;

  g_return_val_if_fail (templ != NULL, NULL);

  if (templ->direction != GST_PAD_SINK)
    goto wrong_direction;

  g_return_val_if_fail (GST_IS_AVI_MUX (element), NULL);
  avimux = GST_AVI_MUX (element);

  if (!avimux->write_header)
    goto too_late;

  klass = GST_ELEMENT_GET_CLASS (element);

  if (templ == gst_element_class_get_pad_template (klass, "audio_%u")) {
    /* don't mix named and unnamed pads, if the pad already exists we fail when
     * trying to add it */
    if (req_name != NULL && sscanf (req_name, "audio_%u", &pad_id) == 1) {
      pad_name = req_name;
    } else {
      name = g_strdup_printf ("audio_%u", avimux->audio_pads++);
      pad_name = name;
    }

    /* init pad specific data */
    avipad = g_malloc0 (sizeof (GstAviAudioPad));
    avipad->is_video = FALSE;
    avipad->hdr.type = GST_MAKE_FOURCC ('a', 'u', 'd', 's');
    /* audio goes last */
    avimux->sinkpads = g_slist_append (avimux->sinkpads, avipad);
  } else if (templ == gst_element_class_get_pad_template (klass, "video_%u")) {
    /* though streams are pretty generic and relatively self-contained,
     * some video info goes in a single avi header -and therefore mux struct-
     * so video restricted to one stream */
    if (avimux->video_pads > 0)
      goto too_many_video_pads;

    /* setup pad */
    pad_name = "video_0";
    avimux->video_pads++;

    /* init pad specific data */
    avipad = g_malloc0 (sizeof (GstAviVideoPad));
    avipad->is_video = TRUE;
    avipad->hdr.type = GST_MAKE_FOURCC ('v', 'i', 'd', 's');
    /* video goes first */
    avimux->sinkpads = g_slist_prepend (avimux->sinkpads, avipad);
  } else
    goto wrong_template;

  newpad = gst_pad_new_from_template (templ, pad_name);

  avipad->collect = gst_collect_pads_add_pad (avimux->collect,
      newpad, sizeof (GstAviCollectData), NULL, TRUE);
  ((GstAviCollectData *) (avipad->collect))->avipad = avipad;

  if (!gst_element_add_pad (element, newpad))
    goto pad_add_failed;

  g_free (name);

  GST_DEBUG_OBJECT (newpad, "Added new request pad");

  return newpad;

  /* ERRORS */
wrong_direction:
  {
    g_warning ("avimux: request pad that is not a SINK pad\n");
    return NULL;
  }
too_late:
  {
    g_warning ("avimux: request pad cannot be added after streaming started\n");
    return NULL;
  }
wrong_template:
  {
    g_warning ("avimux: this is not our template!\n");
    return NULL;
  }
too_many_video_pads:
  {
    GST_WARNING_OBJECT (avimux, "Can only have one video stream");
    return NULL;
  }
pad_add_failed:
  {
    GST_WARNING_OBJECT (avimux, "Adding the new pad '%s' failed", pad_name);
    g_free (name);
    gst_object_unref (newpad);
    return NULL;
  }
}

static void
gst_avi_mux_release_pad (GstElement * element, GstPad * pad)
{
  GstAviMux *avimux = GST_AVI_MUX (element);
  GSList *node;

  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    if (avipad->collect->pad == pad) {
      /* pad count should not be adjusted,
       * as it also represent number of streams present */
      avipad->collect = NULL;
      GST_DEBUG_OBJECT (avimux, "removed pad '%s'", GST_PAD_NAME (pad));
      gst_collect_pads_remove_pad (avimux->collect, pad);
      gst_element_remove_pad (element, pad);
      /* if not started yet, we can remove any sign this pad ever existed */
      /* in this case _start will take care of the real pad count */
      if (avimux->write_header) {
        avimux->sinkpads = g_slist_remove (avimux->sinkpads, avipad);
        gst_avi_mux_pad_reset (avipad, TRUE);
        g_free (avipad);
      }
      return;
    }

    node = node->next;
  }

  g_warning ("Unknown pad %s", GST_PAD_NAME (pad));
}

static inline guint
gst_avi_mux_start_chunk (GstByteWriter * bw, const gchar * tag, guint32 fourcc)
{
  guint chunk_offset;

  if (tag)
    gst_byte_writer_put_data (bw, (const guint8 *) tag, 4);
  else
    gst_byte_writer_put_uint32_le (bw, fourcc);

  chunk_offset = gst_byte_writer_get_pos (bw);
  /* real chunk size comes later */
  gst_byte_writer_put_uint32_le (bw, 0);

  return chunk_offset;
}

static inline void
gst_avi_mux_end_chunk (GstByteWriter * bw, guint chunk_offset)
{
  guint size;

  size = gst_byte_writer_get_pos (bw);

  gst_byte_writer_set_pos (bw, chunk_offset);
  gst_byte_writer_put_uint32_le (bw, size - chunk_offset - 4);
  gst_byte_writer_set_pos (bw, size);

  /* arrange for even padding */
  if (size & 1)
    gst_byte_writer_put_uint8 (bw, 0);
}

/* maybe some of these functions should be moved to riff.h? */

static void
gst_avi_mux_write_tag (const GstTagList * list, const gchar * tag,
    gpointer data)
{
  const struct
  {
    guint32 fcc;
    const gchar *tag;
  } rifftags[] = {
    {
    GST_RIFF_INFO_IARL, GST_TAG_LOCATION}, {
    GST_RIFF_INFO_IART, GST_TAG_ARTIST}, {
    GST_RIFF_INFO_ICMT, GST_TAG_COMMENT}, {
    GST_RIFF_INFO_ICOP, GST_TAG_COPYRIGHT}, {
    GST_RIFF_INFO_ICRD, GST_TAG_DATE}, {
    GST_RIFF_INFO_IGNR, GST_TAG_GENRE}, {
    GST_RIFF_INFO_IKEY, GST_TAG_KEYWORDS}, {
    GST_RIFF_INFO_INAM, GST_TAG_TITLE}, {
    GST_RIFF_INFO_ISFT, GST_TAG_ENCODER}, {
    GST_RIFF_INFO_ISRC, GST_TAG_ISRC}, {
    0, NULL}
  };
  gint n;
  gchar *str = NULL;
  GstByteWriter *bw = data;
  guint chunk;

  for (n = 0; rifftags[n].fcc != 0; n++) {
    if (!strcmp (rifftags[n].tag, tag)) {
      if (rifftags[n].fcc == GST_RIFF_INFO_ICRD) {
        GDate *date;
        /* special case for the date tag */
        if (gst_tag_list_get_date (list, tag, &date)) {
          str =
              g_strdup_printf ("%04d:%02d:%02d", g_date_get_year (date),
              g_date_get_month (date), g_date_get_day (date));
          g_date_free (date);
        }
      } else {
        gst_tag_list_get_string (list, tag, &str);
      }
      if (str) {
        chunk = gst_avi_mux_start_chunk (bw, NULL, rifftags[n].fcc);
        gst_byte_writer_put_string (bw, str);
        gst_avi_mux_end_chunk (bw, chunk);
        g_free (str);
        str = NULL;
        break;
      }
    }
  }
}

static GstBuffer *
gst_avi_mux_riff_get_avi_header (GstAviMux * avimux)
{
  const GstTagList *tags;
  GstBuffer *buffer = NULL;
  gint size = 0;
  GstByteWriter bw;
  GSList *node;
  guint avih, riff, hdrl;
  GstMapInfo map;
  gboolean hdl = TRUE;

  GST_DEBUG_OBJECT (avimux, "creating avi header, data_size %u, idx_size %u",
      avimux->data_size, avimux->idx_size);

  if (avimux->tags_snap)
    tags = avimux->tags_snap;
  else {
    /* need to make snapshot of current state of tags to ensure the same set
     * is used next time around during header rewrite at the end */
    tags = gst_tag_setter_get_tag_list (GST_TAG_SETTER (avimux));
    if (tags)
      tags = avimux->tags_snap = gst_tag_list_copy (tags);
  }

  gst_byte_writer_init_with_size (&bw, 1024, FALSE);

  /* avi header metadata */
  riff = gst_avi_mux_start_chunk (&bw, "RIFF", 0);
  hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "AVI ", 4);
  hdrl = gst_avi_mux_start_chunk (&bw, "LIST", 0);
  hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "hdrl", 4);

  avih = gst_avi_mux_start_chunk (&bw, "avih", 0);
  /* the AVI header itself */
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.us_frame);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.max_bps);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.pad_gran);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.flags);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.tot_frames);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.init_frames);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.streams);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.bufsize);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.width);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.height);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.scale);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.rate);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.start);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->avi_hdr.length);
  gst_avi_mux_end_chunk (&bw, avih);

  /* stream data */
  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;
    GstAviVideoPad *vidpad = (GstAviVideoPad *) avipad;
    GstAviAudioPad *audpad = (GstAviAudioPad *) avipad;
    gint codec_size = 0;
    guint strh, strl, strf, indx;

    /* stream list metadata */
    strl = gst_avi_mux_start_chunk (&bw, "LIST", 0);
    hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "strl", 4);

    /* generic header */
    strh = gst_avi_mux_start_chunk (&bw, "strh", 0);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.type);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.fcc_handler);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.flags);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.priority);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.init_frames);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.scale);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.rate);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.start);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.length);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.bufsize);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.quality);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->hdr.samplesize);
    hdl &= gst_byte_writer_put_uint16_le (&bw, 0);
    hdl &= gst_byte_writer_put_uint16_le (&bw, 0);
    hdl &= gst_byte_writer_put_uint16_le (&bw, 0);
    hdl &= gst_byte_writer_put_uint16_le (&bw, 0);
    gst_avi_mux_end_chunk (&bw, strh);

    if (avipad->is_video) {
      codec_size = vidpad->vids_codec_data ?
          gst_buffer_get_size (vidpad->vids_codec_data) : 0;
      /* the video header */
      strf = gst_avi_mux_start_chunk (&bw, "strf", 0);
      /* the actual header */
      hdl &=
          gst_byte_writer_put_uint32_le (&bw, vidpad->vids.size + codec_size);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.width);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.height);
      hdl &= gst_byte_writer_put_uint16_le (&bw, vidpad->vids.planes);
      hdl &= gst_byte_writer_put_uint16_le (&bw, vidpad->vids.bit_cnt);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.compression);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.image_size);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.xpels_meter);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.ypels_meter);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.num_colors);
      hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vids.imp_colors);
      if (vidpad->vids_codec_data) {
        gst_buffer_map (vidpad->vids_codec_data, &map, GST_MAP_READ);
        hdl &= gst_byte_writer_put_data (&bw, map.data, map.size);
        gst_buffer_unmap (vidpad->vids_codec_data, &map);
      }
      gst_avi_mux_end_chunk (&bw, strf);

      /* add video property data, mainly for aspect ratio, if any */
      if (vidpad->vprp.aspect) {
        gint f;
        guint vprp;

        /* let's be on the safe side */
        vidpad->vprp.fields = MIN (vidpad->vprp.fields,
            GST_RIFF_VPRP_VIDEO_FIELDS);
        /* the vprp header */
        vprp = gst_avi_mux_start_chunk (&bw, "vprp", 0);
        /* the actual data */
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.format_token);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.standard);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.vert_rate);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.hor_t_total);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.vert_lines);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.aspect);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.width);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.height);
        hdl &= gst_byte_writer_put_uint32_le (&bw, vidpad->vprp.fields);

        for (f = 0; f < vidpad->vprp.fields; ++f) {
          gst_riff_vprp_video_field_desc *fd;

          fd = &(vidpad->vprp.field_info[f]);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->compressed_bm_height);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->compressed_bm_width);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->valid_bm_height);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->valid_bm_width);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->valid_bm_x_offset);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->valid_bm_y_offset);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->video_x_t_offset);
          hdl &= gst_byte_writer_put_uint32_le (&bw, fd->video_y_start);
        }
        gst_avi_mux_end_chunk (&bw, vprp);
      }
    } else {
      codec_size = audpad->auds_codec_data ?
          gst_buffer_get_size (audpad->auds_codec_data) : 0;
      /* the audio header */
      strf = gst_avi_mux_start_chunk (&bw, "strf", 0);
      /* the actual header */
      hdl &= gst_byte_writer_put_uint16_le (&bw, audpad->auds.format);
      hdl &= gst_byte_writer_put_uint16_le (&bw, audpad->auds.channels);
      hdl &= gst_byte_writer_put_uint32_le (&bw, audpad->auds.rate);
      hdl &= gst_byte_writer_put_uint32_le (&bw, audpad->auds.av_bps);
      hdl &= gst_byte_writer_put_uint16_le (&bw, audpad->auds.blockalign);
      hdl &= gst_byte_writer_put_uint16_le (&bw, audpad->auds.bits_per_sample);
      hdl &= gst_byte_writer_put_uint16_le (&bw, codec_size);
      if (audpad->auds_codec_data) {
        gst_buffer_map (audpad->auds_codec_data, &map, GST_MAP_READ);
        hdl &= gst_byte_writer_put_data (&bw, map.data, map.size);
        gst_buffer_unmap (audpad->auds_codec_data, &map);
      }
      gst_avi_mux_end_chunk (&bw, strf);
    }

    /* odml superindex chunk */
    if (avipad->idx_index > 0)
      indx = gst_avi_mux_start_chunk (&bw, "indx", 0);
    else
      indx = gst_avi_mux_start_chunk (&bw, "JUNK", 0);
    hdl &= gst_byte_writer_put_uint16_le (&bw, 4);      /* bytes per entry */
    hdl &= gst_byte_writer_put_uint8 (&bw, 0);  /* index subtype */
    hdl &= gst_byte_writer_put_uint8 (&bw, GST_AVI_INDEX_OF_INDEXES);   /* index type */
    hdl &= gst_byte_writer_put_uint32_le (&bw, avipad->idx_index);      /* entries in use */
    hdl &= gst_byte_writer_put_data (&bw, (guint8 *) avipad->tag, 4);   /* stream id */
    hdl &= gst_byte_writer_put_uint32_le (&bw, 0);      /* reserved */
    hdl &= gst_byte_writer_put_uint32_le (&bw, 0);      /* reserved */
    hdl &= gst_byte_writer_put_uint32_le (&bw, 0);      /* reserved */
    hdl &= gst_byte_writer_put_data (&bw, (guint8 *) avipad->idx,
        GST_AVI_SUPERINDEX_COUNT * sizeof (gst_avi_superindex_entry));
    gst_avi_mux_end_chunk (&bw, indx);

    /* end strl for this stream */
    gst_avi_mux_end_chunk (&bw, strl);

    node = node->next;
  }

  if (avimux->video_pads > 0) {
    guint odml, dmlh;
    /* odml header */
    odml = gst_avi_mux_start_chunk (&bw, "LIST", 0);
    hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "odml", 4);
    dmlh = gst_avi_mux_start_chunk (&bw, "dmlh", 0);
    hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->total_frames);
    gst_avi_mux_end_chunk (&bw, dmlh);
    gst_avi_mux_end_chunk (&bw, odml);
  }

  /* end hdrl */
  gst_avi_mux_end_chunk (&bw, hdrl);

  /* tags */
  if (tags) {
    guint info;

    info = gst_avi_mux_start_chunk (&bw, "LIST", 0);
    hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "INFO", 4);

    gst_tag_list_foreach (tags, gst_avi_mux_write_tag, &bw);
    if (info + 8 == gst_byte_writer_get_pos (&bw)) {
      /* no tags written, remove the empty INFO LIST as it is useless
       * and prevents playback in vlc */
      gst_byte_writer_set_pos (&bw, info - 4);
    } else {
      gst_avi_mux_end_chunk (&bw, info);
    }
  }

  /* pop RIFF */
  gst_avi_mux_end_chunk (&bw, riff);

  /* avi data header */
  hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "LIST", 4);
  hdl &= gst_byte_writer_put_uint32_le (&bw, avimux->data_size);
  hdl &= gst_byte_writer_put_data (&bw, (guint8 *) "movi", 4);

  if (!hdl)
    goto beach;

  /* now get the data */
  buffer = gst_byte_writer_reset_and_get_buffer (&bw);

  /* ... but RIFF includes more than just header */
  gst_buffer_map (buffer, &map, GST_MAP_READWRITE);
  size = GST_READ_UINT32_LE (map.data + 4);
  size += 8 + avimux->data_size + avimux->idx_size;
  GST_WRITE_UINT32_LE (map.data + 4, size);

  GST_MEMDUMP_OBJECT (avimux, "avi header", map.data, map.size);
  gst_buffer_unmap (buffer, &map);

beach:
  return buffer;
}

static GstBuffer *
gst_avi_mux_riff_get_avix_header (guint32 datax_size)
{
  GstBuffer *buffer;
  GstMapInfo map;

  buffer = gst_buffer_new_and_alloc (24);

  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  memcpy (map.data + 0, "RIFF", 4);
  GST_WRITE_UINT32_LE (map.data + 4, datax_size + 3 * 4);
  memcpy (map.data + 8, "AVIX", 4);
  memcpy (map.data + 12, "LIST", 4);
  GST_WRITE_UINT32_LE (map.data + 16, datax_size);
  memcpy (map.data + 20, "movi", 4);
  gst_buffer_unmap (buffer, &map);

  return buffer;
}

static inline GstBuffer *
gst_avi_mux_riff_get_header (GstAviPad * avipad, guint32 video_frame_size)
{
  GstBuffer *buffer;
  GstMapInfo map;

  buffer = gst_buffer_new_and_alloc (8);

  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  memcpy (map.data + 0, avipad->tag, 4);
  GST_WRITE_UINT32_LE (map.data + 4, video_frame_size);
  gst_buffer_unmap (buffer, &map);

  return buffer;
}

/* write an odml index chunk in the movi list */
static GstFlowReturn
gst_avi_mux_write_avix_index (GstAviMux * avimux, GstAviPad * avipad,
    gchar * code, gchar * chunk, gst_avi_superindex_entry * super_index,
    gint * super_index_count)
{
  GstFlowReturn res;
  GstBuffer *buffer;
  guint8 *data;
  gst_riff_index_entry *entry;
  gint i;
  guint32 size, entry_count;
  gboolean is_pcm = FALSE;
  guint32 pcm_samples = 0;
  GstMapInfo map;

  /* check if it is pcm */
  if (avipad && !avipad->is_video) {
    GstAviAudioPad *audiopad = (GstAviAudioPad *) avipad;
    if (audiopad->auds.format == GST_RIFF_WAVE_FORMAT_PCM) {
      pcm_samples = audiopad->samples;
      is_pcm = TRUE;
    }
  }

  /* allocate the maximum possible */
  buffer = gst_buffer_new_and_alloc (32 + 8 * avimux->idx_index);

  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  data = map.data;

  /* general index chunk info */
  memcpy (map.data + 0, chunk, 4);      /* chunk id */
  GST_WRITE_UINT32_LE (map.data + 4, 0);        /* chunk size; fill later */
  GST_WRITE_UINT16_LE (map.data + 8, 2);        /* index entry is 2 words */
  map.data[10] = 0;             /* index subtype */
  map.data[11] = GST_AVI_INDEX_OF_CHUNKS;       /* index type: AVI_INDEX_OF_CHUNKS */
  GST_WRITE_UINT32_LE (map.data + 12, 0);       /* entries in use; fill later */
  memcpy (map.data + 16, code, 4);      /* stream to which index refers */
  GST_WRITE_UINT64_LE (map.data + 20, avimux->avix_start);      /* base offset */
  GST_WRITE_UINT32_LE (map.data + 28, 0);       /* reserved */
  map.data += 32;

  /* now the actual index entries */
  i = avimux->idx_index;
  entry = avimux->idx;
  while (i > 0) {
    if (memcmp (&entry->id, code, 4) == 0) {
      /* enter relative offset to the data (!) */
      GST_WRITE_UINT32_LE (map.data, GUINT32_FROM_LE (entry->offset) + 8);
      /* msb is set if not (!) keyframe */
      GST_WRITE_UINT32_LE (map.data + 4, GUINT32_FROM_LE (entry->size)
          | (GUINT32_FROM_LE (entry->flags)
              & GST_RIFF_IF_KEYFRAME ? 0 : 1U << 31));
      map.data += 8;
    }
    i--;
    entry++;
  }

  /* ok, now we know the size and no of entries, fill in where needed */
  size = map.data - data;
  GST_WRITE_UINT32_LE (data + 4, size - 8);
  entry_count = (size - 32) / 8;
  GST_WRITE_UINT32_LE (data + 12, entry_count);
  gst_buffer_unmap (buffer, &map);
  gst_buffer_resize (buffer, 0, size);

  /* send */
  if ((res = gst_pad_push (avimux->srcpad, buffer)) != GST_FLOW_OK)
    return res;

  /* keep track of this in superindex (if room) ... */
  if (*super_index_count < GST_AVI_SUPERINDEX_COUNT) {
    i = *super_index_count;
    super_index[i].offset = GUINT64_TO_LE (avimux->total_data);
    super_index[i].size = GUINT32_TO_LE (size);
    if (is_pcm) {
      super_index[i].duration = GUINT32_TO_LE (pcm_samples);
    } else {
      super_index[i].duration = GUINT32_TO_LE (entry_count);
    }
    (*super_index_count)++;
  } else
    GST_WARNING_OBJECT (avimux, "No more room in superindex of stream %s",
        code);

  /* ... and in size */
  avimux->total_data += size;
  if (avimux->is_bigfile)
    avimux->datax_size += size;
  else
    avimux->data_size += size;

  return GST_FLOW_OK;
}

/* some other usable functions (thankyou xawtv ;-) ) */

static void
gst_avi_mux_add_index (GstAviMux * avimux, GstAviPad * avipad, guint32 flags,
    guint32 size)
{
  gchar *code = avipad->tag;
  if (avimux->idx_index == avimux->idx_count) {
    avimux->idx_count += 256;
    avimux->idx =
        g_realloc (avimux->idx,
        avimux->idx_count * sizeof (gst_riff_index_entry));
  }

  /* in case of pcm audio, we need to count the number of samples for
   * putting in the indx entries */
  if (!avipad->is_video) {
    GstAviAudioPad *audiopad = (GstAviAudioPad *) avipad;
    if (audiopad->auds.format == GST_RIFF_WAVE_FORMAT_PCM) {
      audiopad->samples += size / audiopad->auds.blockalign;
    }
  }

  memcpy (&(avimux->idx[avimux->idx_index].id), code, 4);
  avimux->idx[avimux->idx_index].flags = GUINT32_TO_LE (flags);
  avimux->idx[avimux->idx_index].offset = GUINT32_TO_LE (avimux->idx_offset);
  avimux->idx[avimux->idx_index].size = GUINT32_TO_LE (size);
  avimux->idx_index++;
}

static GstFlowReturn
gst_avi_mux_write_index (GstAviMux * avimux)
{
  GstFlowReturn res;
  GstBuffer *buffer;
  GstMapInfo map;
  guint8 *data;
  gsize size;

  buffer = gst_buffer_new_and_alloc (8);

  gst_buffer_map (buffer, &map, GST_MAP_WRITE);
  memcpy (map.data + 0, "idx1", 4);
  GST_WRITE_UINT32_LE (map.data + 4,
      avimux->idx_index * sizeof (gst_riff_index_entry));
  gst_buffer_unmap (buffer, &map);

  res = gst_pad_push (avimux->srcpad, buffer);
  if (res != GST_FLOW_OK)
    return res;

  buffer = gst_buffer_new ();

  size = avimux->idx_index * sizeof (gst_riff_index_entry);
  data = (guint8 *) avimux->idx;
  avimux->idx = NULL;           /* will be free()'ed by gst_buffer_unref() */

  gst_buffer_append_memory (buffer,
      gst_memory_new_wrapped (0, data, size, 0, size, data, g_free));

  avimux->total_data += size + 8;

  res = gst_pad_push (avimux->srcpad, buffer);
  if (res != GST_FLOW_OK)
    return res;

  avimux->idx_size += avimux->idx_index * sizeof (gst_riff_index_entry) + 8;

  /* update header */
  avimux->avi_hdr.flags |= GST_RIFF_AVIH_HASINDEX;
  return GST_FLOW_OK;
}

static GstFlowReturn
gst_avi_mux_bigfile (GstAviMux * avimux, gboolean last)
{
  GstFlowReturn res = GST_FLOW_OK;
  GstBuffer *header;
  GSList *node;

  /* first some odml standard index chunks in the movi list */
  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    node = node->next;

    res = gst_avi_mux_write_avix_index (avimux, avipad, avipad->tag,
        avipad->idx_tag, avipad->idx, &avipad->idx_index);
    if (res != GST_FLOW_OK)
      return res;
  }

  if (avimux->is_bigfile) {
    GstSegment segment;

    gst_segment_init (&segment, GST_FORMAT_BYTES);

    /* search back */
    segment.start = avimux->avix_start;
    segment.time = avimux->avix_start;
    gst_pad_push_event (avimux->srcpad, gst_event_new_segment (&segment));

    /* rewrite AVIX header */
    header = gst_avi_mux_riff_get_avix_header (avimux->datax_size);
    res = gst_pad_push (avimux->srcpad, header);

    /* go back to current location, at least try */
    segment.start = avimux->total_data;
    segment.time = avimux->total_data;
    gst_pad_push_event (avimux->srcpad, gst_event_new_segment (&segment));

    if (res != GST_FLOW_OK)
      return res;
  } else {                      /* write a standard index in the first riff chunk */
    res = gst_avi_mux_write_index (avimux);
    /* the index data/buffer is freed by pushing it */
    avimux->idx_count = 0;
    if (res != GST_FLOW_OK)
      return res;
  }

  avimux->avix_start = avimux->total_data;

  if (last)
    return res;

  avimux->is_bigfile = TRUE;
  avimux->numx_frames = 0;
  avimux->datax_size = 4;       /* movi tag */
  avimux->idx_index = 0;
  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;
    node = node->next;
    if (!avipad->is_video) {
      GstAviAudioPad *audiopad = (GstAviAudioPad *) avipad;
      audiopad->samples = 0;
    }
  }

  header = gst_avi_mux_riff_get_avix_header (0);
  avimux->total_data += gst_buffer_get_size (header);
  /* avix_start is used as base offset for the odml index chunk */
  avimux->idx_offset = avimux->total_data - avimux->avix_start;

  return gst_pad_push (avimux->srcpad, header);
}

/* enough header blabla now, let's go on to actually writing the headers */

static GstFlowReturn
gst_avi_mux_start_file (GstAviMux * avimux)
{
  GstFlowReturn res;
  GstBuffer *header;
  GSList *node;
  GstCaps *caps;
  GstSegment segment;

  avimux->total_data = 0;
  avimux->total_frames = 0;
  avimux->data_size = 4;        /* movi tag */
  avimux->datax_size = 0;
  avimux->num_frames = 0;
  avimux->numx_frames = 0;
  avimux->avix_start = 0;

  avimux->idx_index = 0;
  avimux->idx_offset = 0;       /* see 10 lines below */
  avimux->idx_size = 0;
  avimux->idx_count = 0;
  avimux->idx = NULL;

  /* state */
  avimux->write_header = FALSE;
  avimux->restart = FALSE;

  /* init streams, see what we've got */
  node = avimux->sinkpads;
  avimux->audio_pads = avimux->video_pads = 0;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    node = node->next;

    if (!avipad->is_video) {
      /* audio stream numbers must start at 1 iff there is a video stream 0;
       * request_pad inserts video pad at head of list, so this test suffices */
      if (avimux->video_pads)
        avimux->audio_pads++;
      avipad->tag = g_strdup_printf ("%02uwb", avimux->audio_pads);
      avipad->idx_tag = g_strdup_printf ("ix%02u", avimux->audio_pads);
      if (!avimux->video_pads)
        avimux->audio_pads++;
    } else {
      avipad->tag = g_strdup_printf ("%02udb", avimux->video_pads);
      avipad->idx_tag = g_strdup_printf ("ix%02u", avimux->video_pads++);
    }
  }

  /* stream-start (FIXME: create id based on input ids) */
  {
    gchar s_id[32];

    g_snprintf (s_id, sizeof (s_id), "avimux-%08x", g_random_int ());
    gst_pad_push_event (avimux->srcpad, gst_event_new_stream_start (s_id));
  }

  caps = gst_pad_get_pad_template_caps (avimux->srcpad);
  gst_pad_set_caps (avimux->srcpad, caps);
  gst_caps_unref (caps);

  /* let downstream know we think in BYTES and expect to do seeking later on */
  gst_segment_init (&segment, GST_FORMAT_BYTES);
  gst_pad_push_event (avimux->srcpad, gst_event_new_segment (&segment));

  /* header */
  avimux->avi_hdr.streams = g_slist_length (avimux->sinkpads);
  avimux->is_bigfile = FALSE;

  header = gst_avi_mux_riff_get_avi_header (avimux);
  avimux->total_data += gst_buffer_get_size (header);

  res = gst_pad_push (avimux->srcpad, header);

  avimux->idx_offset = avimux->total_data;

  return res;
}

static GstFlowReturn
gst_avi_mux_stop_file (GstAviMux * avimux)
{
  GstFlowReturn res = GST_FLOW_OK;
  GstBuffer *header;
  GSList *node;
  GstSegment segment;

  /* Do not write index and header, if the index has no data */
  if (avimux->idx == NULL)
    return GST_FLOW_OK;

  /* if bigfile, rewrite header, else write indexes */
  /* don't bail out at once if error, still try to re-write header */
  if (avimux->video_pads > 0) {
    if (avimux->is_bigfile) {
      res = gst_avi_mux_bigfile (avimux, TRUE);
    } else {
      res = gst_avi_mux_write_index (avimux);
    }
  }

  /* we do our best to make it interleaved at least ... */
  if (avimux->audio_pads > 0 && avimux->video_pads > 0)
    avimux->avi_hdr.flags |= GST_RIFF_AVIH_ISINTERLEAVED;

  /* set rate and everything having to do with that */
  avimux->avi_hdr.max_bps = 0;
  node = avimux->sinkpads;
  while (node) {
    GstAviPad *avipad = (GstAviPad *) node->data;

    node = node->next;

    if (!avipad->is_video) {
      GstAviAudioPad *audpad = (GstAviAudioPad *) avipad;

      /* calculate bps if needed */
      if (!audpad->auds.av_bps) {
        if (audpad->audio_time) {
          audpad->auds.av_bps =
              (GST_SECOND * audpad->audio_size) / audpad->audio_time;
          /* round bps to nearest multiple of 8;
           * which is much more likely to be the (cbr) bitrate in use;
           * which in turn results in better timestamp calculation on playback */
          audpad->auds.av_bps = GST_ROUND_UP_8 (audpad->auds.av_bps - 4);
        } else {
          GST_ELEMENT_WARNING (avimux, STREAM, MUX,
              (_("No or invalid input audio, AVI stream will be corrupt.")),
              (NULL));
          audpad->auds.av_bps = 0;
        }
      }
      /* housekeeping for vbr case */
      if (audpad->max_audio_chunk)
        audpad->auds.blockalign = audpad->max_audio_chunk;
      if (audpad->auds.blockalign == 0)
        audpad->auds.blockalign = 1;
      /* note that hdr.rate is actually used by demux in cbr case */
      if (avipad->hdr.scale <= 1)
        avipad->hdr.rate = audpad->auds.av_bps / audpad->auds.blockalign;
      avimux->avi_hdr.max_bps += audpad->auds.av_bps;
      avipad->hdr.length = gst_util_uint64_scale (audpad->audio_time,
          avipad->hdr.rate, avipad->hdr.scale * GST_SECOND);
    } else {
      GstAviVideoPad *vidpad = (GstAviVideoPad *) avipad;

      avimux->avi_hdr.max_bps += ((vidpad->vids.bit_cnt + 7) / 8) *
          (1000000. / avimux->avi_hdr.us_frame) * vidpad->vids.image_size;
      avipad->hdr.length = avimux->total_frames;
    }
  }

  /* statistics/total_frames/... */
  avimux->avi_hdr.tot_frames = avimux->num_frames;

  /* seek and rewrite the header */
  gst_segment_init (&segment, GST_FORMAT_BYTES);
  gst_pad_push_event (avimux->srcpad, gst_event_new_segment (&segment));

  /* the first error survives */
  header = gst_avi_mux_riff_get_avi_header (avimux);
  if (res == GST_FLOW_OK)
    res = gst_pad_push (avimux->srcpad, header);
  else
    gst_pad_push (avimux->srcpad, header);

  segment.start = avimux->total_data;
  segment.time = avimux->total_data;
  gst_pad_push_event (avimux->srcpad, gst_event_new_segment (&segment));

  avimux->write_header = TRUE;

  return res;
}

static GstFlowReturn
gst_avi_mux_restart_file (GstAviMux * avimux)
{
  GstFlowReturn res;

  if ((res = gst_avi_mux_stop_file (avimux)) != GST_FLOW_OK)
    return res;

  gst_pad_push_event (avimux->srcpad, gst_event_new_eos ());

  return gst_avi_mux_start_file (avimux);
}

/* handle events (search) */
static gboolean
gst_avi_mux_handle_event (GstCollectPads * pads, GstCollectData * data,
    GstEvent * event, gpointer user_data)
{
  GstAviMux *avimux;
  gboolean ret = TRUE;

  avimux = GST_AVI_MUX (user_data);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      GstAviCollectData *collect_pad;
      GstAviVideoPad *avipad;

      gst_event_parse_caps (event, &caps);

      /* find stream data */
      collect_pad = (GstAviCollectData *) data;
      g_assert (collect_pad);
      avipad = (GstAviVideoPad *) collect_pad->avipad;
      g_assert (avipad);

      if (avipad->parent.is_video) {
        ret = gst_avi_mux_vidsink_set_caps (data->pad, caps);
      } else {
        ret = gst_avi_mux_audsink_set_caps (data->pad, caps);
      }
      gst_event_unref (event);
      event = NULL;
      break;
    }
    case GST_EVENT_TAG:{
      GstTagList *list;
      GstTagSetter *setter = GST_TAG_SETTER (avimux);
      const GstTagMergeMode mode = gst_tag_setter_get_tag_merge_mode (setter);

      gst_event_parse_tag (event, &list);
      gst_tag_setter_merge_tags (setter, list, mode);
      gst_event_unref (event);
      event = NULL;
      break;
    }
    default:
      break;
  }

  if (event != NULL)
    return gst_collect_pads_event_default (pads, data, event, FALSE);

  return ret;
}

/* send extra 'padding' data */
static GstFlowReturn
gst_avi_mux_send_pad_data (GstAviMux * avimux, gulong num_bytes)
{
  GstBuffer *buffer;

  buffer = gst_buffer_new_and_alloc (num_bytes);
  gst_buffer_memset (buffer, 0, 0, num_bytes);

  return gst_pad_push (avimux->srcpad, buffer);
}

#define gst_avi_mux_is_uncompressed(fourcc)		\
  (fourcc == GST_RIFF_DIB ||				\
   fourcc == GST_RIFF_rgb ||				\
   fourcc == GST_RIFF_RGB || fourcc == GST_RIFF_RAW)

/*
 * Helper for gst_avi_demux_invert()
 */
static inline void
swap_line (guint8 * d1, guint8 * d2, guint8 * tmp, gint bytes)
{
  memcpy (tmp, d1, bytes);
  memcpy (d1, d2, bytes);
  memcpy (d2, tmp, bytes);
}

/*
 * Invert DIB buffers... Takes existing buffer and
 * returns either the buffer or a new one (with old
 * one dereferenced).
 * FFMPEG does this by simply negating the height in the header. Should we?
 * FIXME: can't we preallocate tmp? and remember stride, bpp?
 *        this could be done in do_one_buffer() I suppose
 */
static GstBuffer *
gst_avi_mux_invert (GstAviPad * avipad, GstBuffer * buf)
{
  gint y, w, h;
  gint bpp, stride;
  guint8 *tmp = NULL;
  GstMapInfo map;

  GstAviVideoPad *vidpad = (GstAviVideoPad *) avipad;

  h = vidpad->vids.height;
  w = vidpad->vids.width;
  bpp = vidpad->vids.bit_cnt ? vidpad->vids.bit_cnt : 8;
  stride = GST_ROUND_UP_4 (w * (bpp / 8));

  buf = gst_buffer_make_writable (buf);

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  if (map.size < (stride * h)) {
    GST_WARNING ("Buffer is smaller than reported Width x Height x Depth");
    gst_buffer_unmap (buf, &map);
    return buf;
  }

  tmp = g_malloc (stride);

  for (y = 0; y < h / 2; y++) {
    swap_line (map.data + stride * y, map.data + stride * (h - 1 - y), tmp,
        stride);
  }

  g_free (tmp);

  gst_buffer_unmap (buf, &map);

  return buf;
}

/* do buffer */
static GstFlowReturn
gst_avi_mux_do_buffer (GstAviMux * avimux, GstAviPad * avipad)
{
  GstFlowReturn res;
  GstBuffer *data, *header;
  gulong total_size, pad_bytes = 0;
  guint flags;
  gsize datasize;
  GstClockTime time;

  data = gst_collect_pads_pop (avimux->collect, avipad->collect);
  /* arrange downstream running time */
  time = gst_segment_to_running_time (&avipad->collect->segment,
      GST_FORMAT_TIME, GST_BUFFER_TIMESTAMP (data));
  if (time != GST_BUFFER_TIMESTAMP (data)) {
    data = gst_buffer_make_writable (data);
    GST_BUFFER_TIMESTAMP (data) = time;
  }

  /* Prepend a special buffer to the first one for some formats */
  if (avipad->is_video) {
    GstAviVideoPad *vidpad = (GstAviVideoPad *) avipad;

    if (vidpad->prepend_buffer) {
      /* Keep a reference to data until we copy the timestamps, then release it */
      GstBuffer *newdata =
          gst_buffer_append (vidpad->prepend_buffer, gst_buffer_ref (data));
      gst_buffer_copy_into (newdata, data, GST_BUFFER_COPY_TIMESTAMPS, 0, -1);
      gst_buffer_unref (data);

      data = newdata;
      vidpad->prepend_buffer = NULL;
    }

    /* DIB buffers are stored topdown (I don't know why) */
    if (gst_avi_mux_is_uncompressed (avipad->hdr.fcc_handler)) {
      data = gst_avi_mux_invert (avipad, data);
    }
  }

  if (avimux->restart) {
    if ((res = gst_avi_mux_restart_file (avimux)) != GST_FLOW_OK)
      goto done;
  }

  datasize = gst_buffer_get_size (data);

  /* need to restart or start a next avix chunk ? */
  if ((avimux->is_bigfile ? avimux->datax_size : avimux->data_size) +
      datasize > GST_AVI_MAX_SIZE) {
    if (avimux->enable_large_avi) {
      if ((res = gst_avi_mux_bigfile (avimux, FALSE)) != GST_FLOW_OK)
        goto done;
    } else {
      if ((res = gst_avi_mux_restart_file (avimux)) != GST_FLOW_OK)
        goto done;
    }
  }

  /* get header and record some stats */
  if (datasize & 1) {
    pad_bytes = 2 - (datasize & 1);
  }
  header = gst_avi_mux_riff_get_header (avipad, datasize);
  total_size = gst_buffer_get_size (header) + datasize + pad_bytes;

  if (avimux->is_bigfile) {
    avimux->datax_size += total_size;
  } else {
    avimux->data_size += total_size;
  }

  if (G_UNLIKELY (avipad->hook)) {
    gst_buffer_ref (data);
    avipad->hook (avimux, avipad, data);
  }

  /* the suggested buffer size is the max frame size */
  if (avipad->hdr.bufsize < datasize)
    avipad->hdr.bufsize = datasize;

  if (avipad->is_video) {
    avimux->total_frames++;

    if (avimux->is_bigfile) {
      avimux->numx_frames++;
    } else {
      avimux->num_frames++;
    }

    flags = 0x02;
    if (!GST_BUFFER_FLAG_IS_SET (data, GST_BUFFER_FLAG_DELTA_UNIT))
      flags |= 0x10;
  } else {
    GstAviAudioPad *audpad = (GstAviAudioPad *) avipad;

    flags = 0;
    audpad->audio_size += datasize;
    audpad->audio_time += GST_BUFFER_DURATION (data);
    if (audpad->max_audio_chunk && datasize > audpad->max_audio_chunk)
      audpad->max_audio_chunk = datasize;
  }

  gst_avi_mux_add_index (avimux, avipad, flags, datasize);

  /* send buffers */
  GST_LOG_OBJECT (avimux, "pushing buffers: head, data");

  if ((res = gst_pad_push (avimux->srcpad, header)) != GST_FLOW_OK)
    goto done;

  gst_buffer_ref (data);
  if ((res = gst_pad_push (avimux->srcpad, data)) != GST_FLOW_OK)
    goto done;

  if (pad_bytes) {
    if ((res = gst_avi_mux_send_pad_data (avimux, pad_bytes)) != GST_FLOW_OK)
      goto done;
  }

  /* if any push above fails, we're in trouble with file consistency anyway */
  avimux->total_data += total_size;
  avimux->idx_offset += total_size;

done:
  gst_buffer_unref (data);
  return res;
}

/* pick the oldest buffer from the pads and push it */
static GstFlowReturn
gst_avi_mux_do_one_buffer (GstAviMux * avimux)
{
  GstAviPad *avipad, *best_pad;
  GSList *node;
  GstBuffer *buffer;
  GstClockTime time, best_time, delay;

  node = avimux->sinkpads;
  best_pad = NULL;
  best_time = GST_CLOCK_TIME_NONE;
  for (; node; node = node->next) {
    avipad = (GstAviPad *) node->data;

    if (!avipad->collect)
      continue;

    buffer = gst_collect_pads_peek (avimux->collect, avipad->collect);
    if (!buffer)
      continue;
    time = GST_BUFFER_TIMESTAMP (buffer);
    gst_buffer_unref (buffer);

    /* invalid should pass */
    if (G_LIKELY (GST_CLOCK_TIME_IS_VALID (time))) {
      time = gst_segment_to_running_time (&avipad->collect->segment,
          GST_FORMAT_TIME, time);
      if (G_UNLIKELY (!GST_CLOCK_TIME_IS_VALID (time))) {
        GST_DEBUG_OBJECT (avimux, "clipping buffer on pad %s outside segment",
            GST_PAD_NAME (avipad->collect->pad));
        buffer = gst_collect_pads_pop (avimux->collect, avipad->collect);
        gst_buffer_unref (buffer);
        return GST_FLOW_OK;
      }
    }

    delay = avipad->is_video ? GST_SECOND / 2 : 0;

    /* invalid timestamp buffers pass first,
     * these are probably initialization buffers */
    if (best_pad == NULL || !GST_CLOCK_TIME_IS_VALID (time)
        || (GST_CLOCK_TIME_IS_VALID (best_time) && time + delay < best_time)) {
      best_pad = avipad;
      best_time = time + delay;
    }
  }

  if (best_pad) {
    GST_LOG_OBJECT (avimux, "selected pad %s with time %" GST_TIME_FORMAT,
        GST_PAD_NAME (best_pad->collect->pad), GST_TIME_ARGS (best_time));

    return gst_avi_mux_do_buffer (avimux, best_pad);
  } else {
    /* simply finish off the file and send EOS */
    gst_avi_mux_stop_file (avimux);
    gst_pad_push_event (avimux->srcpad, gst_event_new_eos ());
    return GST_FLOW_EOS;
  }

}

static GstFlowReturn
gst_avi_mux_collect_pads (GstCollectPads * pads, GstAviMux * avimux)
{
  GstFlowReturn res;

  if (G_UNLIKELY (avimux->write_header)) {
    if ((res = gst_avi_mux_start_file (avimux)) != GST_FLOW_OK)
      return res;
  }

  return gst_avi_mux_do_one_buffer (avimux);
}


static void
gst_avi_mux_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstAviMux *avimux;

  avimux = GST_AVI_MUX (object);

  switch (prop_id) {
    case PROP_BIGFILE:
      g_value_set_boolean (value, avimux->enable_large_avi);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_avi_mux_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstAviMux *avimux;

  avimux = GST_AVI_MUX (object);

  switch (prop_id) {
    case PROP_BIGFILE:
      avimux->enable_large_avi = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_avi_mux_change_state (GstElement * element, GstStateChange transition)
{
  GstAviMux *avimux;
  GstStateChangeReturn ret;

  avimux = GST_AVI_MUX (element);

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      gst_collect_pads_start (avimux->collect);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_collect_pads_stop (avimux->collect);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto done;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_avi_mux_reset (avimux);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

done:
  return ret;
}
