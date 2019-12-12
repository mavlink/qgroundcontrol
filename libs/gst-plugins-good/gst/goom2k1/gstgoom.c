/* gstgoom.c: implementation of goom drawing element
 * Copyright (C) <2001> Richard Boulton <richard@tartarus.org>
 *           (C) <2006> Wim Taymans <wim at fluendo dot com>
 *           (C) <2015> Luis de Bethencourt <luis@debethencourt.com>
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
 * SECTION:element-goom2k1
 * @title: goom2k1
 * @see_also: goom, synaesthesia
 *
 * Goom2k1 is an audio visualisation element. It creates warping structures
 * based on the incoming audio signal. Goom2k1 is the older version of the
 * visualisation. Also available is goom2k4, with a different look.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v audiotestsrc ! goom2k1 ! videoconvert ! xvimagesink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstgoom.h"
#include "goom_core.h"

GST_DEBUG_CATEGORY_STATIC (goom2k1_debug);
#define GST_CAT_DEFAULT goom2k1_debug

#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240
#define DEFAULT_FPS_N  25
#define DEFAULT_FPS_D  1

#if G_BYTE_ORDER == G_BIG_ENDIAN
#define RGB_ORDER "xRGB"
#else
#define RGB_ORDER "BGRx"
#endif

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE (RGB_ORDER))
    );

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",    /* the name of the pads */
    GST_PAD_SINK,               /* type of the pad */
    GST_PAD_ALWAYS,             /* ALWAYS/SOMETIMES */
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "rate = (int) [ 8000, 96000 ], "
        "channels = (int) 1, "
        "layout = (string) interleaved; "
        "audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "rate = (int) [ 8000, 96000 ], "
        "channels = (int) 2, "
        "channel-mask = (bitmask) 0x3, " "layout = (string) interleaved")
    );

static void gst_goom2k1_finalize (GObject * object);

static gboolean gst_goom2k1_setup (GstAudioVisualizer * base);
static gboolean gst_goom2k1_render (GstAudioVisualizer * base,
    GstBuffer * audio, GstVideoFrame * video);


G_DEFINE_TYPE (GstGoom2k1, gst_goom2k1, GST_TYPE_AUDIO_VISUALIZER);

static void
gst_goom2k1_class_init (GstGoom2k1Class * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAudioVisualizerClass *visualizer_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  visualizer_class = (GstAudioVisualizerClass *) klass;

  gobject_class->finalize = gst_goom2k1_finalize;

  gst_element_class_set_static_metadata (gstelement_class,
      "GOOM: what a GOOM! 2k1 edition", "Visualization",
      "Takes frames of data and outputs video frames using the GOOM 2k1 filter",
      "Wim Taymans <wim@fluendo.com>");
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  GST_DEBUG_CATEGORY_INIT (goom2k1_debug, "goom2k1", 0,
      "goom2k1 visualisation element");

  visualizer_class->setup = GST_DEBUG_FUNCPTR (gst_goom2k1_setup);
  visualizer_class->render = GST_DEBUG_FUNCPTR (gst_goom2k1_render);
}

static void
gst_goom2k1_init (GstGoom2k1 * goom)
{
  goom->width = DEFAULT_WIDTH;
  goom->height = DEFAULT_HEIGHT;
  goom->channels = 0;

  goom_init (&(goom->goomdata), goom->width, goom->height);
}

static void
gst_goom2k1_finalize (GObject * object)
{
  GstGoom2k1 *goom = GST_GOOM2K1 (object);

  goom_close (&(goom->goomdata));

  G_OBJECT_CLASS (gst_goom2k1_parent_class)->finalize (object);
}

static gboolean
gst_goom2k1_setup (GstAudioVisualizer * base)
{
  GstGoom2k1 *goom = GST_GOOM2K1 (base);

  goom->width = GST_VIDEO_INFO_WIDTH (&base->vinfo);
  goom->height = GST_VIDEO_INFO_HEIGHT (&base->vinfo);

  goom_set_resolution (&(goom->goomdata), goom->width, goom->height);

  return TRUE;
}

static gboolean
gst_goom2k1_render (GstAudioVisualizer * base, GstBuffer * audio,
    GstVideoFrame * video)
{
  GstGoom2k1 *goom = GST_GOOM2K1 (base);
  GstMapInfo amap;
  gint16 datain[2][GOOM2K1_SAMPLES];
  gint16 *adata;
  gint i;

  /* get next GOOM2K1_SAMPLES, we have at least this amount of samples */
  gst_buffer_map (audio, &amap, GST_MAP_READ);
  adata = (gint16 *) amap.data;

  if (goom->channels == 2) {
    for (i = 0; i < GOOM2K1_SAMPLES; i++) {
      datain[0][i] = *adata++;
      datain[1][i] = *adata++;
    }
  } else {
    for (i = 0; i < GOOM2K1_SAMPLES; i++) {
      datain[0][i] = *adata;
      datain[1][i] = *adata++;
    }
  }

  video->data[0] = goom_update (&(goom->goomdata), datain);
  gst_buffer_unmap (audio, &amap);

  return TRUE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "goom2k1", GST_RANK_NONE,
      GST_TYPE_GOOM2K1);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    goom2k1,
    "GOOM 2k1 visualization filter",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
