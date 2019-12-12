/* gstgoom.c: implementation of goom drawing element
 * Copyright (C) <2001> Richard Boulton <richard@tartarus.org>
 *           (C) <2006> Wim Taymans <wim at fluendo dot com>
 *           (C) <2011> Wim Taymans <wim.taymans at gmail dot com>
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
 * SECTION:element-goom
 * @title: goom
 * @see_also: synaesthesia
 *
 * Goom is an audio visualisation element. It creates warping structures
 * based on the incoming audio signal.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 -v audiotestsrc ! goom ! videoconvert ! xvimagesink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstgoom.h"
#include "goom.h"

#if HAVE_ORC
#include <orc/orc.h>
#endif

GST_DEBUG_CATEGORY (goom_debug);
#define GST_CAT_DEFAULT goom_debug

#define DEFAULT_WIDTH  320
#define DEFAULT_HEIGHT 240

/* signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0
      /* FILL ME */
};

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


static void gst_goom_finalize (GObject * object);

static gboolean gst_goom_setup (GstAudioVisualizer * base);
static gboolean gst_goom_render (GstAudioVisualizer * base, GstBuffer * audio,
    GstVideoFrame * video);

G_DEFINE_TYPE (GstGoom, gst_goom, GST_TYPE_AUDIO_VISUALIZER);

static void
gst_goom_class_init (GstGoomClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstAudioVisualizerClass *visualizer_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  visualizer_class = (GstAudioVisualizerClass *) klass;

  gobject_class->finalize = gst_goom_finalize;

  gst_element_class_set_static_metadata (gstelement_class, "GOOM: what a GOOM!",
      "Visualization",
      "Takes frames of data and outputs video frames using the GOOM filter",
      "Wim Taymans <wim@fluendo.com>");
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);
  gst_element_class_add_static_pad_template (gstelement_class, &src_template);

  visualizer_class->setup = GST_DEBUG_FUNCPTR (gst_goom_setup);
  visualizer_class->render = GST_DEBUG_FUNCPTR (gst_goom_render);
}

static void
gst_goom_init (GstGoom * goom)
{
  goom->width = DEFAULT_WIDTH;
  goom->height = DEFAULT_HEIGHT;
  goom->channels = 0;

  goom->plugin = goom_init (goom->width, goom->height);
}

static void
gst_goom_finalize (GObject * object)
{
  GstGoom *goom = GST_GOOM (object);

  goom_close (goom->plugin);
  goom->plugin = NULL;

  G_OBJECT_CLASS (gst_goom_parent_class)->finalize (object);
}

static gboolean
gst_goom_setup (GstAudioVisualizer * base)
{
  GstGoom *goom = GST_GOOM (base);

  goom->width = GST_VIDEO_INFO_WIDTH (&base->vinfo);
  goom->height = GST_VIDEO_INFO_HEIGHT (&base->vinfo);
  goom_set_resolution (goom->plugin, goom->width, goom->height);

  return TRUE;
}

static gboolean
gst_goom_render (GstAudioVisualizer * base, GstBuffer * audio,
    GstVideoFrame * video)
{
  GstGoom *goom = GST_GOOM (base);
  GstMapInfo amap;
  gint16 datain[2][GOOM_SAMPLES];
  gint16 *adata;
  gint i;

  /* get next GOOM_SAMPLES, we have at least this amount of samples */
  gst_buffer_map (audio, &amap, GST_MAP_READ);
  adata = (gint16 *) amap.data;

  if (goom->channels == 2) {
    for (i = 0; i < GOOM_SAMPLES; i++) {
      datain[0][i] = *adata++;
      datain[1][i] = *adata++;
    }
  } else {
    for (i = 0; i < GOOM_SAMPLES; i++) {
      datain[0][i] = *adata;
      datain[1][i] = *adata++;
    }
  }

  video->data[0] = goom_update (goom->plugin, datain, 0, 0);
  gst_buffer_unmap (audio, &amap);

  return TRUE;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (goom_debug, "goom", 0, "goom visualisation element");

#if HAVE_ORC
  orc_init ();
#endif

  return gst_element_register (plugin, "goom", GST_RANK_NONE, GST_TYPE_GOOM);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    goom,
    "GOOM visualization filter",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
