/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
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
 * SECTION:element-cacasink
 * @title: cacasink
 * @see_also: #GstAASink
 *
 * Displays video as color ascii art.
 *
 * ## Example launch line
 * |[
 * CACA_GEOMETRY=160x60 CACA_FONT=5x7 gst-launch-1.0 filesrc location=test.avi ! decodebin ! videoconvert ! cacasink
 * ]| This pipeline renders a video to ascii art into a separate window using a
 * small font and specifying the ascii resolution.
 * |[
 * CACA_DRIVER=ncurses gst-launch-1.0 filesrc location=test.avi ! decodebin ! videoconvert ! cacasink
 * ]| This pipeline renders a video to ascii art into the current terminal.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "gstcacasink.h"


//#define GST_CACA_DEFAULT_RED_MASK R_MASK_32_REVERSE_INT
//#define GST_CACA_DEFAULT_GREEN_MASK G_MASK_32_REVERSE_INT
//#define GST_CACA_DEFAULT_BLUE_MASK B_MASK_32_REVERSE_INT

/* cacasink signals and args */
enum
{
  LAST_SIGNAL
};

#define GST_CACA_DEFAULT_SCREEN_WIDTH 80
#define GST_CACA_DEFAULT_SCREEN_HEIGHT 25
#define GST_CACA_DEFAULT_DITHER CACA_DITHERING_NONE
#define GST_CACA_DEFAULT_ANTIALIASING TRUE

enum
{
  PROP_0,
  PROP_SCREEN_WIDTH,
  PROP_SCREEN_HEIGHT,
  PROP_DITHER,
  PROP_ANTIALIASING
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ RGB, BGR, RGBx, xRGB, BGRx, xBGR, RGB16, RGB15 }"))
    );

static gboolean gst_cacasink_setcaps (GstBaseSink * pad, GstCaps * caps);
static void gst_cacasink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static GstFlowReturn gst_cacasink_render (GstBaseSink * basesink,
    GstBuffer * buffer);

static void gst_cacasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_cacasink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_cacasink_change_state (GstElement * element,
    GstStateChange transition);

#define gst_cacasink_parent_class parent_class
G_DEFINE_TYPE (GstCACASink, gst_cacasink, GST_TYPE_BASE_SINK);

#define GST_TYPE_CACADITHER (gst_cacasink_dither_get_type())
static GType
gst_cacasink_dither_get_type (void)
{
  static GType dither_type = 0;

  static const GEnumValue dither_types[] = {
    {CACA_DITHERING_NONE, "No dithering", "none"},
    {CACA_DITHERING_ORDERED2, "Ordered 2x2 Bayer dithering", "2x2"},
    {CACA_DITHERING_ORDERED4, "Ordered 4x4 Bayer dithering", "4x4"},
    {CACA_DITHERING_ORDERED8, "Ordered 8x8 Bayer dithering", "8x8"},
    {CACA_DITHERING_RANDOM, "Random dithering", "random"},
    {0, NULL, NULL},
  };

  if (!dither_type) {
    dither_type = g_enum_register_static ("GstCACASinkDithering", dither_types);
  }
  return dither_type;
}

static void
gst_cacasink_class_init (GstCACASinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_cacasink_set_property;
  gobject_class->get_property = gst_cacasink_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SCREEN_WIDTH,
      g_param_spec_int ("screen-width", "Screen Width",
          "The width of the screen", 0, G_MAXINT, GST_CACA_DEFAULT_SCREEN_WIDTH,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_SCREEN_HEIGHT,
      g_param_spec_int ("screen-height", "Screen Height",
          "The height of the screen", 0, G_MAXINT,
          GST_CACA_DEFAULT_SCREEN_HEIGHT,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DITHER,
      g_param_spec_enum ("dither", "Dither Type", "Set type of Dither",
          GST_TYPE_CACADITHER, GST_CACA_DEFAULT_DITHER,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_ANTIALIASING,
      g_param_spec_boolean ("anti-aliasing", "Anti Aliasing",
          "Enables Anti-Aliasing", GST_CACA_DEFAULT_ANTIALIASING,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = gst_cacasink_change_state;

  gst_element_class_set_static_metadata (gstelement_class,
      "A colored ASCII art video sink", "Sink/Video",
      "A colored ASCII art videosink", "Zeeshan Ali <zak147@yahoo.com>");
  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);

  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_cacasink_setcaps);
  gstbasesink_class->get_times = GST_DEBUG_FUNCPTR (gst_cacasink_get_times);
  gstbasesink_class->preroll = GST_DEBUG_FUNCPTR (gst_cacasink_render);
  gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_cacasink_render);
}

static void
gst_cacasink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  *start = GST_BUFFER_TIMESTAMP (buffer);
  *end = *start + GST_BUFFER_DURATION (buffer);
}

static gboolean
gst_cacasink_setcaps (GstBaseSink * basesink, GstCaps * caps)
{
  GstCACASink *cacasink;
  GstVideoInfo info;
  guint bpp, red_mask, green_mask, blue_mask;

  cacasink = GST_CACASINK (basesink);

  if (!gst_video_info_from_caps (&info, caps))
    goto caps_error;

  switch (GST_VIDEO_INFO_FORMAT (&info)) {
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR:
    case GST_VIDEO_FORMAT_RGBx:
    case GST_VIDEO_FORMAT_xRGB:
    case GST_VIDEO_FORMAT_BGRx:
    case GST_VIDEO_FORMAT_xBGR:
      bpp = 8 * info.finfo->pixel_stride[0];
      red_mask = 0xff << (8 * info.finfo->poffset[GST_VIDEO_COMP_R]);
      green_mask = 0xff << (8 * info.finfo->poffset[GST_VIDEO_COMP_G]);
      blue_mask = 0xff << (8 * info.finfo->poffset[GST_VIDEO_COMP_B]);
      break;
    case GST_VIDEO_FORMAT_RGB16:
      bpp = 16;
      red_mask = 0xf800;
      green_mask = 0x07e0;
      blue_mask = 0x001f;
      break;
    case GST_VIDEO_FORMAT_RGB15:
      bpp = 16;
      red_mask = 0x7c00;
      green_mask = 0x03e0;
      blue_mask = 0x001f;
      break;
    default:
      goto invalid_format;
  }

  if (cacasink->bitmap) {
    caca_free_bitmap (cacasink->bitmap);
  }
  cacasink->bitmap = caca_create_bitmap (bpp,
      GST_VIDEO_INFO_WIDTH (&info),
      GST_VIDEO_INFO_HEIGHT (&info),
      GST_ROUND_UP_4 (GST_VIDEO_INFO_WIDTH (&info) * bpp / 8),
      red_mask, green_mask, blue_mask, 0);
  if (!cacasink->bitmap)
    goto no_bitmap;

  cacasink->info = info;

  return TRUE;

  /* ERROS */
caps_error:
  {
    GST_ERROR_OBJECT (cacasink, "error parsing caps");
    return FALSE;
  }
invalid_format:
  {
    GST_ERROR_OBJECT (cacasink, "invalid format");
    return FALSE;
  }
no_bitmap:
  {
    GST_ERROR_OBJECT (cacasink, "could not create bitmap");
    return FALSE;
  }
}

static void
gst_cacasink_init (GstCACASink * cacasink)
{
  cacasink->screen_width = GST_CACA_DEFAULT_SCREEN_WIDTH;
  cacasink->screen_height = GST_CACA_DEFAULT_SCREEN_HEIGHT;

  cacasink->dither = GST_CACA_DEFAULT_DITHER;
  cacasink->antialiasing = GST_CACA_DEFAULT_ANTIALIASING;
}

static GstFlowReturn
gst_cacasink_render (GstBaseSink * basesink, GstBuffer * buffer)
{
  GstCACASink *cacasink = GST_CACASINK (basesink);
  GstVideoFrame frame;

  GST_DEBUG ("render");

  if (!gst_video_frame_map (&frame, &cacasink->info, buffer, GST_MAP_READ))
    goto invalid_frame;

  caca_clear ();
  caca_draw_bitmap (0, 0, cacasink->screen_width - 1,
      cacasink->screen_height - 1, cacasink->bitmap,
      GST_VIDEO_FRAME_PLANE_DATA (&frame, 0));
  caca_refresh ();

  gst_video_frame_unmap (&frame);

  return GST_FLOW_OK;

  /* ERRORS */
invalid_frame:
  {
    GST_ERROR_OBJECT (cacasink, "invalid frame received");
    return GST_FLOW_ERROR;
  }
}

static void
gst_cacasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCACASink *cacasink;

  g_return_if_fail (GST_IS_CACASINK (object));

  cacasink = GST_CACASINK (object);

  switch (prop_id) {
    case PROP_DITHER:{
      cacasink->dither = g_value_get_enum (value);
      caca_set_dithering (cacasink->dither + CACA_DITHERING_NONE);
      break;
    }
    case PROP_ANTIALIASING:{
      cacasink->antialiasing = g_value_get_boolean (value);
      if (cacasink->antialiasing) {
        caca_set_feature (CACA_ANTIALIASING_MAX);
      } else {
        caca_set_feature (CACA_ANTIALIASING_MIN);
      }
      break;
    }
    default:
      break;
  }
}

static void
gst_cacasink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstCACASink *cacasink;

  cacasink = GST_CACASINK (object);

  switch (prop_id) {
    case PROP_SCREEN_WIDTH:{
      g_value_set_int (value, cacasink->screen_width);
      break;
    }
    case PROP_SCREEN_HEIGHT:{
      g_value_set_int (value, cacasink->screen_height);
      break;
    }
    case PROP_DITHER:{
      g_value_set_enum (value, cacasink->dither);
      break;
    }
    case PROP_ANTIALIASING:{
      g_value_set_boolean (value, cacasink->antialiasing);
      break;
    }
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static gboolean
gst_cacasink_open (GstCACASink * cacasink)
{
  cacasink->bitmap = NULL;

  if (caca_init () < 0)
    goto init_failed;

  cacasink->screen_width = caca_get_width ();
  cacasink->screen_height = caca_get_height ();
  cacasink->antialiasing = TRUE;
  caca_set_feature (CACA_ANTIALIASING_MAX);
  cacasink->dither = 0;
  caca_set_dithering (CACA_DITHERING_NONE);

  return TRUE;

  /* ERRORS */
init_failed:
  {
    GST_ELEMENT_ERROR (cacasink, RESOURCE, OPEN_WRITE, (NULL),
        ("caca_init() failed"));
    return FALSE;
  }
}

static void
gst_cacasink_close (GstCACASink * cacasink)
{
  if (cacasink->bitmap) {
    caca_free_bitmap (cacasink->bitmap);
    cacasink->bitmap = NULL;
  }
  caca_end ();
}

static GstStateChangeReturn
gst_cacasink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (!gst_cacasink_open (GST_CACASINK (element)))
        return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_cacasink_close (GST_CACASINK (element));
      break;
    default:
      break;
  }
  return ret;
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "cacasink", GST_RANK_NONE,
          GST_TYPE_CACASINK))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cacasink,
    "Colored ASCII Art video sink",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
