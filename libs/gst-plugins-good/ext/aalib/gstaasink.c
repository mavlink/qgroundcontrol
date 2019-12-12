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
 * SECTION:element-aasink
 * @title: aasink
 * @see_also: #GstCACASink
 *
 * Displays video as b/w ascii art.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 filesrc location=test.avi ! decodebin ! videoconvert ! aasink
 * ]| This pipeline renders a video to ascii art into a separate window.
 * |[
 * gst-launch-1.0 filesrc location=test.avi ! decodebin ! videoconvert ! aasink driver=curses
 * ]| This pipeline renders a video to ascii art into the current terminal.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <sys/time.h>

#include <gst/video/gstvideometa.h>
#include "gstaasink.h"

/* aasink signals and args */
enum
{
  LAST_SIGNAL
};


enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_DRIVER,
  PROP_DITHER,
  PROP_BRIGHTNESS,
  PROP_CONTRAST,
  PROP_GAMMA,
  PROP_INVERSION,
  PROP_RANDOMVAL,
  PROP_FRAMES_DISPLAYED,
  PROP_FRAME_TIME
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("I420"))
    );

static GstCaps *gst_aasink_fixate (GstBaseSink * bsink, GstCaps * caps);
static gboolean gst_aasink_setcaps (GstBaseSink * bsink, GstCaps * caps);
static void gst_aasink_get_times (GstBaseSink * bsink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_aasink_propose_allocation (GstBaseSink * bsink,
    GstQuery * query);
static GstFlowReturn gst_aasink_show_frame (GstVideoSink * videosink,
    GstBuffer * buffer);

static void gst_aasink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_aasink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn gst_aasink_change_state (GstElement * element,
    GstStateChange transition);

#define gst_aasink_parent_class parent_class
G_DEFINE_TYPE (GstAASink, gst_aasink, GST_TYPE_VIDEO_SINK);

#define GST_TYPE_AADRIVERS (gst_aasink_drivers_get_type())
static GType
gst_aasink_drivers_get_type (void)
{
  static GType driver_type = 0;

  if (!driver_type) {
    GEnumValue *drivers;
    const struct aa_driver *driver;
    gint n_drivers;
    gint i;

    for (n_drivers = 0; aa_drivers[n_drivers]; n_drivers++) {
      /* count number of drivers */
    }

    drivers = g_new0 (GEnumValue, n_drivers + 1);

    for (i = 0; i < n_drivers; i++) {
      driver = aa_drivers[i];
      drivers[i].value = i;
      drivers[i].value_name = g_strdup (driver->name);
      drivers[i].value_nick = g_utf8_strdown (driver->shortname, -1);
    }
    drivers[i].value = 0;
    drivers[i].value_name = NULL;
    drivers[i].value_nick = NULL;

    driver_type = g_enum_register_static ("GstAASinkDrivers", drivers);
  }
  return driver_type;
}

#define GST_TYPE_AADITHER (gst_aasink_dither_get_type())
static GType
gst_aasink_dither_get_type (void)
{
  static GType dither_type = 0;

  if (!dither_type) {
    GEnumValue *ditherers;
    gint n_ditherers;
    gint i;

    for (n_ditherers = 0; aa_dithernames[n_ditherers]; n_ditherers++) {
      /* count number of ditherers */
    }

    ditherers = g_new0 (GEnumValue, n_ditherers + 1);

    for (i = 0; i < n_ditherers; i++) {
      ditherers[i].value = i;
      ditherers[i].value_name = g_strdup (aa_dithernames[i]);
      ditherers[i].value_nick =
          g_strdelimit (g_strdup (aa_dithernames[i]), " _", '-');
    }
    ditherers[i].value = 0;
    ditherers[i].value_name = NULL;
    ditherers[i].value_nick = NULL;

    dither_type = g_enum_register_static ("GstAASinkDitherers", ditherers);
  }
  return dither_type;
}

static void
gst_aasink_class_init (GstAASinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = gst_aasink_set_property;
  gobject_class->get_property = gst_aasink_get_property;

  /* FIXME: add long property descriptions */
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WIDTH,
      g_param_spec_int ("width", "width", "width", G_MININT, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_HEIGHT,
      g_param_spec_int ("height", "height", "height", G_MININT, G_MAXINT, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DRIVER,
      g_param_spec_enum ("driver", "driver", "driver", GST_TYPE_AADRIVERS, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_DITHER,
      g_param_spec_enum ("dither", "dither", "dither", GST_TYPE_AADITHER, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_BRIGHTNESS,
      g_param_spec_int ("brightness", "brightness", "brightness", G_MININT,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_CONTRAST,
      g_param_spec_int ("contrast", "contrast", "contrast", G_MININT, G_MAXINT,
          0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_GAMMA,
      g_param_spec_float ("gamma", "gamma", "gamma", 0.0, 5.0, 1.0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_INVERSION,
      g_param_spec_boolean ("inversion", "inversion", "inversion", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_RANDOMVAL,
      g_param_spec_int ("randomval", "randomval", "randomval", G_MININT,
          G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass),
      PROP_FRAMES_DISPLAYED, g_param_spec_int ("frames-displayed",
          "frames displayed", "frames displayed", G_MININT, G_MAXINT, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FRAME_TIME,
      g_param_spec_int ("frame-time", "frame time", "frame time", G_MININT,
          G_MAXINT, 0, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  gst_element_class_add_static_pad_template (gstelement_class, &sink_template);

  gst_element_class_set_static_metadata (gstelement_class,
      "ASCII art video sink", "Sink/Video", "An ASCII art videosink",
      "Wim Taymans <wim.taymans@chello.be>");

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_aasink_change_state);

  gstbasesink_class->fixate = GST_DEBUG_FUNCPTR (gst_aasink_fixate);
  gstbasesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_aasink_setcaps);
  gstbasesink_class->get_times = GST_DEBUG_FUNCPTR (gst_aasink_get_times);
  gstbasesink_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_aasink_propose_allocation);

  gstvideosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_aasink_show_frame);
}

static GstCaps *
gst_aasink_fixate (GstBaseSink * bsink, GstCaps * caps)
{
  GstStructure *structure;

  caps = gst_caps_make_writable (caps);

  structure = gst_caps_get_structure (caps, 0);

  gst_structure_fixate_field_nearest_int (structure, "width", 320);
  gst_structure_fixate_field_nearest_int (structure, "height", 240);
  gst_structure_fixate_field_nearest_fraction (structure, "framerate", 30, 1);

  caps = GST_BASE_SINK_CLASS (parent_class)->fixate (bsink, caps);

  return caps;
}

static gboolean
gst_aasink_setcaps (GstBaseSink * basesink, GstCaps * caps)
{
  GstAASink *aasink;
  GstVideoInfo info;

  aasink = GST_AASINK (basesink);

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_caps;

  aasink->info = info;

  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_DEBUG_OBJECT (aasink, "invalid caps");
    return FALSE;
  }
}

static void
gst_aasink_init (GstAASink * aasink)
{
  memcpy (&aasink->ascii_surf, &aa_defparams,
      sizeof (struct aa_hardware_params));
  aasink->ascii_parms.bright = 0;
  aasink->ascii_parms.contrast = 16;
  aasink->ascii_parms.gamma = 1.0;
  aasink->ascii_parms.dither = 0;
  aasink->ascii_parms.inversion = 0;
  aasink->ascii_parms.randomval = 0;
  aasink->aa_driver = 0;
}

static void
gst_aasink_scale (GstAASink * aasink, guchar * src, guchar * dest,
    gint sw, gint sh, gint ss, gint dw, gint dh)
{
  gint ypos, yinc, y;
  gint xpos, xinc, x;

  g_return_if_fail ((dw != 0) && (dh != 0));

  ypos = 0x10000;
  yinc = (sh << 16) / dh;
  xinc = (sw << 16) / dw;

  for (y = dh; y; y--) {
    while (ypos > 0x10000) {
      ypos -= 0x10000;
      src += ss;
    }
    xpos = 0x10000;
    {
      guchar *destp = dest;
      guchar *srcp = src;

      for (x = dw; x; x--) {
        while (xpos >= 0x10000L) {
          srcp++;
          xpos -= 0x10000L;
        }
        *destp++ = *srcp;
        xpos += xinc;
      }
    }
    dest += dw;
    ypos += yinc;
  }
}

static void
gst_aasink_get_times (GstBaseSink * sink, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  *start = GST_BUFFER_TIMESTAMP (buffer);
  if (GST_BUFFER_DURATION_IS_VALID (buffer))
    *end = *start + GST_BUFFER_DURATION (buffer);
}

static gboolean
gst_aasink_propose_allocation (GstBaseSink * bsink, GstQuery * query)
{
  GstCaps *caps;
  GstVideoInfo info;
  guint size;

  gst_query_parse_allocation (query, &caps, NULL);

  if (caps == NULL)
    goto no_caps;

  if (!gst_video_info_from_caps (&info, caps))
    goto invalid_caps;

  size = GST_VIDEO_INFO_SIZE (&info);

  /* we need at least 2 buffer because we hold on to the last one */
  gst_query_add_allocation_pool (query, NULL, size, 2, 0);

  /* we support various metadata */
  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return TRUE;

  /* ERRORS */
no_caps:
  {
    GST_DEBUG_OBJECT (bsink, "no caps specified");
    return FALSE;
  }
invalid_caps:
  {
    GST_DEBUG_OBJECT (bsink, "invalid caps specified");
    return FALSE;
  }
}

static GstFlowReturn
gst_aasink_show_frame (GstVideoSink * videosink, GstBuffer * buffer)
{
  GstAASink *aasink;
  GstVideoFrame frame;

  aasink = GST_AASINK (videosink);

  GST_DEBUG ("show frame");

  if (!gst_video_frame_map (&frame, &aasink->info, buffer, GST_MAP_READ))
    goto invalid_frame;

  gst_aasink_scale (aasink, GST_VIDEO_FRAME_PLANE_DATA (&frame, 0),     /* src */
      aa_image (aasink->context),       /* dest */
      GST_VIDEO_INFO_WIDTH (&aasink->info),     /* sw */
      GST_VIDEO_INFO_HEIGHT (&aasink->info),    /* sh */
      GST_VIDEO_FRAME_PLANE_STRIDE (&frame, 0), /* ss */
      aa_imgwidth (aasink->context),    /* dw */
      aa_imgheight (aasink->context));  /* dh */

  aa_render (aasink->context, &aasink->ascii_parms,
      0, 0, aa_imgwidth (aasink->context), aa_imgheight (aasink->context));
  aa_flush (aasink->context);
  aa_getevent (aasink->context, FALSE);
  gst_video_frame_unmap (&frame);

  return GST_FLOW_OK;

  /* ERRORS */
invalid_frame:
  {
    GST_DEBUG_OBJECT (aasink, "invalid frame");
    return GST_FLOW_ERROR;
  }
}


static void
gst_aasink_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstAASink *aasink;

  aasink = GST_AASINK (object);

  switch (prop_id) {
    case PROP_WIDTH:
      aasink->ascii_surf.width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      aasink->ascii_surf.height = g_value_get_int (value);
      break;
    case PROP_DRIVER:{
      aasink->aa_driver = g_value_get_enum (value);
      break;
    }
    case PROP_DITHER:{
      aasink->ascii_parms.dither = g_value_get_enum (value);
      break;
    }
    case PROP_BRIGHTNESS:{
      aasink->ascii_parms.bright = g_value_get_int (value);
      break;
    }
    case PROP_CONTRAST:{
      aasink->ascii_parms.contrast = g_value_get_int (value);
      break;
    }
    case PROP_GAMMA:{
      aasink->ascii_parms.gamma = g_value_get_float (value);
      break;
    }
    case PROP_INVERSION:{
      aasink->ascii_parms.inversion = g_value_get_boolean (value);
      break;
    }
    case PROP_RANDOMVAL:{
      aasink->ascii_parms.randomval = g_value_get_int (value);
      break;
    }
    default:
      break;
  }
}

static void
gst_aasink_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstAASink *aasink;

  aasink = GST_AASINK (object);

  switch (prop_id) {
    case PROP_WIDTH:{
      g_value_set_int (value, aasink->ascii_surf.width);
      break;
    }
    case PROP_HEIGHT:{
      g_value_set_int (value, aasink->ascii_surf.height);
      break;
    }
    case PROP_DRIVER:{
      g_value_set_enum (value, aasink->aa_driver);
      break;
    }
    case PROP_DITHER:{
      g_value_set_enum (value, aasink->ascii_parms.dither);
      break;
    }
    case PROP_BRIGHTNESS:{
      g_value_set_int (value, aasink->ascii_parms.bright);
      break;
    }
    case PROP_CONTRAST:{
      g_value_set_int (value, aasink->ascii_parms.contrast);
      break;
    }
    case PROP_GAMMA:{
      g_value_set_float (value, aasink->ascii_parms.gamma);
      break;
    }
    case PROP_INVERSION:{
      g_value_set_boolean (value, aasink->ascii_parms.inversion);
      break;
    }
    case PROP_RANDOMVAL:{
      g_value_set_int (value, aasink->ascii_parms.randomval);
      break;
    }
    case PROP_FRAMES_DISPLAYED:{
      g_value_set_int (value, aasink->frames_displayed);
      break;
    }
    case PROP_FRAME_TIME:{
      g_value_set_int (value, aasink->frame_time / 1000000);
      break;
    }
    default:{
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static gboolean
gst_aasink_open (GstAASink * aasink)
{
  if (!aasink->context) {
    aa_recommendhidisplay (aa_drivers[aasink->aa_driver]->shortname);

    aasink->context = aa_autoinit (&aasink->ascii_surf);
    if (aasink->context == NULL) {
      GST_ELEMENT_ERROR (GST_ELEMENT (aasink), LIBRARY, TOO_LAZY, (NULL),
          ("error opening aalib context"));
      return FALSE;
    }
    aa_autoinitkbd (aasink->context, 0);
    aa_resizehandler (aasink->context, (void *) aa_resize);
  }
  return TRUE;
}

static gboolean
gst_aasink_close (GstAASink * aasink)
{
  aa_close (aasink->context);
  aasink->context = NULL;

  return TRUE;
}

static GstStateChangeReturn
gst_aasink_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;


  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      if (!gst_aasink_open (GST_AASINK (element)))
        goto open_failed;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_aasink_close (GST_AASINK (element));
      break;
    default:
      break;
  }

  return ret;

open_failed:
  {
    return GST_STATE_CHANGE_FAILURE;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "aasink", GST_RANK_NONE, GST_TYPE_AASINK))
    return FALSE;

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    aasink,
    "ASCII Art video sink",
    plugin_init, VERSION, "LGPL", GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
